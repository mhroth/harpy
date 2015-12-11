/**
 * Copyright (c) 2014, 2015, Enzien Audio Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "Heavy.h"

#if !HV_WIN
#pragma mark - Heavy Table
#endif

int hv_table_resize(HvTable *o, hv_uint32_t newLength) {
  return hTable_resize(o, newLength);
}

float *hv_table_getBuffer(HvTable *o) {
  return hTable_getBuffer(o);
}

hv_size_t hv_table_getLength(HvTable *o) {
  return hTable_getLength(o);
}



#if !HV_WIN
#pragma mark - Heavy Message
#endif

hv_size_t hv_msg_getByteSize(hv_uint32_t numElements) {
  return msg_getByteSize(numElements);
}

void hv_msg_init(HvMessage *m, int numElements, hv_uint32_t timestamp) {
  msg_init(m, numElements, timestamp);
}

hv_size_t hv_msg_getNumElements(const HvMessage *const m) {
  return msg_getNumElements(m);
}

double hv_msg_getTimestamp(const HvMessage *const m) {
  return msg_getTimestamp(m);
}

void hv_msg_setTimestamp(HvMessage *m, hv_uint32_t timestamp) {
  msg_setTimestamp(m, timestamp);
}

bool hv_msg_isBang(const HvMessage *const m, int i) {
  return msg_isBang(m,i);
}

void hv_msg_setBang(HvMessage *m, int i) {
  msg_setBang(m,i);
}

bool hv_msg_isFloat(const HvMessage *const m, int i) {
  return msg_isFloat(m, i);
}

float hv_msg_getFloat(const HvMessage *const m, int i) {
  return msg_getFloat(m,i);
}

void hv_msg_setFloat(HvMessage *m, int i, float f) {
  msg_setFloat(m,i,f);
}

bool hv_msg_isSymbol(const HvMessage *const m, int i) {
  return msg_isSymbol(m,i);
}

char *hv_msg_getSymbol(const HvMessage *const m, int i) {
  return msg_getSymbol(m,i);
}

void hv_msg_setSymbol(HvMessage *m, int i, char *s) {
  msg_setSymbol(m,i,s);
}

bool hv_msg_isHash(const HvMessage *const m, int i) {
  return msg_isHash(m, i);
}

unsigned int hv_msg_getHash(const HvMessage *const m, int i) {
  return msg_getHash(m, i);
}

bool hv_msg_hasFormat(const HvMessage *const m, const char *fmt) {
  return msg_hasFormat(m, fmt);
}

char *hv_msg_toString(const HvMessage *const m) {
  return msg_toString(m);
}

HvMessage *hv_msg_copy(HvMessage *m) {
  return msg_copy(m);
}

void hv_msg_free(HvMessage *m) {
  msg_free(m);
}



#if !HV_WIN
#pragma mark - Heavy Common
#endif

double hv_getSampleRate(Heavy *c) {
  return ctx_getSampleRate(Base(c));
}

int hv_getNumInputChannels(Heavy *c) {
  return ctx_getNumInputChannels(Base(c));
}

int hv_getNumOutputChannels(Heavy *c) {
  return ctx_getNumOutputChannels(Base(c));
}

const char *hv_getName(Heavy *c) {
  return ctx_getName(Base(c));
}

void hv_setPrintHook(Heavy *c,
    void (*f)(double, const char *, const char *, void *)) {
  ctx_setPrintHook(Base(c), f);
}

void hv_setSendHook(Heavy *c,
    void (*f)(double, const char *, const HvMessage *const, void *)) {
  ctx_setSendHook(Base(c), f);
}

void hv_sendBangToReceiver(Heavy *c, const char *receiverName) {
  HvMessage *m = HV_MESSAGE_ON_STACK(1);
  msg_initWithBang(m, Base(c)->blockStartTimestamp);
  ctx_scheduleMessageForReceiver(Base(c), receiverName, m);
}

void hv_sendFloatToReceiver(Heavy *c, const char *receiverName, const float x) {
  HvMessage *m = HV_MESSAGE_ON_STACK(1);
  msg_initWithFloat(m, Base(c)->blockStartTimestamp, x);
  ctx_scheduleMessageForReceiver(Base(c), receiverName, m);
}

void hv_sendSymbolToReceiver(Heavy *c, const char *receiverName, char *s) {
  HvMessage *m = HV_MESSAGE_ON_STACK(1);
  msg_initWithSymbol(m, Base(c)->blockStartTimestamp, s);
  ctx_scheduleMessageForReceiver(Base(c), receiverName, m);
}

void hv_vscheduleMessageForReceiver(Heavy *c, const char *receiverName,
    const double delayMs, const char *format, ...) {
  va_list ap;
  va_start(ap, format);

  const int numElem = (int) hv_strlen(format);
  HvMessage *m = HV_MESSAGE_ON_STACK(numElem);
  msg_init(m, numElem, Base(c)->blockStartTimestamp +
      (hv_uint32_t) (delayMs*ctx_getSampleRate(Base(c))/1000.0));
  for (int i = 0; i < numElem; i++) {
    switch (format[i]) {
      case 'b': msg_setBang(m,i); break;
      case 'f': msg_setFloat(m, i, (float) va_arg(ap, double)); break;
      case 's': msg_setSymbol(m, i, (char *) va_arg(ap, char *)); break;
      default: break;
    }
  }
  ctx_scheduleMessageForReceiver(Base(c), receiverName, m);

  va_end(ap);
}

void hv_scheduleMessageForReceiver(Heavy *c, const char *receiverName,
    double delayMs, HvMessage *m) {
  hv_assert(delayMs >= 0.0);
  msg_setTimestamp(m, Base(c)->blockStartTimestamp +
      (hv_uint32_t) (delayMs*ctx_getSampleRate(Base(c))/1000.0));
  ctx_scheduleMessageForReceiver(Base(c), receiverName, m);
}

HvTable *hv_getTableForName(Heavy *c, const char *tableName) {
  return ctx_getTableForName(Base(c), tableName);
}

void hv_cancelMessage(Heavy *c, HvMessage *m) {
  ctx_cancelMessage(Base(c), m, NULL);
}

double hv_getCurrentTime(Heavy *c) {
  return ((double) Base(c)->blockStartTimestamp)/Base(c)->sampleRate;
}

void *hv_getUserData(Heavy *c) {
  return ctx_getUserData(Base(c));
}

void hv_setUserData(Heavy *c, void *userData) {
  ctx_setUserData(Base(c), userData);
}

void hv_setBasePath(Heavy *c, const char *basePath) {
  ctx_setBasePath(Base(c), basePath);
}
