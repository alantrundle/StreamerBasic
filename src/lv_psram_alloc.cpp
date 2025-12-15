#include <stdlib.h>
#include "esp_heap_caps.h"

// Include LVGL's memory header from your libdeps path
// Adjust the relative path if needed, but this is typical for PlatformIO:
#include "../.pio/libdeps/esp32dev/lvgl/src/stdlib/lv_mem.h"

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM

extern "C" {

// Nothing to init/deinit when using heap_caps_*
void lv_mem_init(void)
{
    return; // Nothing to init
}

void lv_mem_deinit(void)
{
    return; // Nothing to deinit
}

// Optional pool API – not used, just stubbed
lv_mem_pool_t lv_mem_add_pool(void * mem, size_t bytes)
{
    LV_UNUSED(mem);
    LV_UNUSED(bytes);
    return nullptr;
}

void lv_mem_remove_pool(lv_mem_pool_t pool)
{
    LV_UNUSED(pool);
}

// *** Core allocation functions that LVGL calls internally ***

void * lv_malloc_core(size_t size)
{
    if(size == 0) return nullptr;
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void * lv_realloc_core(void * p, size_t new_size)
{
    // LVGL may call with new_size == 0 to free
    if(new_size == 0) {
        if(p) heap_caps_free(p);
        return nullptr;
    }
    return heap_caps_realloc(p, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void lv_free_core(void * p)
{
    if(p) heap_caps_free(p);
}

// Monitoring / test hooks – stubbed out
void lv_mem_monitor_core(lv_mem_monitor_t * mon_p)
{
    LV_UNUSED(mon_p);
}

lv_result_t lv_mem_test_core(void)
{
    return LV_RESULT_OK;
}

} // extern "C"

#endif // LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM
