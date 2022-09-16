#!/bin/bash

#
# Clone proton-clang toolchain if needed
#
#if [ ! -d ./toolchain/ ];
#then
#    git clone --depth=1 git@github.com:kdrag0n/proton-clang.git ./toolchain/
#fi

KERNEL_DEFCONFIG=vendor/op4ad9_defconfig
DIR=$PWD
export ARCH=arm64
export SUBARCH=arm64
export CLANG_PATH=/home/huaji/upload/toolchain/bin
export PATH="$CLANG_PATH:$PATH"
export CROSS_COMPILE=aarch64-linux-gnu-
export CROSS_COMPILE_ARM32=arm-linux-gnueabi-
export KBUILD_BUILD_USER=beluga
export KBUILD_BUILD_HOST=hecker

echo
echo "Kernel is going to be built using $KERNEL_DEFCONFIG"
echo

#make clean && make mrproper
A=$(date +%s)
make CC=clang AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip O=out $KERNEL_DEFCONFIG

make CC=clang AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip O=out -j$(nproc --all) | tee build.txt
#find out/arch/arm64/boot/dts/vendor/19161 -name '*.dtb' -exec cat {} + > out/arch/arm64/boot/dtb
B=$(date +%s)
C=$(expr $B - $A)
#echo $(expr $C / 60)
echo "kernel finish" $(expr $C / 60)"min"$(expr $C % 60)"s"
