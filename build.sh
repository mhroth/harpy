#!/bin/bash

# -DNDEBUG disables all assert checking
gcc main.c tinyosc/*.c -std=c11 -Werror -O3 -lm -lrt -o rpistorius
