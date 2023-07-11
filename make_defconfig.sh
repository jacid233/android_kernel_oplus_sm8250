BUILD_CROSS_COMPILE=/media/huaji/fc1e4abe-0fb6-45f0-a812-4410e3006a32/ArrowOS13.0/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
CLANG_PATH=/media/huaji/fc1e4abe-0fb6-45f0-a812-4410e3006a32/ArrowOS13.0/prebuilts/clang/host/linux-x86/clang-r383902/bin
CROSS_COMPILE_ARM32=/media/huaji/fc1e4abe-0fb6-45f0-a812-4410e3006a32/ArrowOS13.0/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-

CLANG_TRIPLE=aarch64-linux-gnu-

export ARCH=arm64
export PATH=${CLANG_PATH}:${PATH}

make -j8 -C $(pwd) O=$(pwd)/out CROSS_COMPILE=$BUILD_CROSS_COMPILE CLANG_TRIPLE=$CLANG_TRIPLE CROSS_COMPILE_ARM32=$CROSS_COMPILE_ARM32 \
    CC=clang \
    stock_defconfig
    
make -j8 -C $(pwd) O=$(pwd)/out CROSS_COMPILE=$BUILD_CROSS_COMPILE CLANG_TRIPLE=$CLANG_TRIPLE CROSS_COMPILE_ARM32=$CROSS_COMPILE_ARM32 \
    CC=clang \
    menuconfig
