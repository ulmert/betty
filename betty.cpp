/*
    BSD 3-Clause License

    Copyright (c) 2022, Jacob Ulmert
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * File: betty.cpp
 *
 * Sample player
 *
 */

#include "patterns.h"
#include "samplebank.h"
#include "betty.hpp"

static Betty betty;

// https://wiki.superfamicom.org/bit-rate-reduction-(brr)

inline uint8_t decompress(const uint8_t *pIn, int16_t *pOut) {
  uint8_t range, end=0, loop, filter, counter, temp; // Size of these doesn't matter
  uint16_t input;               // These must be 16-bit
  uint16_t now = 0;                                   // Pointer to where we are in output[];
  
  range = *pIn++;     // Let's just put the header here for now
  end = range&1;         // END bit is bit 0
  loop = range&2;        // LOOP bit is bit 1
  filter = (range >> 2) & 3; // FILTER is bits 2 and 3
  range >>= 4;               // RANGE is the upper 4 bits

  for(counter = 0; counter < 8; counter++){ // Get the next 8 bytes
    temp = *pIn++;                 // Wherever you want to get this from
    input = temp>>4;                      // Get the first nibble of the byte
    input &= 0xF;
    if(input >= 8){                        // The nibble is negative, so make the 
        input |= 0xFFF0;                  // 16-bit value negative
    }
    pOut[now] = (input << range);         // Apply the RANGE value
    
    // Filter processing goes here (explained later)
    if(filter==1){
        pOut[now] += (int16_t)((float)pOut[now-1] * 15.0/16.0);
    }
    else if(filter==2){
        pOut[now]+=(int16_t)(((float)pOut[now-1] * 61.0/32.0) - ((float)pOut[now-2] * 15.0/16.0));
    }
    else if(filter==3){
        pOut[now]+=(int16_t)(((float)pOut[now-1] * 115.0/64.0) - ((float)pOut[now-2] * 13.0/16.0));
    }
    now++;

    // Now do the same thing for the other nibble
    input = temp & 0xF;                   // Get the second nibble of the byte
    if(input >= 8){                        // The nibble is negative, so make the 
        input |= 0xFFF0;                  // 16-bit value negative
    }
    pOut[now] = (input << range);        // Apply the RANGE value

    if(filter==1){
        pOut[now] += (int16_t)((float)pOut[now-1] * 15.0/16.0);
    }
    else if(filter==2){
        pOut[now]+=(int16_t)(((float)pOut[now-1] * 61.0/32.0) - ((float)pOut[now-2] * 15.0/16.0));
    }
    else if(filter==3){
        pOut[now]+=(int16_t)(((float)pOut[now-1] * 115.0/64.0) - ((float)pOut[now-2] * 13.0/16.0));
    }
    now++;
  }
  return end;
}

uint16_t duckEnv[2] = {0,0};

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;

  duckEnv[0] = 512;
  duckEnv[1] = 8192 * 1;
}

