#pragma once
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
 *  File: betty.hpp
 *
 *  Sample player
 *
 */

#include "userosc.h"
#include "fx_api.h"
#include "biquad.hpp"

#define MAX_VOICES 3

struct Betty {
  
  struct Params {

    uint8_t ptrnIdx[MAX_VOICES];

    float ptch[MAX_VOICES];
    float mptch;

    float repeat;
    float duck;

    uint8_t randCur;
    uint8_t randNxt;

    Params(void) : 
      repeat(0.0), 
      duck(0.0),
      mptch(0.0),
      randNxt(0),
      randCur(0)
    { 
      uint8_t i = MAX_VOICES;
      while(i != 0) {
        i--;
        ptrnIdx[i] = 0;
        ptch[i] = 0;
      }
    }
  };

  struct Voice {
    Sample *pSample;
     
    const uint8_t *pCurData;
    const uint8_t *pNxtData;

    int16_t decodebuf[16 + 2];
    uint16_t dly;
    uint16_t retrigCnt;
    float stp;
    float rate;
  
    float decompIdx;
    uint16_t blockIdx;
    float div, divInit;

    bool duck;

    Voice(void)
    {
    }
  };

  struct Sequence {
    uint8_t stepIdx;

    uint16_t cbsCnt;
    uint16_t cbsPrStep;

    uint16_t duckCnt;
    int8_t duckIdx;

    Sequence(void) :
        stepIdx(0), 
        cbsCnt(0),
        cbsPrStep(128),
        duckCnt(0),
        duckIdx(-1)
      {
      }
  };

  struct State {
    bool trig;
    State(void) : 
      trig(false)
    {
    }
  };

  Betty(void) {
    init();
  }

  void init(void) {
    state = State();
    params = Params();

    uint8_t i = MAX_VOICES;
    while(i != 0) {
      i--;
      voices[i] = Voice();
      
      voices[i].pCurData = &dataSilence[0];
      voices[i].decompIdx = (16 + 2);
      voices[i].blockIdx = -9;
      voices[i].decodebuf[16] = 0;
      voices[i].decodebuf[17] = 0;
      voices[i].dly = 0;
      voices[i].stp = 0.1;
      voices[i].duck = false; 
    }
    sequence = Sequence();
  }

  Sequence sequence;
  State state;
  Params params;
  Voice voices[MAX_VOICES];

};
