#pragma once
#include <Arduino.h>

#include "playlistFlash.h"

// -----------------------------------------------------------------------------
// FULL URL LIST (Disc 1 + Disc 2 + Disc 3) — fully URL-encoded
// Base: http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// System Playlist belongs here
// Example playlist exists
// -----------------------------------------------------------------------------

const char* urls_flash[] = {

/* ===================== Be My Baby [Disc 1] ===================== */
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-01%20Be%20My%20Baby.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-02%20Then%20He%20Kissed%20Me.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-03%20You%20Can%27t%20Hurry%20Love.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-04%20Rescue%20Me.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-05%20Jimmy%20Mack.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-06%20Will%20You%20Love%20Me%20Tomorrow.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-07%20It%20Might%20As%20Well%20Rain%20Until%20September.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-08%20He%27s%20So%20Fine.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-09%20Chapel%20Of%20Love.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-10%20I%20Will%20Follow%20Him.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-11%20Move%20Over%20Darling.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-12%20Fever.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-13%20My%20Baby%20Just%20Cares%20For%20Me.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-14%20I%20Just%20Want%20To%20Make%20Love%20To%20You.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-15%20Please%20Mr.%20Postman.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-16%20It%27s%20My%20Party.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-17%20My%20Boy%20Lollipop.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-18%20My%20Boyfriend%27s%20Back.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-19%20Tell%20Him.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-20%20Da%20Doo%20Ron%20Ron.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-21%20The%20Boy%20From%20New%20York%20City.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-22%20Just%20One%20Look.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-23%20Chains.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-24%20%28The%20Best%20Part%20Of%29%20Breakin%27%20Up.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%201%5D/1-25%20%28Today%20I%20Met%29%20The%20Boy%20I%27m%20Gonna%20Marry.mp3",

/* ===================== Be My Baby [Disc 2] ===================== */
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-01%20My%20Guy.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-02%20I%20Only%20Want%20To%20Be%20With%20You.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-03%20River%20Deep%20-%20Mountain%20High.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-04%20Baby%2C%20I%20Love%20You.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-05%20He%20Was%20Really%20Sayin%27%20Somethin%27.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-06%20He%27s%20A%20Rebel.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-07%20Mr.%20Sandman.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-08%20Our%20Day%20Will%20Come.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-09%20A%20Lover%27s%20Concerto.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-10%20I%20Couldn%27t%20Live%20Without%20Your%20Love.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-11%20Long%20Live%20Love.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-12%20Something%20Here%20In%20My%20Heart.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-13%20It%27s%20Getting%20Better.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-14%20Dream%20A%20Little%20Dream%20Of%20Me.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-15%20Goodnight%20Midnight.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-16%20Captain%20Of%20Your%20Ship.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-17%20I%20Want%20You%20To%20Be%20My%20Baby.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-18%20Rose%20Garden.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-19%20Something%20Tells%20Me%20%28Something%27s%20Gonna%20Happen%20Tonight%29.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-20%20Third%20Finger%2C%20Left%20Hand.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-21%20When%20Will%20I%20See%20You%20Again.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%202%5D/2-22%20Baby%20Don%27t%20Change%20Your%20Mind.mp3",

/* ===================== Be My Baby [Disc 3] ===================== */
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-01%20I%20Say%20A%20Little%20Prayer.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-02%20Don%27t%20Make%20Me%20Over.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-03%20Remember%20%28Walkin%27%20In%20The%20Sand%29.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-04%20The%20End%20Of%20The%20World.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-05%20To%20Sir%20With%20Love.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-06%20Love%20Letters.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-07%20Piece%20Of%20My%20Heart.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-08%20Me%20And%20Bobby%20McGee.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-09%20Wedding%20Bell%20Blues.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-10%20Angel%20Of%20The%20Morning.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-11%20Stay%20With%20Me.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-12%20Oh%20No%20Not%20My%20Baby.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-13%20Sunny.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-14%20When%20You%27re%20Young%20And%20In%20Love.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-15%20I%27m%20Gonna%20Run%20Away%20From%20You.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-16%20My%20Man%2C%20A%20Sweet%20Man.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-17%20Moonlight%2C%20Music%20And%20You.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-18%20Time%20Will%20Pass%20You%20By.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-19%20Nothing%20But%20A%20Heartache.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-20%20Hard%20To%20Handle.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-21%20Make%20Me%20Yours.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-22%20B-A-B-Y.mp3",
"http://81.2.125.100/codec_board/Girls%20From%20The%20Sixties/Be%20My%20Baby%20%5BDisc%203%5D/3-23%20When%20You%20Walk%20In%20The%20Room.mp3",

};

const size_t PLAYLIST_COUNT = sizeof(urls_flash) / sizeof(urls_flash[0]);
