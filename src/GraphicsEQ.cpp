#include "GraphicsEQ.h"
#include <math.h>

// Simple biquad
struct Biquad {
  float a0, a1, a2;
  float b1, b2;
  float z1L, z2L;
  float z1R, z2R;
};

static Biquad biq;

// Flat by default
static float bass_gain   = 1.0f;
static float treble_gain = 1.0f;

static void biquad_compute(float fs) {
  const float fc = 2000.0f;
  const float Q  = 0.707f;

  float w0 = 2.0f * M_PI * fc / fs;
  float cosw = cosf(w0);
  float sinw = sinf(w0);
  float alpha = sinw / (2.0f * Q);

  float b0 = (1 + cosw) / 2;
  float b1 = -(1 + cosw);
  float b2 = (1 + cosw) / 2;
  float a0 = 1 + alpha;
  float a1 = -2 * cosw;
  float a2 = 1 - alpha;

  biq.a0 = b0 / a0;
  biq.a1 = b1 / a0;
  biq.a2 = b2 / a0;
  biq.b1 = a1 / a0;
  biq.b2 = a2 / a0;
}

void eq_set_samplerate(int samplerate) {
  biquad_compute((float)samplerate);
  biq.z1L = biq.z2L = 0;
  biq.z1R = biq.z2R = 0;
}

static inline int16_t clip(int32_t v) {
  if (v > 32767) return 32767;
  if (v < -32768) return -32768;
  return (int16_t)v;
}

void eq_process_block(int16_t* pcm, size_t samples) {
  for (size_t i = 0; i + 1 < samples; i += 2) {
    float L = pcm[i];
    float R = pcm[i + 1];

    float outL = biq.a0 * L + biq.a1 * biq.z1L + biq.a2 * biq.z2L
                 - biq.b1 * biq.z1L - biq.b2 * biq.z2L;
    float outR = biq.a0 * R + biq.a1 * biq.z1R + biq.a2 * biq.z2R
                 - biq.b1 * biq.z1R - biq.b2 * biq.z2R;

    biq.z2L = biq.z1L;
    biq.z1L = outL;
    biq.z2R = biq.z1R;
    biq.z1R = outR;

    pcm[i]     = clip((int32_t)(outL * bass_gain));
    pcm[i + 1] = clip((int32_t)(outR * treble_gain));
  }
}