void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames)
{
  Betty::Params &p = betty.params;
  Betty::Voice *voices = &betty.voices[0];
  Betty::Sequence &seq = betty.sequence;
  {
#ifdef RETRIG     
    seq.cbsCnt++;
#endif

    float duckGain = 1.0;
    if (seq.duckIdx != -1) {
      duckGain = (float)seq.duckCnt / (float)duckEnv[seq.duckIdx];
      if (duckGain > 1.0) {
        duckGain = 1.0;
      }
      if (seq.duckIdx != 0) {
        duckGain = 1.0 - duckGain;
      }
      seq.duckCnt += frames;
      if (seq.duckCnt >= duckEnv[seq.duckIdx]) {
        seq.duckCnt = 0;
        seq.duckIdx++;
        if (seq.duckIdx == 2) {
          seq.duckIdx = -1;
        }  
      }
      duckGain = (1.0 + duckGain * p.duck);
    }

    if (betty.state.trig) {
      betty.state.trig = false;

#ifdef RETRIG            
      seq.cbsPrStep = seq.cbsCnt;
      seq.cbsCnt = 0;

      bool retrig = (1 << seq.stepIdx) & (uint8_t)(p.repeat * 255);
#endif

      uint8_t v = MAX_VOICES;
      while(v != 0) {
        v--;
        Betty::Voice &voice = voices[v];
        uint8_t i = 6;
        while(i != 2) {
          i--;
          if (ptrns[p.ptrnIdx[v]][v][i] & ((0b1000000000000000) >> seq.stepIdx)) {
            if (v == 0) {
              if (i == 2) {
                seq.duckIdx = 0;
                seq.duckCnt = 0;
              }
              voice.duck = false;
            } else { 
              voice.duck = true;
            }

            voice.pNxtData = samples[(v * 4) + (i - 2)].pData;
            voice.stp = 0;
            voice.dly = (seq.stepIdx & 1) * 2;
            if (ptrns[p.ptrnIdx[v]][v][0] & ((0b1000000000000000) >> seq.stepIdx)) {
              voice.divInit = samples[(v * 4) + (i - 2)].divAc; 
            } else {
              voice.divInit = samples[(v * 4) + (i - 2)].div; 
            }
        
            if (ptrns[p.ptrnIdx[v]][v][1] & ((0b1000000000000000) >> seq.stepIdx)) {
               voice.rate = samples[(v * 4) + (i - 2)].rate * 0.5; 
            } else {
               voice.rate = samples[(v * 4) + (i - 2)].rate; 
            }
#ifdef RETRIG            
            voice.retrigCnt = 0;
#endif
            break;
          }
        }
      }
      if (p.randNxt != p.randCur) {
        p.randCur = p.randNxt;
        uint8_t v = MAX_VOICES;
        while(v != 0) {
          v--;
          p.ptrnIdx[v] = osc_rand() % 0xf;
          p.ptch[v] = 0.1 - osc_white() * 0.1;
        }
      }

      seq.stepIdx++;
      if (seq.stepIdx >= 16) {
        seq.stepIdx = 0;
      }
    }
    uint8_t i = MAX_VOICES;
    while(i != 0) {
      i--;
      Betty::Voice &voice = voices[i];

      if (voice.duck) {
        voice.div = voice.divInit * duckGain;  
      } else {
        voice.div = voice.divInit;  
      }
    
#ifdef RETRIG
      bool trig = false;
      if (voice.retrigCnt) {
        voice.retrigCnt--;
        if (!voice.retrigCnt) {
          trig = true;
          voice.dly = 0;
        }
      }
      if (!voice.stp || trig) {
        if (!voice.dly || trig) {
#else
      if (!voice.stp) {
        if (!voice.dly) {
#endif
          voice.pCurData = voice.pNxtData;

          voice.stp = voice.rate / k_samplerate * (1.0 - p.ptch[i]);
          voice.decompIdx = (16 + 2);
          voice.blockIdx = -9;

          voice.decodebuf[16] = 0;
          voice.decodebuf[17] = 0;

        } else {
          voice.dly--;
        }
      }
    }

    float fr;
    float sig;

    q31_t * __restrict y = (q31_t *)yn;
    const q31_t * y_e = y + frames;

    for (; y != y_e; ) {
      sig = 0;
      uint8_t i = MAX_VOICES;
      while(i != 0) {
        i--;
        Betty::Voice &voice = voices[i];
        if (voice.decompIdx >= (16 + 2)) {
          voice.decompIdx = voice.decompIdx - 16;
          voice.decodebuf[0] = voice.decodebuf[16];
          voice.decodebuf[1] = voice.decodebuf[17];
          voice.blockIdx += 9;
          if (decompress(&voice.pCurData[voice.blockIdx], &voice.decodebuf[2])) {
            voice.pCurData = &dataSilence[0];
            voice.blockIdx = -9;
            voice.decodebuf[16] = 0;
            voice.decodebuf[17] = 0;
          }
        }
        uint16_t bufIdx = (uint16_t)voice.decompIdx;
        float fr = (voice.decompIdx - bufIdx);
        const float sample1 = (float)(voice.decodebuf[(uint16_t)voice.decompIdx - 1]);
        const float sample2 = (float)(voice.decodebuf[(uint16_t)voice.decompIdx]);
        sig += (sample1 * (1.0 - fr) + sample2 * fr) / voice.div;
        voice.decompIdx += voice.stp;
      }
      *(y++) = f32_to_q31(sig * 1.5);
    }
  }
}

void OSC_NOTEON(const user_osc_param_t * const params) 
{
  betty.state.trig = true;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
}

void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  Betty::Params &p = betty.params;

  switch (index) {
    case k_user_osc_param_id1: 
      p.ptrnIdx[0] = value;
      break;
  
    case k_user_osc_param_id2:
      p.ptrnIdx[1] = value;
      break;
    
    case k_user_osc_param_id3:
      p.ptrnIdx[2] = value;
      break;

    case k_user_osc_param_id4:
      p.randNxt = value;
      break;

    case k_user_osc_param_shape:
      p.duck = param_val_to_f32(value) * 4.0;
      break;

    case k_user_osc_param_shiftshape:
      p.mptch = param_val_to_f32(value) * 0.5;
      p.ptch[0] = p.mptch;
      p.ptch[1] = p.mptch * 0.5;
      p.ptch[2] = p.mptch;
      break;

    default:
      break;
  }
}