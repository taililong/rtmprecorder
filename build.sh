#!/bin/bash
set -eu
[[ -z ${CMAKE:-} ]] && CMAKE=cmake
KERNEL=(`uname -s | tr [A-Z] [a-z]`)
ARCH=(`uname -m | tr [A-Z] [a-z]`)
case $KERNEL in
    darwin)
        OS=macosx
        ;;
    mingw*)
        OS=windows
        KERNEL=windows
        if [[ $TARGET_CPU == "x64" ]]; then
            ARCH=x86_64
        fi
        ;;
    *)
        OS=$KERNEL
        ;;
esac
case $ARCH in
    arm*)
        ARCH=arm
        ;;
    i386|i486|i586|i686)
        ARCH=x86
        ;;
    amd64|x86-64)
        ARCH=x86_64
        ;;
esac
PLATFORM=$OS-$ARCH
echo "Detected platform \"$PLATFORM\""
export ANDROID_CPP="$ANDROID_NDK/sources/cxx-stl/gnu-libstdc++/4.6/"
export ANDROID_BIN="$ANDROID_NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/$KERNEL-$ARCH/bin/arm-linux-androideabi"
export ANDROID_ROOT="$ANDROID_NDK/platforms/android-9/arch-arm/"
INSTALL_PATH=`pwd`
#cd openssl
#CROSS_COMPILE="$ANDROID_BIN-" ./Configure android-armv7 -DANDROID -fPIC -ffunction-sections -funwind-tables -fstack-protector -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=300 no-shared --prefix=$INSTALL_PATH
#ANDROID_DEV="$ANDROID_ROOT/usr" make
#make install
#cd x264
#./configure --prefix=$INSTALL_PATH --enable-static --enable-pic --disable-cli --cross-prefix="$ANDROID_BIN-" --sysroot="$ANDROID_ROOT" --host=arm-linux --extra-cflags="-DANDROID -fPIC -ffunction-sections -funwind-tables -fstack-protector -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=300" --extra-ldflags="-nostdlib -Wl,--fix-cortex-a8 -lgcc -ldl -lz -lm -lc"
#make -j4
#make install
cd ffmpeg
#patch -Np1 < ../pffmpeg.patch
./configure --prefix=.. --enable-shared --enable-swscale --disable-avresample --disable-postproc --disable-avfilter --disable-avdevice  --enable-gpl --enable-version3 --enable-nonfree --enable-runtime-cpudetect --disable-iconv --disable-libxcb --disable-opencl --disable-outdev=sdl --disable-ffmpeg --disable-ffplay --enable-libfaac --disable-ffprobe --disable-parsers --enable-parser=h264 --enable-parser=aac --enable-parser=aac_latm --disable-ffserver  --disable-encoders --disable-decoders --disable-demuxers --disable-muxers  --enable-demuxer=flv  --enable-muxer=flv --enable-libx264 --enable-openssl --enable-encoder=flv --enable-decoder=flv --enable-encoder=libfaac --enable-decoder=aac --enable-encoder=libx264  --enable-dct --enable-dwt --enable-lsp --enable-mdct --enable-rdft --enable-fft --enable-cross-compile --cross-prefix="$ANDROID_BIN-" --ranlib="$ANDROID_BIN-ranlib" --sysroot="$ANDROID_ROOT" --target-os=linux --arch=arm --extra-cflags="-I../include/ -DANDROID -fPIC -ffunction-sections -funwind-tables -fstack-protector -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=300" --extra-ldflags="$ANDROID_ROOT/usr/lib/crtbegin_so.o -L../lib/ -L$ANDROID_CPP/libs/armeabi/ -nostdlib -Wl,--fix-cortex-a8" --extra-libs="-lgnustl_static -lgcc -ldl -lz -lm -lc" --disable-symver --disable-programs
make -j4
make install
