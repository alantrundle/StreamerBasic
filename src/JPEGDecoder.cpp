#include "JPEGDecoder.h"
#include "tjpgd.h"
#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)
  #include <esp_heap_caps.h>
#else
  #define heap_caps_malloc(sz, caps) malloc(sz)
  #define heap_caps_free(p) free(p)
  #define MALLOC_CAP_SPIRAM 0
  #define MALLOC_CAP_8BIT   0
#endif

extern "C" {

typedef struct {
    const uint8_t* data;
    size_t len;
    size_t pos;
} jpeg_in_t;

static jpeg_in_t gin;

static uint16_t* g_full     = nullptr;
static int       g_full_w   = 0;
static int       g_full_h   = 0;

// ------------------------------------------------------------
// TJpgDec input callback
// ------------------------------------------------------------
static uint16_t jd_input(JDEC*, uint8_t* buf, uint16_t len)
{
    size_t remaining = gin.len - gin.pos;
    if (len > remaining) len = remaining;

    if (buf)
        memcpy(buf, gin.data + gin.pos, len);

    gin.pos += len;
    return len;
}

// ------------------------------------------------------------
// TJpgDec output callback — writes FULL image
// ------------------------------------------------------------
static uint16_t jd_output(JDEC*, void* bitmap, JRECT* r)
{
    uint16_t* src = (uint16_t*)bitmap;

    for (int y = r->top; y <= r->bottom; y++) {
        uint16_t* dst = g_full + y * g_full_w + r->left;
        int w = r->right - r->left + 1;
        memcpy(dst, src, w * sizeof(uint16_t));
        src += w;
    }
    return 1;
}

// ------------------------------------------------------------
// Detect baseline vs progressive JPEG
// ------------------------------------------------------------
static int detect_progressive(const uint8_t* d, size_t len)
{
    if (len < 4 || d[0] != 0xFF || d[1] != 0xD8)
        return -1;

    for (size_t i = 2; i + 3 < len; ) {
        if (d[i] != 0xFF) { i++; continue; }

        uint8_t m = d[i + 1];

        if (m == 0xC0) return 0; // baseline
        if (m == 0xC2) return 1; // progressive

        if (i + 4 >= len) break;
        uint16_t l = (d[i+2] << 8) | d[i+3];
        if (l < 2) break;

        i += 2 + l;
    }
    return -2;
}

// ------------------------------------------------------------
// Public API — decode → scale → 120x120
// ------------------------------------------------------------
int JpegDecode565(
    const uint8_t* jpeg,
    size_t         jpeg_len,
    uint16_t*      out120
){
    if (!jpeg || !out120)
        return 2;

    int t = detect_progressive(jpeg, jpeg_len);
    if (t == 1) return 1;
    if (t != 0) return 2;

    static uint8_t* work = nullptr;
    if (!work) {
        work = (uint8_t*)heap_caps_malloc(
            TJPGD_WORKSPACE_SIZE,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
        );
        if (!work) return 2;
    }

    JDEC jd;

    gin.data = jpeg;
    gin.len  = jpeg_len;
    gin.pos  = 0;

    if (jd_prepare(&jd, jd_input,
                   work,
                   TJPGD_WORKSPACE_SIZE,
                   nullptr) != JDR_OK)
        return 2;

    g_full_w = jd.width;
    g_full_h = jd.height;

    g_full = (uint16_t*)heap_caps_malloc(
        g_full_w * g_full_h * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    if (!g_full)
        return 2;

    if (jd_decomp(&jd, jd_output, 0) != JDR_OK) {
        heap_caps_free(g_full);
        g_full = nullptr;
        return 2;
    }

    // ---- Downscale to 120x120 (nearest, safe)
    for (int y = 0; y < THUMB_H; y++) {
        int sy = y * g_full_h / THUMB_H;
        for (int x = 0; x < THUMB_W; x++) {
            int sx = x * g_full_w / THUMB_W;
            out120[y * THUMB_W + x] =
                g_full[sy * g_full_w + sx];
        }
    }

    heap_caps_free(g_full);
    g_full = nullptr;

    return 0;
}

} // extern "C"
