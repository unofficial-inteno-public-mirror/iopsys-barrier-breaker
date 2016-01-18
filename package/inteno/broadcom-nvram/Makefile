#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=broadcom-nvram
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE_VERSION:=8b6018d9ce5e292f0a4a3e86f8fa7d8bc003c3fb
PKG_SOURCE_PROTO:=git
ifeq ($(CONFIG_BCM_OPEN),y)
PKG_SOURCE_URL:=http://public.inteno.se/broadcom-nvram
else
PKG_SOURCE_URL:=git@public.inteno.se:broadcom-nvram
endif

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_RELEASE)-$(PKG_SOURCE_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

STAMP_PREPARED:=$(STAMP_PREPARED)_$(call confvar,CONFIG_MTD_REDBOOT_PARTS)

include $(INCLUDE_DIR)/package.mk

LDFLAGS+= \
	-Wl,-rpath-link=$(STAGING_DIR)/usr/lib \
	-Wl,-rpath-link=$(STAGING_DIR)/lib

RSTRIP:=true

define Package/bcmnvram
	CATEGORY:=Libraries
	TITLE:=Broadcom nvram emulator library
	URL:=
	DEPENDS:=PACKAGE_libuci:libuci
endef

define Package/bcmnvram/description
	Broadcom nvram to uci wrapper
endef

target=$(firstword $(subst -, ,$(BOARD)))

MAKE_FLAGS += TARGET="$(target)"
TARGET_CFLAGS += -Dtarget_$(target)=1 -Wall

define Package/bcmnvram/install
	$(CP) ./files/* $(1)/
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
	cp $(PKG_BUILD_DIR)/libnvram.so $(1)/usr/lib/
	cp $(PKG_BUILD_DIR)/uci_test $(1)/usr/bin/
endef

$(eval $(call BuildPackage,bcmnvram))
