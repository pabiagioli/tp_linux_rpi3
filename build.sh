#!/bin/sh

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH::jansson-lib/lib/pkgconfig/
echo "--------BEGIN COMPILE SOURCES--------"
aarch64-rpi3-gcc -v -g led_operator.c  -I/home/pampa/dev-folder/linux-embedded/facu/tpfinal/jansson/arm64-lib/include -I/home/pampa/dev-folder/linux-embedded/facu/tpfinal/jansson/arm64-lib/lib `pkg-config --cflags --libs jansson`
echo "--------END COMPILE SOURCES--------"

echo "--------BEGIN COPY BINARY TO RASPBERRY PI 3--------"
cp -v a.out ~/linux-embedded-labs/nfs-root/led_ops
echo "--------END COPY BINARY TO RASPBERRY PI 3--------"