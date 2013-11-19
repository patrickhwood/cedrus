GCC = arm-linux-gnueabihf-gcc
TARGET = jpeg-test
LIB = libjpeg.a
LIBSRC = jpeg.c decode.c show.c slideshow.c ../common/disp.c ../common/ve.c
SRC = main.c jpeg.c decode.c show.c slideshow.c ../common/disp.c ../common/ve.c
CFLAGS = -Wall -Wextra -O2
LDFLAGS = 
