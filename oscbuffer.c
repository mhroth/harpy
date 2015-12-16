/* Copyright (c) 2015, Martin Roth (mhroth@gmail.com). All Rights Reserved. */

#include "oscbuffer.h"

void oscbuffer_init(OscBuffer *o, uint32_t numBytes) {
  o->buffer = (char *) malloc(numBytes);
  o->len = numBytes;
  oscbuffer_clear(o);
}

char *oscbuffer_getBuffer(OscBuffer *o) {
  return o->buffer;
}

void oscbuffer_free(OscBuffer *o) {
  free(o->buffer);
  o->buffer = NULL;
  o->iterator = NULL;
  o->marker = NULL;
  o->len = 0;
}

void oscbuffer_resetIterator(OscBuffer *o) {
  o->iterator = o->buffer;
}

char *oscbuffer_getNextBuffer(OscBuffer *o, uint32_t *len) {
  if (o->iterator - o->buffer < o->len) {
    const uint32_t l = *((uint32_t *) o->iterator);
    *len = l;
    char *const b = o->iterator + 4;
    o->iterator += (4 + l);
    return b;
  } else {
    *len = 0;
    return NULL;
  }
}

bool oscbuffer_addBuffer(OscBuffer *o, const char *buffer, uint32_t len) {
  if (o->marker + len + 4 - o->buffer < o->len) {
    *((uint32_t *) o->marker) = len;
    memcpy(o->marker+4, buffer, len);
    return true;
  } else {
    return false;
  }
}

void oscbuffer_clear(OscBuffer *o) {
  memset(o->buffer, 0, o->len);
  o->iterator = o->buffer;
  o->marker = o->buffer;
}
