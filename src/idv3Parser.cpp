#include <Arduino.h>
#include "idv3Parser.h"
#include <stdbool.h>
#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)
  #include <esp_heap_caps.h>
#else
  #define heap_caps_malloc(sz, caps) malloc(sz)
  #define heap_caps_free(p) free(p)
  #define MALLOC_CAP_SPIRAM 0
  #define MALLOC_CAP_8BIT   0
#endif

static inline uint32_t synchsafe_to_u32(const uint8_t b[4]) {
  return ((uint32_t)(b[0] & 0x7F) << 21) |
         ((uint32_t)(b[1] & 0x7F) << 14) |
         ((uint32_t)(b[2] & 0x7F) <<  7) |
         ((uint32_t)(b[3] & 0x7F));
}

void id3v2_reset_meta(ID3v2Meta* m) {
  if (!m) return;
  memset(m, 0, sizeof(*m));
}

void id3v2_reset_collector(ID3v2Collector* c) {
  if (!c) return;
  if (c->buf_psram) { heap_caps_free(c->buf_psram); c->buf_psram = nullptr; }
  c->need = c->have = 0;
  c->pkts = 0;
  c->collecting = false;
  c->checked_head = false;
}

void id3v2_free_collector(ID3v2Collector* c) {
  if (!c) return;
  if (c->buf_psram) { heap_caps_free(c->buf_psram); c->buf_psram = nullptr; }
}

// naive text copy: ISO-8859-1 / UTF-8 → copy bytes; UTF-16 → keep LSB
static size_t id3_copy_text(char* out, size_t outcap,
                            const uint8_t* in, size_t len, uint8_t enc) {
  if (!out || outcap == 0) return 0;
  size_t w = 0;
  if (enc == 0 || enc == 3) {
    for (size_t i=0; i<len && w+1<outcap; ++i) {
      if (in[i] == 0) break;
      out[w++] = (char)in[i];
    }
  } else {
    size_t off = 0;
    if (len >= 2 && ((in[0]==0xFF && in[1]==0xFE) || (in[0]==0xFE && in[1]==0xFF))) off = 2;
    for (size_t i=off; i+1<len && w+1<outcap; i+=2) {
      char c = (in[i+1]==0) ? (char)in[i] : '?';
      if (c == 0) break;
      out[w++] = c;
    }
  }
  out[w] = 0;
  return w;
}

