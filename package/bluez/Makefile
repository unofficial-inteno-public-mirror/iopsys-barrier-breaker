#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=bluez
PKG_VERSION:=5.28
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=http://www.kernel.org/pub/linux/bluetooth
PKG_MD5SUM:=bc20a8285530758c68f6a60e4ca62a15

PKG_FIXUP:=autoreconf
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/bluez
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libpthread +libusb-compat +glib2 +dbus +udev $(INTL_DEPENDS) $(ICONV_DEPENDS)
  TITLE:=Bluetooth stack new version 
  URL:=http://www.bluez.org/
endef

define Package/bluez/conffiles
/etc/config/bluetooth
endef

TARGET_LDFLAGS+= \
                -Wl,-rpath-link=$(STAGING_DIR)/usr/lib/libiconv-full/lib \

CONFIGURE_ARGS += \
		--enable-shared \
		--disable-cups \
		--enable-debug \
		--enable-test \
		--disable-obex \
		--disable-systemd \
		--disable-udev \
		--enable-tools \
		--enable-experimental \
		--enable-library \
		--disable-static \

define Build/Compile
    $(MAKE) -C $(PKG_BUILD_DIR) \
        LDFLAGS="$(TARGET_LDFLAGS) \
            -L$(ICONV_PREFIX)/lib \
            -L$(INTL_PREFIX)/lib -lm -lncurses" \
        DESTDIR="$(PKG_INSTALL_DIR)" \
        all install
endef

define Package/bluez/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/hciconfig $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/hcitool $(1)/usr/bin/
endef

$(eval $(call BuildPackage,bluez))
