#!/bin/sh

OPENWRT_DIR=/home/prog/openwrt/lede-all/2019-openwrt-all/openwrt-ipq4xxx
export STAGING_DIR=${OPENWRT_DIR}/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-7.4.0_musl_eabi

CC=gcc
#CC=$STAGING_DIR/bin/arm-openwrt-linux-gcc

#CFLAGS="-g -Os -fno-common -ffixed-r8"
#CFLAGS2="-fno-builtin -ffreestanding"
#CFLAGS3="-pipe -DCONFIG_ARM -D__ARM__ -fPIC -marm -mno-thumb-interwork -mabi=aapcs-linux -march=armv7-a -mno-unaligned-access"
#CFLAGS4="-Wall -Wstrict-prototypes -fno-stack-protector -Wno-format-nonliteral -Wno-format-security -fstack-usage"

$CC ./LzmaDecode.c -c -O2 $CFLAGS $CFLAGS2 $CFLAGS3 $CFLAGS4
$CC ./lzma.c -c
$CC ./test.c -o ./test ./LzmaDecode.o ./lzma.o $CFLAGS $CFLAGS2 $CFLAGS3 $CFLAGS4
