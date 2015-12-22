#!/bin/bash

clang main.c oscbuffer.c tinyosc/*.c \
./heavy/static/*.c ./heavy/slot0/*.c \
./heavy/rpis_osc/*.c \
-I./heavy/static \
-std=c11 \
-D_GNU_SOURCE -DNDEBUG -DPRINT_PERF=0 \
-Werror -Wno-#warnings \
-Ofast -ffast-math \
-mcpu=cortex-a7 -mfloat-abi=hard \
-mfpu=neon -march=armv7-a -mtune=cortex-a7 \
-lm -lrt -lasound -lpthread -o harpy
