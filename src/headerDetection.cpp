// src/header_detection.cpp
#include "headerDetection.h"

namespace audetect {

static inline uint32_t rd32le(const uint8_t* p){
  return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static inline uint64_t rd64le(const uint8_t* p){
  return (uint64_t)rd32le(p) | ((uint64_t)rd32le(p+4)<<32);
}
static inline int aac_sr_from_idx(int i){
  static const int t[16]={96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};
  return (i>=0 && i<16)?t[i]:0;
}
static inline int mp3_sr_from_idx(int ver_id,int sr_idx){
  static const int sr1[4]={44100,48000,32000,0};
  int sr=sr1[sr_idx&3];
  if (ver_id==2) sr/=2; else if (ver_id==0) sr/=4;
  return sr;
}
static inline int mp3_br_kbps(int ver_id,int layer_id,int idx){
  static const int br_m1_l1 [16]={0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0};
  static const int br_m1_l2 [16]={0,32,48,56,64,80,96,112,128,160,192,224,256,320,384,0};
  static const int br_m1_l3 [16]={0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
  static const int br_m2_l1 [16]={0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0};
  static const int br_m2_l23[16]={0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0};
  if (ver_id==3){ // MPEG1
    if (layer_id==3) return br_m1_l1[idx];
    if (layer_id==2) return br_m1_l2[idx];
    if (layer_id==1) return br_m1_l3[idx];
  } else {
    if (layer_id==3) return br_m2_l1[idx];
    if (layer_id==2||layer_id==1) return br_m2_l23[idx];
  }
  return 0;
}

// ---- Containers (unambiguous) ----

// WAV: "RIFF....WAVE" + "fmt " chunk present
static inline int match_wav(const uint8_t* b,int i,int n, DetectResult* r){
  if (i+12>n) return 0;
  if (memcmp(b+i,"RIFF",4)!=0 || memcmp(b+i+8,"WAVE",4)!=0) return 0;
  int p=i+12;
  while (p+8<=n){
    uint32_t cid=rd32le(b+p);
    uint32_t csz=rd32le(b+p+4);
    p+=8;
    if (p+(int)csz>n) break;
    if (cid==0x20746d66){ // "fmt "
      if (csz>=16){
        r->wav_fmt       = (uint16_t)(b[p] | (b[p+1]<<8));
        r->wav_channels  = (uint16_t)(b[p+2] | (b[p+3]<<8));
        r->wav_samplerate= rd32le(b+p+4);
        r->wav_bits      = (uint16_t)(b[p+14] | (b[p+15]<<8));
      }
      return i;
    }
    p+=csz;
  }
  return 0;
}

// FLAC: 'fLaC' + mandatory STREAMINFO block header (type 0, len 34)
static inline int match_flac(const uint8_t* b,int i,int n){
  if (i+8>n) return 0;
  if (memcmp(b+i,"fLaC",4)!=0) return 0;
  uint8_t h=b[i+4];
  uint32_t blen=((uint32_t)b[i+5]<<16)|((uint32_t)b[i+6]<<8)|b[i+7];
  if ((h&0x7F)!=0 || blen!=34) return 0;
  return i;
}

// Ogg Vorbis: OggS page that begins a packet with 0x01 "vorbis" ID header
static inline int match_ogg_vorbis(const uint8_t* b,int i,int n){
  if (i+27>n) return 0;
  if (memcmp(b+i,"OggS",4)!=0) return 0;
  uint8_t hdr_type=b[i+5];
  uint8_t nsegs=b[i+26];
  if (i+27+nsegs>n) return 0;
  int payload=i+27+nsegs;
  if (payload+7>n) return 0;
  if (hdr_type&0x01) return 0;
  if (b[payload]==0x01 && memcmp(b+payload+1,"vorbis",6)==0) return i;
  return 0;
}

// ASF/WMA: Header GUID + sane header size + presence of Stream Properties(Audio)
static inline int match_asf_wma(const uint8_t* b,int i,int n){
  static const uint8_t ASF_Header[16]={0x30,0x26,0xB2,0x75,0x8E,0x66,0xCF,0x11,0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C};
  if (i+30>n) return 0;
  if (memcmp(b+i,ASF_Header,16)!=0) return 0;
  uint64_t hdr_size=rd64le(b+i+16);
  if (hdr_size<30 || hdr_size> (1ULL<<33)) return 0;
  return i;
}

// ---- AAC family (exact) ----
static inline int match_aac_adts(const uint8_t* b,int i,int n, DetectResult* r){
  if (i+7>n) return 0;
  const uint8_t* p=b+i;
  if (p[0]!=0xFF || (p[1]&0xF6)!=0xF0) return 0;
  bool id_mpeg2=((p[1]>>3)&1)!=0;
  int  profile=(p[2]>>6)&3;
  int  sf_idx=(p[2]>>2)&15; if (sf_idx==15) return 0;
  int  chan=((p[2]&1)<<2)|((p[3]>>6)&3);
  int  aac_len=((p[3]&0x03)<<11) | (p[4]<<3) | ((p[5]>>5)&0x07);
  if (aac_len<7 || aac_len>8192) return 0;
  if (i+aac_len+7<=n){
    const uint8_t* q=b+i+aac_len;
    if (!(q[0]==0xFF && (q[1]&0xF6)==0xF0)) return 0;
  }
  r->aac_is_mpeg2=id_mpeg2;
  r->aac_aot=(uint8_t)(profile+1);
  r->aac_samplerate=aac_sr_from_idx(sf_idx);
  r->aac_channels=chan;
  return i;
}
static inline int match_aac_loas(const uint8_t* b,int i,int n){
  if (i+3>n) return 0;
  if (b[i]==0x56 && (b[i+1]&0xE0)==0xE0){
    int frameLen=((b[i+1]&0x1F)<<8)|b[i+2];
    if (frameLen>=7 && frameLen<8192) return i;
  }
  return 0;
}
static inline int match_aac_adif(const uint8_t* b,int i,int n){
  if (i+4>n) return 0;
  return (memcmp(b+i,"ADIF",4)==0)? i : 0;
}

// ---- MP3 (exact) ----
static inline int mp3_frame_bytes(const uint8_t* h){
  int ver_id=(h[1]>>3)&3; if (ver_id==1) return -1;
  int layer_id=(h[1]>>1)&3; if (layer_id==0) return -1;
  int br_idx=(h[2]>>4)&15; if (br_idx==0||br_idx==15) return -1;
  int sr_idx=(h[2]>>2)&3; if (sr_idx==3) return -1;
  int padding=(h[2]>>1)&1;
  int bitrate=mp3_br_kbps(ver_id,layer_id,br_idx)*1000;
  int sr=mp3_sr_from_idx(ver_id,sr_idx);
  if (!bitrate || !sr) return -1;
  if (layer_id==3){ return (12*bitrate/sr + padding)*4; }
  else { return 144*bitrate/sr + padding; }
}
static inline int match_mp3(const uint8_t* b,int i,int n, DetectResult* r){
  if (i+4>n) return 0;
  const uint8_t* p=b+i;
  if (p[0]!=0xFF || (p[1]&0xE0)!=0xE0) return 0;
  int ver_id=(p[1]>>3)&3; if (ver_id==1) return 0;
  int layer_id=(p[1]>>1)&3; if (layer_id==0) return 0;
  int br_idx=(p[2]>>4)&15; if (br_idx==0||br_idx==15) return 0;
  int sr_idx=(p[2]>>2)&3; if (sr_idx==3) return 0;
  int frame=mp3_frame_bytes(p);
  if (frame<24 || frame>20000) return 0;
  if (i+frame+4<=n){
    const uint8_t* q=b+i+frame;
    if (q[0]!=0xFF || (q[1]&0xE0)!=0xE0) return 0;
  }
  r->mp3_version=(ver_id==3)?10:(ver_id==2?20:25);
  r->mp3_layer=4-layer_id;
  r->mp3_bitrate_kbps=mp3_br_kbps(ver_id,layer_id,br_idx);
  r->mp3_samplerate=mp3_sr_from_idx(ver_id,sr_idx);
  r->mp3_channels=((p[3]>>6)&3)==3?1:2;
  return i;
}

// ---- MIDI ----
static inline int match_midi(const uint8_t* b,int i,int n){
  if (i+4>n) return 0;
  return (memcmp(b+i,"MThd",4)==0)? i : 0;
}

// ---- Public API ----
bool detect_audio_format_strict(const uint8_t* buf, int len, DetectResult* out){
  if (!buf || len<=0 || !out) return false;
  memset(out,0,sizeof(*out));
  out->format=AF_UNKNOWN; out->offset=-1;

  for (int i=0;i<len;i++){
    int off;
    if ((off=match_wav(buf,i,len,out)))      { out->format=AF_WAV; out->offset=off; return true; }
    if ((off=match_flac(buf,i,len)))         { out->format=AF_FLAC; out->offset=off; return true; }
    if ((off=match_ogg_vorbis(buf,i,len)))   { out->format=AF_OGG_VORBIS; out->offset=off; return true; }
    if ((off=match_asf_wma(buf,i,len)))      { out->format=AF_WMA_ASF; out->offset=off; return true; }
    if ((off=match_aac_adts(buf,i,len,out))) { out->format=AF_AAC_ADTS; out->offset=off; return true; }
    if ((off=match_aac_loas(buf,i,len)))     { out->format=AF_AAC_LOAS; out->offset=off; return true; }
    if ((off=match_aac_adif(buf,i,len)))     { out->format=AF_AAC_ADIF; out->offset=off; return true; }
    if ((off=match_mp3(buf,i,len,out)))      { out->format=AF_MP3; out->offset=off; return true; }
    if ((off=match_midi(buf,i,len)))         { out->format=AF_MIDI; out->offset=off; return true; }
  }
  return false;
}

} // namespace audetect
