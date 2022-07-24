#!/bin/bash

#
# Clone proton-clang toolchain if needed
#
if [ ! -d ./toolchain/ ];
then
    git clone --depth=1 git@github.com:kdrag0n/proton-clang.git ./toolchain/
fi

KERNEL_DEFCONFIG=vendor/sm8250_defconfig
DIR=$PWD
export ARCH=arm64
export SUBARCH=arm64
export CLANG_PATH=$DIR/toolchain/bin
export PATH="$CLANG_PATH:$PATH"
export CROSS_COMPILE=aarch64-linux-gnu-
export CROSS_COMPILE_ARM32=arm-linux-gnueabi-
export KBUILD_BUILD_USER=beluga
export KBUILD_BUILD_HOST=hecker

echo
echo "Kernel is going to be built using $KERNEL_DEFCONFIG"
echo

make clean && make mrproper

make CC=clang AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip O=out $KERNEL_DEFCONFIG

make CC=clang AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip O=out -j$(nproc --all)
