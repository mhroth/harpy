/* Copyright (c) 2015, Martin Roth (mhroth@gmail.com). All Rights Reserved. */

#ifndef _HARPY_OSC_BUFFER_
#define _HARPY_OSC_BUFFER_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *buffer;
  char *iterator;
  char *marker;
  uint32_t len; // length of buffer in bytes
} OscBuffer;

void oscbuffer_init(OscBuffer *o, uint32_t numBytes);

char *oscbuffer_getBuffer(OscBuffer *o);

void oscbuffer_free(OscBuffer *o);

void oscbuffer_resetIterator(OscBuffer *o);

char *oscbuffer_getNextBuffer(OscBuffer *o, uint32_t *len);

bool oscbuffer_addBuffer(OscBuffer *o, const char *buffer, uint32_t len);

void oscbuffer_clear(OscBuffer *o);

#endif // _HARPY_OSC_BUFFER_
