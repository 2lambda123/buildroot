################################################################################
#
# cups-filters
#
################################################################################

CUPS_FILTERS_VERSION = 1.28.4
CUPS_FILTERS_SITE = http://openprinting.org/download/cups-filters
CUPS_FILTERS_LICENSE = GPL-2.0, GPL-2.0+, GPL-3.0, GPL-3.0+, LGPL-2, LGPL-2.1+, MIT, BSD-4-Clause
CUPS_FILTERS_LICENSE_FILES = COPYING
CUPS_FILTERS_CPE_ID_VENDOR = linuxfoundation

CUPS_FILTERS_DEPENDENCIES = cups libglib2 lcms2 qpdf fontconfig freetype jpeg

CUPS_FILTERS_CONF_OPTS = \
	--disable-mutool \
	--disable-foomatic \
	--disable-braille \
	--enable-imagefilters \
	--with-cups-config=$(STAGING_DIR)/usr/bin/cups-config \
	--with-sysroot=$(STAGING_DIR) \
	--with-pdftops=pdftops \
	--with-jpeg \
	--with-test-font-path=/dev/null \
	--without-rcdir

# 0001-install-support-old-ln-versions-without-the-r-option.patch adds
# a ln-srf script for older distributions, but GNU patch < 2.7 does
# not handle the git patch permission extensions - So ensure it is
# executable
define CUPS_FILTERS_MAKE_LN_SRF_EXECUTABLE
	chmod +x $(@D)/ln-srf
endef

CUPS_FILTERS_POST_PATCH_HOOKS += CUPS_FILTERS_MAKE_LN_SRF_EXECUTABLE

# After 0002-filter-texttotext.c-link-with-libiconv-if-needed.patch autoreconf
# needs config.rpath and ABOUT-NLS, which are not in v1.25.4 yet. Fake them.
define CUPS_FILTERS_ADD_MISSING_FILE
	touch $(@D)/config.rpath $(@D)/ABOUT-NLS
endef

CUPS_FILTERS_PRE_CONFIGURE_HOOKS = CUPS_FILTERS_ADD_MISSING_FILE

ifeq ($(BR2_PACKAGE_LIBPNG),y)
CUPS_FILTERS_CONF_OPTS += --with-png
CUPS_FILTERS_DEPENDENCIES += libpng
else
CUPS_FILTERS_CONF_OPTS += --without-png
endif

ifeq ($(BR2_PACKAGE_TIFF),y)
CUPS_FILTERS_CONF_OPTS += --with-tiff
CUPS_FILTERS_DEPENDENCIES += tiff
else
CUPS_FILTERS_CONF_OPTS += --without-tiff
endif

ifeq ($(BR2_PACKAGE_DBUS),y)
CUPS_FILTERS_CONF_OPTS += --enable-dbus
CUPS_FILTERS_DEPENDENCIES += dbus
else
CUPS_FILTERS_CONF_OPTS += --disable-dbus
endif

# avahi support requires avahi-client, which needs avahi-daemon and dbus
ifeq ($(BR2_PACKAGE_AVAHI_DAEMON)$(BR2_PACKAGE_DBUS),yy)
CUPS_FILTERS_DEPENDENCIES += avahi
CUPS_FILTERS_CONF_OPTS += --enable-avahi
else
CUPS_FILTERS_CONF_OPTS += --disable-avahi
endif

ifeq ($(BR2_PACKAGE_GHOSTSCRIPT),y)
CUPS_FILTERS_DEPENDENCIES += ghostscript
CUPS_FILTERS_CONF_OPTS += --enable-ghostscript
else
CUPS_FILTERS_CONF_OPTS += --disable-ghostscript
endif

ifeq ($(BR2_PACKAGE_IJS),y)
CUPS_FILTERS_DEPENDENCIES += ijs
CUPS_FILTERS_CONF_OPTS += --enable-ijs
else
CUPS_FILTERS_CONF_OPTS += --disable-ijs
endif

ifeq ($(BR2_PACKAGE_POPPLER),y)
CUPS_FILTERS_DEPENDENCIES += poppler
CUPS_FILTERS_CONF_OPTS += --enable-poppler
else
CUPS_FILTERS_CONF_OPTS += --disable-poppler
endif

define CUPS_FILTERS_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0755 package/cups-filters/S82cups-browsed \
		$(TARGET_DIR)/etc/init.d/S82cups-browsed
endef

define CUPS_FILTERS_INSTALL_INIT_SYSTEMD
	$(INSTALL) -D -m 0755 $(@D)/utils/cups-browsed.service \
		$(TARGET_DIR)/usr/lib/systemd/system/cups-browsed.service
endef

$(eval $(autotools-package))
