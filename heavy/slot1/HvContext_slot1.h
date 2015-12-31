
/**
 * Copyright (c) 2014,2015 Enzien Audio, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, and/or
 * sublicense copies of the Software, strictly on a non-commercial basis,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * DO NOT MODIFY. THIS CODE IS MACHINE GENERATED BY THE ENZIEN AUDIO HEAVY COMPILER.
 */

#ifndef _HEAVYCONTEXT_SLOT1_H_
#define _HEAVYCONTEXT_SLOT1_H_

#include "HvBase.h"

#define Context(_x) ((Hv_slot1 *) (_x))

// object includes
#include "SignalVar.h"
#include "ControlCast.h"
#include "ControlSlice.h"
#include "SignalLine.h"
#include "HeavyMath.h"
#include "ControlBinop.h"
#include "ControlVar.h"
#include "ControlDelay.h"
#include "SignalPhasor.h"
#include "ControlPack.h"

typedef struct Hv_slot1 {
  HvBase base;

  // objects
  SignalPhasor sPhasor_Gs1GI;
  SignalLine sLine_vHi7m;
  SignalVarf sVarf_9eDxl;
  SignalPhasor sPhasor_6kFDB;
  SignalVarf sVarf_bKNZg;
  SignalVarf sVarf_5QeE6;
  SignalPhasor sPhasor_fbdId;
  SignalVarf sVarf_O33ga;
  ControlBinop cBinop_PXK5I;
  ControlVar cVar_bpe3k;
  ControlVar cVar_GUnko;
  ControlVar cVar_6QKM5;
  ControlVar cVar_blf0i;
  ControlDelay cDelay_MbsR2;
  ControlPack cPack_BExUd;
  ControlBinop cBinop_YKLJB;
  ControlBinop cBinop_b5YnN;
  ControlBinop cBinop_ALjJc;
  ControlBinop cBinop_cT4rk;
  ControlBinop cBinop_AeJlV;
  ControlSlice cSlice_VAx35;
  ControlSlice cSlice_rByiO;
  ControlSlice cSlice_Cy6Sr;
  ControlSlice cSlice_5lwBN;
  ControlSlice cSlice_h12g4;
} Hv_slot1;

#endif // _HEAVYCONTEXT_SLOT1_H_