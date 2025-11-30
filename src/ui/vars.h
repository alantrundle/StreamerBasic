#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_IS_PAUSED = 0
};

// Native global variables

extern bool get_var_is_paused();
extern void set_var_is_paused(bool value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/