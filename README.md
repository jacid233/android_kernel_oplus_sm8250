# OPPO Ace2的安卓内核源码

## 分支说明

- oss-t: 官方开源的ColorOS13内核源码整理，仅适用于编译给ColorOS12.1-ColorOS13.1使用

- oss-s: 官方开源的ColorOS12.1内核源码整理，主要给类原生使用，在官方系统上(ColorOS12.1-ColorOS13.1)无法触发oddo私有快充

- oss-r: 官方开源的Findx2内核源码整理，与Ace2通用，但是仅适用于ColorOS11 C11及以前的版本

- oss-q: 官方开源的ColorOS7.1内核源码整理，适用于ColorOS7.1

## 如何编译

* 下载 `aarch64-linux-android-4.9` , 可以去[LineageOS的仓库](https://github.com/LineageOS/android_prebuilts_gcc_linux-x86_aarch64_aarch64-linux-android-4.9)下载

* 下载 `clang-r383902b1`, 在github搜以下就行

* 拉取内核源码，在源码根目录下创建一个编译脚本(这里就叫build.sh), 写入以下内容：

```bash
mkdir out

BUILD_CROSS_COMPILE="aarch64-linux-android-4.9编译器文件夹位置"/bin/aarch64-linux-android-

CLANG_PATH="clang-r383902b1的文件夹位置"/bin
CROSS_COMPILE_ARM32="arm-linux-androideabi-4.9的文件夹位置"/bin/arm-linux-androideabi-

CLANG_TRIPLE=aarch64-linux-gnu-

export ARCH=arm64
export PATH=${CLANG_PATH}:${PATH}

make -j8 -C $(pwd) O=$(pwd)/out CROSS_COMPILE=$BUILD_CROSS_COMPILE CLANG_TRIPLE=$CLANG_TRIPLE CROSS_COMPILE_ARM32=$CROSS_COMPILE_ARM32 \
    CC=clang \
    vendor/op4ad9_defconfig     //编译配置文件，自己去arch/arm64/configs目录下寻找

make -j8 -C $(pwd) O=$(pwd)/out CROSS_COMPILE=$BUILD_CROSS_COMPILE CLANG_TRIPLE=$CLANG_TRIPLE CROSS_COMPILE_ARM32=$CROSS_COMPILE_ARM32 \
    CC=clang \
    -Werror \
    2>&1 | tee build.txt
```

* 最终，编译的文件在 `out/arch/arm64/boot` 下的Image就是编译出来的内核

## 各分支的编译配置文件对应:

> defconfig文件在 `arch/arm64/config` 目录下，然而里面有一堆厂商的配置文件不知道用谁，在此目录找出以下的编译配置文件拿来编译就行

* oss-t: stock_defconfig

* oss-s: vendor/op4ad9_defconfig

* oss-r: vendor/op4ad9-perf_defconfig

* oss-q: vendor/op4ad9-perf_defconfig

## 其余分支是干嘛的

* arrow-13.0: 从realme gt neo2 kang过来的类原生使用的

* rebase-S: 从realme gt neo2 找来的安卓12源码，早期的类原生测试

* topaz: 从realme gt neo2找来的给AOSPA使用的内核源码(安卓13)

## Credits

* [oppo-source](https://github.com/oppo-source) 

* [Mashopy](https://github.com/Mashopy) 

* [youngguo18](https://github.com/youngguo18)

* [EndCredits](https://github.com/EndCredits)

* ......