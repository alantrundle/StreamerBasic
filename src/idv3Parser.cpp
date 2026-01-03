#include <Arduino.h>
#include "idv3Parser.h"
#include <string.h>

#include "JPEGDecoder.h"

#if defined(ARDUINO_ARCH_ESP32)
  #include <esp_heap_caps.h>
#else
  #define heap_caps_malloc(sz, caps) malloc(sz)
  #define heap_caps_free(p) free(p)
  #define MALLOC_CAP_SPIRAM 0
  #define MALLOC_CAP_8BIT   0
#endif

// ============================================================
// Helpers
// ============================================================

static inline uint32_t synchsafe_to_u32(const uint8_t b[4]) {
    return ((uint32_t)(b[0] & 0x7F) << 21) |
           ((uint32_t)(b[1] & 0x7F) << 14) |
           ((uint32_t)(b[2] & 0x7F) << 7)  |
           ((uint32_t)(b[3] & 0x7F));
}

// ============================================================
// JPEG helpers
// ============================================================

static const uint8_t* find_jpeg_start(const uint8_t* buf, size_t len) {
    for (size_t i = 0; i + 1 < len; i++) {
        if (buf[i] == 0xFF && buf[i+1] == 0xD8)
            return buf + i;
    }
    return nullptr;
}

// ============================================================
// APIC parsing (SAFE)
// ============================================================

static void parseAPIC(const uint8_t* frame, size_t len, ID3v2Meta& tags) {

    if (!frame || len < 32)
        return;

    size_t p = 0;
    uint8_t encoding = frame[p++];

    // MIME
    while (p < len && frame[p] != 0) p++;
    if (++p >= len) return;

    uint8_t pictureType = frame[p++];

    // Description
    if (encoding == 0 || encoding == 3) {
        while (p < len && frame[p] != 0) p++;
        p++;
    } else {
        while (p + 1 < len) {
            if (frame[p] == 0 && frame[p+1] == 0) {
                p += 2;
                break;
            }
            p += 2;
        }
    }

    if (p >= len) return;

    const uint8_t* blob     = frame + p;
    size_t         blob_len = len - p;

    const uint8_t* img = find_jpeg_start(blob, blob_len);
    if (!img) {
        Serial.println("[ID3] ❌ APIC no JPEG");
        return;
    }

    size_t img_len = blob + blob_len - img;

    // Scan JPEG markers safely
    for (size_t i = 2; i + 3 < img_len; ) {

        if (img[i] != 0xFF) { i++; continue; }

        uint8_t marker = img[i + 1];

        // Progressive JPEG → reject
        if (marker == 0xC2) {
            Serial.println("[ID3] ❌ Progressive JPEG");
            return;
        }

        // Baseline JPEG
        if (marker == 0xC0) {

            bool front = (pictureType == 3);
            if (tags.albumArtValid && !front)
                return;

            uint16_t* out = (uint16_t*)heap_caps_malloc(
                120 * 120 * sizeof(uint16_t),
                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
            );

            if (!out) {
                Serial.println("[ID3] ❌ PSRAM alloc fail");
                return;
            }

            int rc = JpegDecode565(img, img_len, out);
            if (rc != 0) {
                Serial.printf("[ID3] ❌ JPEG decode failed (%d)\n", rc);
                heap_caps_free(out);
                return;
            }

            // Replace existing safely
            if (tags.albumArtImage)
                heap_caps_free(tags.albumArtImage);

            tags.albumArtImage = out;
            tags.albumArtValid = true;

            Serial.println("[ID3] ✅ Album art decoded");
            return;
        }

        if (i + 4 >= img_len)
            break;

        uint16_t seglen = (img[i+2] << 8) | img[i+3];
        if (seglen < 2 || i + 2 + seglen > img_len)
            break;

        i += 2 + seglen;
    }
}

// ============================================================
// Reset helpers
// ============================================================

void id3v2_reset_meta(ID3v2Meta* m) {
    if (!m) return;

    if (m->albumArtImage) {
        heap_caps_free(m->albumArtImage);
        m->albumArtImage = nullptr;
    }

    m->albumArtValid = false;
    m->header_found  = false;
    m->track         = 0;
    m->tag_bytes     = 0;

    m->ver_major = 0;
    m->ver_minor = 0;

    m->title[0]  = 0;
    m->artist[0] = 0;
    m->album[0]  = 0;
    m->genre[0]  = 0;
    m->year[0]   = 0;
}

void id3v2_reset_collector(ID3v2Collector* c) {
    if (!c) return;
    if (c->buf_psram) {
        heap_caps_free(c->buf_psram);
        c->buf_psram = nullptr;
    }
    c->need = c->have = c->pkts = 0;
    c->collecting = false;
    c->checked_head = false;
}

void id3v2_free_collector(ID3v2Collector* c) {
    if (c && c->buf_psram) {
        heap_caps_free(c->buf_psram);
        c->buf_psram = nullptr;
    }
}

