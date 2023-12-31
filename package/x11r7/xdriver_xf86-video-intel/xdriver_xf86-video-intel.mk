################################################################################
#
# xdriver_xf86-video-intel
#
################################################################################

XDRIVER_XF86_VIDEO_INTEL_VERSION = 5ca3ac1a90af177eb111a965e9b4dd8a27cc58fc
XDRIVER_XF86_VIDEO_INTEL_SITE = git://anongit.freedesktop.org/xorg/driver/xf86-video-intel
XDRIVER_XF86_VIDEO_INTEL_LICENSE = MIT
XDRIVER_XF86_VIDEO_INTEL_LICENSE_FILES = COPYING
XDRIVER_XF86_VIDEO_INTEL_AUTORECONF = YES

# -D_GNU_SOURCE fixes a getline-related compile error in src/sna/kgem.c
# We force -O2 regardless of the optimization level chosen by the user,
# as compiling this package is known to be broken with -Os.
# Disable full RELRO because X can't hang with the big boys.
XDRIVER_XF86_VIDEO_INTEL_CONF_ENV = \
	CFLAGS="$(TARGET_CFLAGS) -D_GNU_SOURCE -O2 -Wl,-z,lazy"
XDRIVER_XF86_VIDEO_INTEL_CONF_ENV += LDFLAGS="$(TARGET_LDFLAGS) -z lazy"

XDRIVER_XF86_VIDEO_INTEL_CONF_OPTS = \
	--disable-xvmc \
	--enable-sna \
	--disable-xaa \
	--disable-dga \
	--disable-async-swap

XDRIVER_XF86_VIDEO_INTEL_DEPENDENCIES = \
	libdrm \
	libpciaccess \
	xlib_libXrandr \
	xorgproto \
	xserver_xorg-server

# X.org server support for DRI depends on a Mesa3D DRI driver
ifeq ($(BR2_PACKAGE_MESA3D_DRI_DRIVER),y)
XDRIVER_XF86_VIDEO_INTEL_CONF_OPTS += \
	--enable-dri2 \
	--enable-dri3 \
	--enable-uxa
endif

ifeq ($(BR2_PACKAGE_XF86_VIDEO_INTEL_DEBUG),y)
XDRIVER_XF86_VIDEO_INTEL_CONF_OPTS += \
	--enable-debug=full
endif

$(eval $(autotools-package))
