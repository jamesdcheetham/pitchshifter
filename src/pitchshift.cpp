/*
 * File: vibrato.cpp
 *
 * Simple vibrato with pitch shifter for Modulation FX, Delay FX and Reverb FX
 * 
 * 2019 (c) Oleg Burdaev
 * mailto: dukesrg@gmail.com
 *
 */

#include "buffer_ops.h"
//#include "fxwrapper.h"
#include "osc_api.h"

//filter
#include "usermodfx.h"
#include "biquad.hpp"
//filter

#define MAX_SHIFT 12 //maximum frequency shift spread in semitones
#define BUF_SIZE (k_samplerate / 100)
#define F_FACTOR 1.0594631f //chromatic semitone frequency factor

static float s_speed = 0.f;
static float s_read_pos;
static uint32_t s_write_pos;
static __sdram f32pair_t s_loop[BUF_SIZE];

//filter
static dsp::BiQuad s_bq_l, s_bq_r;
static dsp::BiQuad s_bqs_l, s_bqs_r;
enum {
  k_polelp = 0,
};
static uint8_t s_type_z, s_type;
static float s_wc_z, s_wc;
static float s_q;
static const float s_fs_recip = 1.f / 48000.f;
//filter


#ifdef FX_MODFX_SUB
static __sdram f32pair_t s_loop_sub[BUF_SIZE];
#endif

void MODFX_INIT(__attribute__((unused)) uint32_t platform, __attribute__((unused)) uint32_t api)
{
  s_read_pos = 0.f;
  s_write_pos = 0;
  buf_clr_f32((float*)s_loop, BUF_SIZE * sizeof(f32pair_t)/sizeof(float));

//filter
  s_wc = s_wc_z = 0.49f;
  s_q = 1.4041f;
  s_type = s_type_z = k_polelp;

  s_bq_l.flush();
  s_bq_r.flush();
  s_bq_l.mCoeffs.setPoleLP(s_wc);
  s_bq_r.mCoeffs = s_bq_l.mCoeffs;

  s_bqs_l.flush();
  s_bqs_r.flush();
  s_bqs_l.mCoeffs = s_bqs_r.mCoeffs = s_bq_l.mCoeffs;
  

//filter

}

void MODFX_PROCESS(const float *xn, float *yn, __attribute__((unused)) const float *sub_xn, __attribute__((unused)) float *sub_yn, uint32_t frames)
{
//	float* yn;
//	const float* xn;
	int fr;
	const float * mx = xn;
  float * __restrict my = yn;
  const float * my_e = my + 2*fr; 
  const float *sx = sub_xn;
  float * __restrict sy = sub_yn;
 
  const float wc = s_wc;

  for (f32pair_t * __restrict x = (f32pair_t*)xn; frames--; x++) {
    fr = frames;
    uint32_t pos = (uint32_t)s_read_pos;
    f32pair_t valp = *x;
    *x = f32pair_linint(s_read_pos - pos, s_loop[pos], s_loop[pos < BUF_SIZE ? pos + 1: 0]);
    s_loop[s_write_pos] = valp;


    s_read_pos += fastpowf(F_FACTOR, s_speed);
    if ((uint32_t)s_read_pos >= BUF_SIZE)
      s_read_pos -= BUF_SIZE;

    s_write_pos++;
    if (s_write_pos >= BUF_SIZE)
      s_write_pos = 0;
  
  //filter

  
  
    s_bq_l.mCoeffs.setPoleLP(1.f - (wc*2.f));
    //s_bq_r.mCoeffs.setPoleLP(1.f - (wc*2.f));	

    s_bq_r.mCoeffs = s_bq_l.mCoeffs;
    s_bqs_l.mCoeffs = s_bq_l.mCoeffs;
    s_bqs_r.mCoeffs = s_bq_l.mCoeffs;  
    s_wc_z = wc;
 
  if (my != my_e) {
    *(my++) = s_bq_l.process_so(*(mx++));
    *(my++) = s_bq_r.process_so(*(mx++));
    *(sy++) = s_bqs_l.process_so(*(sx++));
  *(sy++) = s_bqs_r.process_so(*(sx++)); }
  else {
  my_e = my + 2*fr; }


//filter

  }

}

void MODFX_PARAM(uint8_t index, int32_t value)
{
  const float valf = q31_to_f32(value);
  switch (index) {
  case k_user_modfx_param_time:
  	    s_speed = (valf - .5f) * MAX_SHIFT * 2; //pitch shift
    break;
  case k_user_modfx_param_depth:
		s_wc = valf * valf * 0.49f; //filter
    break;
  default:
    break;
  }
}
