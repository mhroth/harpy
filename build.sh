#!/bin/bash

clang main.c tinyosc/*.c \
./heavy/static/*.c ./heavy/rpis_osc/*.c \
-I./heavy/static \
-std=c11 \
-D_GNU_SOURCE -DNDEBUG -DPRINT_PERF=0 \
-Werror -Wno-#warnings \
-Ofast -ffast-math \
-mcpu=cortex-a7 -mfloat-abi=hard \
-lm -lrt -lasound -lpthread -o harpy
