#!/bin/bash
# 源码版本4.1.2 下载地址： https://ffmpeg.org/download.html
# 选对ndk版本很重要 https://developer.android.google.cn/ndk/downloads/older_releases.html#ndk-17c-downloads
# NDK r17c版本将头文件和库文件进行了分离，我们指定的sysroot文件夹下只有库文件，而头文件放在了NDK目录下的sysroot内.
# 需在-–extra-cflags中添加 “-isysroot $NDK/sysroot”
# 需在--extra-cflags中添加 -I$NDK/sysroot/usr/include/arm-linux-androideabi；arm-linux-androideabi是根据编译目标平台选择
# 注意系统so库，最低兼容
export NDK=/Users/wangrenxing/ffmpeg/android-ndk-r17c
# 此目录下放着系统C/C++ so 库，注意向下兼容性
export SYSROOT=$NDK/platforms/android-16/arch-x86/
# 交叉编译工具链的目录
export TOOLCHAIN=$NDK/toolchains/x86-4.9/prebuilt/darwin-x86_64
# cpu构架
export CPU=x86
# 导出安装目录前缀，具体可见./configure --help 列出的Standard options:
export PREFIX=$(pwd)/android/$CPU
# c编译参数 arm指令集
export ADDI_CFLAGS="-I$NDK/sysroot/usr/include/i686-linux-android -isysroot $NDK/sysroot "
# 配置 具体含义可见./configure --help
./configure \
--prefix=$PREFIX \
--enable-shared \
--disable-static \
--enable-gpl \
--disable-ffmpeg \
--disable-ffplay \
--disable-ffprobe \
--disable-doc \
--disable-yasm \
--disable-asm \
--disable-symver \
--disable-stripping \
--enable-cross-compile \
--target-os=linux \
--cross-prefix=$TOOLCHAIN/bin/i686-linux-android- \
--sysroot=$SYSROOT \
--arch=$CPU \
--extra-cflags="-Os -fpic $ADDI_CFLAGS" \
--extra-ldflags="$ADDI_LDFLAGS" $ADDITIONAL_CONFIGURE_FLAG
# 清除
make clean
# 编译
make -j8
# 安装
make install

# 解决问题 注意如下变量冲突问题，修改一个后直接make即可，无需运行
# 一、libavcodec/aaccoder.c:803:25: error: expected identifier or '(' before numeric constant  int B0 = 0, B1 = 0;
#   方法：修改B0变量名为b0即可
# 二、libavcodec/hevc_mvs.c:207:15: error: 'x0000000' undeclared (first use in this function) 
#   方法：修改B0变量名为b0，xB0改成xb0，yB0改成yb0
# 三、libavcodec/opus_pvq.c:498:9: error: expected identifier or '(' before numeric constant
#   方法：修改B0变量名为b0即可
# 上述问题原因，编译链宏定义#define B0 00000，与工程代码B0冲突 
# 大神解析：http://alientechlab.com/how-to-build-ffmpeg-for-android/

# 四、解决so文件命名问题
# #SLIBNAME_WITH_MAJOR='$(SLIBNAME).$(LIBMAJOR)'
# SLIBNAME_WITH_MAJOR='$(SLIBPREF)$(FULLNAME)-$(LIBMAJOR)$(SLIBSUF)'
# LIB_INSTALL_EXTRA_CMD='$$(RANLIB) "$(LIBDIR)/$(LIBNAME)"'
# #SLIB_INSTALL_NAME='$(SLIBNAME_WITH_VERSION)'
# SLIB_INSTALL_NAME='$(SLIBNAME_WITH_MAJOR)'
# #SLIB_INSTALL_LINKS='$(SLIBNAME_WITH_MAJOR) $(SLIBNAME)'
# SLIB_INSTALL_LINKS='$(SLIBNAME)'
