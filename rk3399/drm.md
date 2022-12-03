# Rockchip DRM

[rockchip drm](https://markyzq.gitbooks.io/rockchip_drm_integration_helper/content/zh/)

## modetest

modetest是libdrm源码自带的调试工具, 可以对drm进行一些基础的调试

android平:

	mmm external/libdrm/tests

linux平台(在开发主机上编译)

	git clone git://anongit.freedesktop.org/mesa/drm && cd drm
	CC=aarch64-linux-gnu-gcc ./autogen.sh --host=aarch64-linux --disable-freedreno --disable-cairo-tests --enable-install-test-programs
	make -j8 && make install DESTDIR=`pwd`/out
	adb push out/usr/local /usr/local
