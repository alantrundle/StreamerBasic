#pragma once
#include <stdint.h>
#include <stddef.h>
#include <esp_heap_caps.h>

#include "Config.h"

#ifdef __cplusplus
extern "C" {
#endif

int JpegDecode565(
    const uint8_t* jpeg,
    size_t         jpeg_len,
    uint16_t*      out120);

#ifdef __cplusplus
}
#endif
