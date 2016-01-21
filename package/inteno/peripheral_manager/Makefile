#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=peripheral_manager
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE_VERSION:=9c3e0825db6a5e8c4ecd950ebf850a6d3c87a556
PKG_SOURCE_PROTO:=git
ifeq ($(CONFIG_BCM_OPEN),y)
PKG_SOURCE_URL:=http://public.inteno.se/peripheral_manager
else
PKG_SOURCE_URL:=git@public.inteno.se:peripheral_manager
endif

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_RELEASE)-$(PKG_SOURCE_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

# support parallel build
PKG_BUILD_PARALLEL:=1

#re create configure scripts if not present.
PKG_FIXUP:=autoreconf

# run install target when cross compiling. basically, make install DESTDIR=$(PKG_INSTALL_DIR)
# this way we don't need to pick out the resulting files from the build dir.
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/peripheral_manager
	CATEGORY:=Utilities
	TITLE:=Application deamon for handling of peripheral
	URL:=
	DEPENDS:=+libuci +libubus +libblobmsg-json bcmkernel
endef

define Package/peripheral_manager/description
	Application handling peripheral
endef

TARGET_CPPFLAGS := \
	-I$(STAGING_DIR)/usr/include/bcm963xx/shared/opensource/include/bcm963xx \
	-I$(STAGING_DIR)/usr/include/bcm963xx/bcmdrivers/opensource/include/bcm963xx \
	$(TARGET_CPPFLAGS)

define Package/peripheral_manager/install
	$(INSTALL_DIR) $(1)/etc/
	$(INSTALL_DIR) $(1)/etc/init.d/

	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/peripheral_manager $(1)/sbin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/gpio_test $(1)/sbin/

	$(CP)  ./files/etc/init.d/* $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,peripheral_manager))