static void id3_parse_frames(const uint8_t* tag, size_t len, ID3v2Meta* m) {
  if (!tag || len < 10 || memcmp(tag, "ID3", 3) != 0) return;
  m->ver_major = tag[3];
  m->ver_minor = tag[4];
  uint8_t flags = tag[5];
  uint32_t body = synchsafe_to_u32(&tag[6]); // OK for 2.3/2.4
  size_t pos = 10;

  if (flags & 0x40) {
    if (len < pos + 4) return;
    uint32_t ext = synchsafe_to_u32(&tag[pos]);
    pos += 4 + ext;
    if (pos > len) return;
  }

  const size_t stop = (10 + body <= len) ? (10 + body) : len;
  bool any_found = false;

  Serial.printf("[ID3] v2.%d.%d tag body=%lu bytes\n",
                m->ver_major, m->ver_minor, (unsigned long)body);

  while (pos + 10 <= stop) {
    const uint8_t* f = tag + pos;
    if (f[0]==0 || f[1]==0 || f[2]==0 || f[3]==0) break; // padding
    char id[5]; memcpy(id, f, 4); id[4]=0;

    uint32_t fsz;
    if (m->ver_major >= 4) fsz = synchsafe_to_u32(&f[4]);
    else                   fsz = ((uint32_t)f[4]<<24)|((uint32_t)f[5]<<16)|((uint32_t)f[6]<<8)|f[7];
    if (fsz == 0 || pos + 10 + fsz > len) break;

    const uint8_t* fp = f + 10;
    uint8_t enc = (fsz>0) ? fp[0] : 0;
    const uint8_t* txt = (fsz>0) ? (fp+1) : fp;
    size_t txtlen = (fsz>0) ? (fsz-1) : 0;

    // Temporary text buffer for debug
    char tmp[128];
    tmp[0] = 0;

    if      (!strcmp(id,"TIT2")) id3_copy_text(m->title,  sizeof(m->title),  txt, txtlen, enc);
    else if (!strcmp(id,"TPE1")) id3_copy_text(m->artist, sizeof(m->artist), txt, txtlen, enc);
    else if (!strcmp(id,"TALB")) id3_copy_text(m->album,  sizeof(m->album),  txt, txtlen, enc);
    else if (!strcmp(id,"TRCK")) {
      id3_copy_text(tmp, sizeof(tmp), txt, txtlen, enc);
      m->track = atoi(tmp);
    }
    else if (!strcmp(id,"TCON")) id3_copy_text(m->genre,  sizeof(m->genre),  txt, txtlen, enc);
    else if (!strcmp(id,"TDRC") || !strcmp(id,"TYER"))
                                 id3_copy_text(m->year,   sizeof(m->year),   txt, txtlen, enc);

    if      (!strcmp(id,"TIT2")) Serial.printf("[ID3] Title : %s\n", m->title);
    else if (!strcmp(id,"TPE1")) Serial.printf("[ID3] Artist: %s\n", m->artist);
    else if (!strcmp(id,"TALB")) Serial.printf("[ID3] Album : %s\n", m->album);
    else if (!strcmp(id,"TRCK")) Serial.printf("[ID3] Track : %d\n", m->track);
    else if (!strcmp(id,"TCON")) Serial.printf("[ID3] Genre : %s\n", m->genre);
    else if (!strcmp(id,"TDRC") || !strcmp(id,"TYER"))
                                 Serial.printf("[ID3] Year  : %s\n", m->year);

    any_found = true;
    pos += 10 + fsz;
  }

  if (any_found) {
    m->header_found = true;

    Serial.printf("[ID3] Parsed OK — v2.%d tag\n", m->ver_major);

  }
}

void id3v2_try_begin(const uint8_t* buf, size_t len,
                     uint64_t absolute_stream_pos,
                     size_t max_packet_bytes,
                     ID3v2Collector* c) {
  if (!c || c->collecting || c->checked_head) return;
  c->checked_head = true; // only once at head of stream
  if (absolute_stream_pos != 0) return;
  if (!buf || len < 10) return;
  if (memcmp(buf, "ID3", 3) != 0) return;

  uint8_t ver_major = buf[3];
  uint8_t flags     = buf[5];
  uint32_t body     = synchsafe_to_u32(&buf[6]);
  uint32_t total    = 10 + body + ((ver_major >= 4 && (flags & 0x10)) ? 10 : 0);

  // Clamp: at most 3 packets
  size_t max_total = 3 * max_packet_bytes;
  if (total > max_total) total = (uint32_t)max_total;

  c->need = total;
  c->have = 0;
  c->pkts = 0;
  c->collecting = true;

  if (!c->buf_psram) {
    c->buf_psram = (uint8_t*)heap_caps_malloc(c->need, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!c->buf_psram) {
      // fail closed
      c->collecting = false;
    }
  }
}

size_t id3v2_consume(const uint8_t* buf, size_t len, ID3v2Collector* c, ID3v2Meta* m) {
  if (!c || !c->collecting || !c->buf_psram || len == 0) return 0;
  size_t take = len;
  if (take > (c->need - c->have)) take = (c->need - c->have);
  memcpy(c->buf_psram + c->have, buf, take);
  c->have += take;
  c->pkts++;

  bool done = (c->have >= c->need) || (c->pkts >= 3);
  if (done) {
    if (m) {
      m->tag_bytes = (uint32_t)c->have;
      id3v2_reset_meta(m); // ensure clean slate before parse
      id3_parse_frames(c->buf_psram, c->have, m);
    }
    id3v2_free_collector(c);
    c->collecting = false;
  }
  return take;
}