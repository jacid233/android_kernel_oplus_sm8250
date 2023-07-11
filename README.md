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

* oss-q: stock_defconfig

## 其余分支是干嘛的

* arrow-13.0: 从realme gt neo2 kang过来的类原生使用的

* rebase-S: 从realme gt neo2 找来的安卓12源码，早期的类原生测试

* topaz: 从realme gt neo2找来的给AOSPA使用的内核源码(安卓13)

<hr/>

## FAQ

* 问：请问作者是怎么整理的，说一下大概思路？
* 答：从[oppo-source](https://github.com/oppo-source)拉取`内核源码`和`kernel module`，然后就开始编译，从ColorOS12.1开始发布的内核源码很完善了(仅限于sm8250)，编译出来的能开机。由于高通部分东西如audio, qcacld, data-kernel需要自己另外合并，才能使声音、wifi正常工作(至于怎么合并，只要是tag相同都是通用的)。

<br/>

* 问：为何要用 `aosp-clang-r383902` + `gcc-4.9` 这么老的编译器？
* 答：懒得上带bintuils的llvm，还是走官方大概使用的版本(在865发布的时候，好像AOSP编译内核用的是r383902)

<br/>

* 问：作者编译使用的Linux发行版？
* 答：Ubuntu20.04

<br/>

* 问：编译所需依赖？
* 答：去LineageOS抄一份安卓编译依赖就行，只不过臃肿了点。

<br/>

* 问：dtb,dtbo这些不编译，刷内核不会炸fastboot吧？
* 答：官方系统就没必要担忧这个，至少我给ColorOS刷内核，dtb,dtbo这些保留官方也能开机正常使用，如果在类原生继续保持dtb,dtbo预编译还真会炸fastboot。

<br/>

* 问：在激进的Linux发行版无法编译 `oss-t` , `oss-r` , `oss-q` 怎么办？
* 答：你给它上 `llvm+binutils` 吧，其实那个 `oss-s` 是做过相关处理了(其实不是我搞的，只是我当初 kang [youngguo18](https://github.com/youngguo18) 的oppo-kernel附带的，编译的时候，参照源码里面的build_llvm.sh)

<br/>

* 问：作者以后会在源码集成kernelsu吗？
* 答：不可能！

<br/>

* 问：以后会对编译出来的内核二进制文件进行收费吗？
* 答：都公开了还有收费的必要吗？

<br/>

* 问：`oss-t` 源码适用于 `FindX2` 吗？
* 答：理论上和 `Ace2` 一样可以用。

<br/>

* 问：`oss-t` , `oss-r` 的内核源码怎么看起来和 `realme`, `oneplus` 的那么像？
* 答：我看过 `oppo` `realme` `oneplus` 三者的 `sm8250` 安卓13的内核源码，基线是一致的，仅设备驱动不一样。如果编译有啥缺失的都可以在这三个寻找一下。

<br/>

* 问：`ColorOS13` 的内核源码和 `ColorOS12.1` 的，有何区别？
* 答：没区别，你这么理解吧，就像MIUI刷版本号，由于这个设备的安卓13是通过 `vendor freeze` 啥的特性快速适配来的，所以这可以解释为何在 `ColorOS12.1` 与 `ColorOS13` 之间互刷 `boot.img` 都能正常开机而不是直接重启到 `fastboot`。

<br/>

* 问：此内核与官方内核有什么不同的吗？
* 答：除了根据开源的东西编译出来之外，没了。也没做过性能优化！

<br/>

* 问：是否可以提供一下TWRP，毕竟Ace2的过于冷门，网上的都乱了？
* 答：可以，不过这些TWRP都不是我适配的，毕竟我也不会这方面的玩意，往下看一下也许能看到分享链接(看到的请尽快保存，随时会被清理)。

<hr/>

## TWRP收集

* ColorOS7.1 (wzsx150): https://wwfg.lanzoum.com/i8rcF09kvzta
* ColorOS11 (残芯): https://wwfg.lanzoum.com/i9mPA0ztcggj

<br/>

> 注意：ColorOS12.1和ColorOS13的并没有Ace2的twrp，因为findx2的twrp能兼容Ace2，所以就去隔壁Findx2找了一份过来。

* ColorOS12.1(pomelohan):https://wwfg.lanzoum.com/iqLv10tzdqze
* ColorOS13.0/1(pomelohan):https://wwfg.lanzoum.com/iW1Xs0tzdeda


<hr/>

## Credits

* [EndCredits](https://github.com/EndCredits)

* [Mashopy](https://github.com/Mashopy) 

* [oppo-source](https://github.com/oppo-source) 

* [pomelohan](https://github.com/pomelohan)

* [TingyiChen](https://github.com/TingyiChen)

* [youngguo18](https://github.com/youngguo18)

* ......
