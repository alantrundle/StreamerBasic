#include "images.h"

const ext_img_desc_t images[8] = {
    { "a2dp_on", &img_a2dp_on },
    { "a2dp_off", &img_a2dp_off },
    { "i2s_on", &img_i2s_on },
    { "i2s_off", &img_i2s_off },
    { "id3_artist", &img_id3_artist },
    { "id3_title", &img_id3_title },
    { "id3_track", &img_id3_track },
    { "id3_album", &img_id3_album },
};
