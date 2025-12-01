#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// ---- Lightweight ID3v2 meta ----
struct ID3v2Meta {
  bool     header_found;     // set to true once we captured+parsed a tag
  uint8_t  ver_major;        // typically 3 or 4
  uint8_t  ver_minor;
  uint32_t tag_bytes;        // bytes collected (header+body [+footer if any])
  char     title[64];
  char     artist[64];
  char     album[64];
  char     year[8];
  int      track;
  char     genre[32];
};



// Collector (per-session)
struct ID3v2Collector {
  uint8_t* buf_psram;        // PSRAM buffer holding the tag
  size_t   need;             // total expected bytes (capped to 3 packets)
  size_t   have;             // collected so far
  int      pkts;             // how many packets consumed (<=3)
  bool     collecting;       // actively reading the tag
  bool     checked_head;     // we only check for ID3 at absolute start once
};

#ifdef __cplusplus
extern "C" {
#endif

void id3v2_reset_meta(ID3v2Meta* m);
void id3v2_reset_collector(ID3v2Collector* c);
void id3v2_free_collector(ID3v2Collector* c);

// Try to begin collection if `buf` looks like an ID3 header right at stream start.
// `absolute_stream_pos` must be the absolute byte offset in the file/stream.
void id3v2_try_begin(const uint8_t* buf, size_t len,
                     uint64_t absolute_stream_pos,
                     size_t max_packet_bytes,
                     ID3v2Collector* c);

// Consume up to `len` bytes from `buf` into the collector. Returns bytes consumed.
// If the tag finishes (or 3 packets reached), parses meta and stops collecting.
size_t id3v2_consume(const uint8_t* buf, size_t len, ID3v2Collector* c, ID3v2Meta* m);

bool id3_fetch_album_art_jpeg(
    const char* artist,
    const char* album,
    uint8_t**   out_jpeg,
    size_t*    out_len);

#ifdef __cplusplus
}
#endif