// ============================================================
// Text helper
// ============================================================

static size_t id3_copy_text(char* out, size_t outcap,
                            const uint8_t* in,
                            size_t len,
                            uint8_t enc) {

    if (!out || outcap == 0) return 0;
    size_t w = 0;

    if (enc == 0 || enc == 3) {
        for (size_t i = 0; i < len && w + 1 < outcap; i++) {
            if (in[i] == 0) break;
            out[w++] = in[i];
        }
    } else {
        size_t off = (len >= 2) ? 2 : 0;
        for (size_t i = off; i + 1 < len && w + 1 < outcap; i += 2) {
            char c = in[i+1] ? '?' : in[i];
            if (!c) break;
            out[w++] = c;
        }
    }

    out[w] = 0;
    return w;
}

// ============================================================
// Frame parsing (SAFE)
// ============================================================

static void id3_parse_frames(const uint8_t* tag, size_t len, ID3v2Meta* m) {

    if (!tag || len < 10 || memcmp(tag, "ID3", 3))
        return;

    id3v2_reset_meta(m);

    m->ver_major = tag[3];
    m->ver_minor = tag[4];
    uint8_t flags = tag[5];
    uint32_t body = synchsafe_to_u32(&tag[6]);

    size_t pos = 10;
    size_t stop = (10 + body < len) ? 10 + body : len;

    if (flags & 0x40) { // extended header
        if (pos + 4 > len) return;
        uint32_t ext = synchsafe_to_u32(&tag[pos]);
        pos += 4 + ext;
    }

    while (pos + 10 <= stop) {

        const uint8_t* f = tag + pos;
        if (!f[0]) break;

        char id[5];
        memcpy(id, f, 4);
        id[4] = 0;

        uint32_t fsz = (m->ver_major >= 4)
            ? synchsafe_to_u32(&f[4])
            : ((uint32_t)f[4] << 24) |
              ((uint32_t)f[5] << 16) |
              ((uint32_t)f[6] << 8)  |
               f[7];

        if (fsz == 0 || pos + 10 + fsz > stop)
            break;

        const uint8_t* fp = f + 10;
        uint8_t enc = fp[0];
        const uint8_t* txt = fp + 1;
        size_t txtlen = fsz - 1;

        char tmp[32];

        if      (!strcmp(id,"TIT2")) id3_copy_text(m->title,  sizeof(m->title),  txt, txtlen, enc);
        else if (!strcmp(id,"TPE1")) id3_copy_text(m->artist, sizeof(m->artist), txt, txtlen, enc);
        else if (!strcmp(id,"TALB")) id3_copy_text(m->album,  sizeof(m->album),  txt, txtlen, enc);
        else if (!strcmp(id,"TRCK")) {
            id3_copy_text(tmp, sizeof(tmp), txt, txtlen, enc);
            m->track = atoi(tmp);
        }
        else if (!strcmp(id,"TDRC") || !strcmp(id,"TYER"))
            id3_copy_text(m->year, sizeof(m->year), txt, txtlen, enc);
        else if (!strcmp(id,"APIC"))
            parseAPIC(fp, fsz, *m);

        pos += 10 + fsz;
    }

    m->header_found = true;
    Serial.printf("[ID3] Parsed v2.%d OK\n", m->ver_major);
}

// ============================================================
// Collector API
// ============================================================

void id3v2_try_begin(const uint8_t* buf, size_t len,
                     uint64_t abs_pos,
                     size_t max_packet_bytes,
                     ID3v2Collector* c) {

    if (!c || c->checked_head || abs_pos != 0 || len < 10)
        return;

    c->checked_head = true;

    if (memcmp(buf, "ID3", 3) != 0)
        return;

    uint32_t body = synchsafe_to_u32(&buf[6]);
    uint32_t total = 10 + body;

    uint32_t max_total = 1024 * 1024;
    if (total > max_total) {
        Serial.println("[ID3] ❌ Tag too large, skipped");
        return;
    }

    c->need = total;
    c->have = 0;
    c->pkts = 0;
    c->collecting = true;

    c->buf_psram = (uint8_t*)heap_caps_malloc(
        total,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    if (!c->buf_psram)
        c->collecting = false;
}

size_t id3v2_consume(const uint8_t* buf, size_t len,
                     ID3v2Collector* c,
                     ID3v2Meta* m) {

    if (!c || !c->collecting || !buf || !c->buf_psram)
        return 0;

    size_t take = min(len, c->need - c->have);
    memcpy(c->buf_psram + c->have, buf, take);
    c->have += take;

    if (c->have < c->need)
        return take;

    if (m) {
        m->tag_bytes = c->have;
        id3_parse_frames(c->buf_psram, c->have, m);
    }

    id3v2_free_collector(c);
    c->collecting = false;

    return take;
}
