#!/bin/bash

# -DNDEBUG disables all assert checking
clang main.c tinyosc/*.c heavy/mixer/*.c \
-std=c11 \
-D_GNU_SOURCE \
-Werror -Wno-#warnings \
-Ofast -ffast-math \
-mcpu=cortex-a7 -mfloat-abi=hard \
-lm -lrt -lasound -lpthread -o rpistorius
