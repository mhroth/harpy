#!/bin/bash

# -DNDEBUG disables all assert checking
clang main.c tinyosc/*.c heavy/mixer/*.c -D_GNU_SOURCE -std=c11 -Werror -Ofast -lm -lrt -lasound -lpthread -o rpistorius
