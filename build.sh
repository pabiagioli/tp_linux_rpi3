#!/bin/sh

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH::jansson-lib/lib/pkgconfig/
echo "--------BEGIN COMPILE SOURCES--------"
aarch64-rpi3-gcc -v -g led_operator.c  -I./jansson-lib/include -I./jansson-lib/lib `pkg-config --cflags --libs jansson`
echo "--------END COMPILE SOURCES--------"

echo "--------BEGIN COPY BINARY TO RASPBERRY PI 3--------"
cp -v a.out ~/linux-embedded-labs/nfs-root/www/cgi-bin/led_ops
cp -v environment ~/linux-embedded-labs/nfs-root/www/cgi-bin/environment
cp -v index.html ~/linux-embedded-labs/nfs-root/www
echo "--------END COPY BINARY TO RASPBERRY PI 3--------"