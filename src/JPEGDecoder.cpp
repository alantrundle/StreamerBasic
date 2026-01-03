#include "JPEGDecoder.h"
#include "tjpgd.h"
#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#define THUMB_W 120
#define THUMB_H 120

// JPEG input buffer
static const uint8_t* s_jpg_ptr = nullptr;
static size_t         s_jpg_len = 0;
static size_t         s_jpg_pos = 0;

// Output thumbnail buffer
static uint16_t* s_out = nullptr;

// Decoded (scaled) dimensions reported by TJpgD for this decode
static uint16_t s_dec_w = 0;
static uint16_t s_dec_h = 0;

// Resample mapping: for each output x/y, which decoded x/y to sample
// Stored as uint16 to keep it fast.
static uint16_t s_map_x[THUMB_W];
static uint16_t s_map_y[THUMB_H];

// TJpgD input callback (matches your tjpgd.h)
static uint16_t jd_input(JDEC* jd, uint8_t* buf, uint16_t len)
{
    (void)jd;
    if (s_jpg_pos >= s_jpg_len) return 0;

    uint16_t n = (uint16_t)min((size_t)len, (size_t)(s_jpg_len - s_jpg_pos));
    if (buf) memcpy(buf, s_jpg_ptr + s_jpg_pos, n);
    s_jpg_pos += n;
    return n;
}

// Build nearest-neighbour maps: output pixel -> decoded pixel
static void build_maps(uint16_t dec_w, uint16_t dec_h)
{
    // Map x: ox in [0..119] -> dx in [0..dec_w-1]
    for (uint16_t ox = 0; ox < THUMB_W; ox++) {
        // (ox + 0.5) * dec_w / THUMB_W - 0.5  (centered)
        // Use integer math: dx = (ox * dec_w) / THUMB_W
        uint32_t dx = (uint32_t)ox * (uint32_t)dec_w / (uint32_t)THUMB_W;
        if (dx >= dec_w) dx = dec_w - 1;
        s_map_x[ox] = (uint16_t)dx;
    }

    for (uint16_t oy = 0; oy < THUMB_H; oy++) {
        uint32_t dy = (uint32_t)oy * (uint32_t)dec_h / (uint32_t)THUMB_H;
        if (dy >= dec_h) dy = dec_h - 1;
        s_map_y[oy] = (uint16_t)dy;
    }
}

// TJpgD output callback
// bitmap is RGB565 pixels for rectangle rect in decoded coordinate space.
static uint16_t jd_output(JDEC* jd, void* bitmap, JRECT* rect)
{
    (void)jd;

    uint16_t* src = (uint16_t*)bitmap;

    // For each pixel in this output block, see if it's needed by our 120x120 resample.
    // We invert the mapping by brute force per pixel block:
    // For each output row oy that maps into [rect->top..rect->bottom],
    // and each output col ox that maps into [rect->left..rect->right],
    // sample the corresponding decoded pixel.

    // Quick reject: if rect has no overlap with any mapped y, skip reading? Not possible
    // because we must advance src. We'll just walk src once and write matches.

    // We'll do it this way (fast enough):
    // Walk through decoded block pixels and write them into out where they are selected.
    // For that we need to know which ox want each dx. That's expensive.
    //
    // Instead, do the simple method:
    // - Copy this block into out by scanning output pixels and picking needed samples
    //   by indexing src. This avoids inverted maps.

    uint16_t rw = (uint16_t)(rect->right - rect->left + 1);

    for (uint16_t oy = 0; oy < THUMB_H; oy++) {
        uint16_t dy = s_map_y[oy];
        if (dy < rect->top || dy > rect->bottom) continue;

        uint16_t* out_row = s_out + (uint32_t)oy * THUMB_W;

        // row offset inside this block
        uint16_t by = (uint16_t)(dy - rect->top);
        uint16_t* src_row = src + (uint32_t)by * rw;

        for (uint16_t ox = 0; ox < THUMB_W; ox++) {
            uint16_t dx = s_map_x[ox];
            if (dx < rect->left || dx > rect->right) continue;

            uint16_t bx = (uint16_t)(dx - rect->left);
            out_row[ox] = src_row[bx];
        }
    }

    return 1;
}

// Exported API (matches your JPEGDecoder.h exactly)
extern "C" int JpegDecode565(const uint8_t* jpeg, size_t jpeg_len, uint16_t* out120)
{
    if (!jpeg || !out120 || jpeg_len < 16) return 1;

    JDEC jd;

    // Put workbuf off-stack to prevent HTTPFill overflow
    static uint8_t workbuf[4096];

    s_jpg_ptr = jpeg;
    s_jpg_len = jpeg_len;
    s_jpg_pos = 0;

    s_out = out120;
    memset(s_out, 0, THUMB_W * THUMB_H * 2);

    JRESULT r = jd_prepare(&jd, jd_input, workbuf, sizeof(workbuf), nullptr);
    if (r != JDR_OK) {
        Serial.printf("[JPEG] jd_prepare failed: %d\n", (int)r);
        return 1;
    }

    // Choose scale to reduce decode workload (still resample to 120x120)
    uint8_t scale = 0;
    while (((jd.width >> scale) > 480 || (jd.height >> scale) > 480) && scale < 3) {
        // Keep decoded size reasonable (<=480) for speed
        scale++;
    }

    s_dec_w = (uint16_t)(jd.width >> scale);
    s_dec_h = (uint16_t)(jd.height >> scale);

    if (s_dec_w == 0 || s_dec_h == 0) return 2;

    build_maps(s_dec_w, s_dec_h);

    Serial.printf("[JPEG] %ux%u -> %ux%u (scale 1/%u) -> 120x120\n",
                  (unsigned)jd.width, (unsigned)jd.height,
                  (unsigned)s_dec_w, (unsigned)s_dec_h,
                  (unsigned)(1U << scale));

    r = jd_decomp(&jd, jd_output, scale);
    if (r != JDR_OK) {
        Serial.printf("[JPEG] jd_decomp failed: %d\n", (int)r);
        return 2;
    }

    return 0;
}