#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bcmhotproxy
PKG_VERSION:=1.0.0
PKG_RELEASE:=13

PKG_SOURCE_VERSION:=63a265031b870a0aa6b45352cf28224518a31638
PKG_SOURCE_PROTO:=git
ifeq ($(CONFIG_BCM_OPEN),y)
PKG_SOURCE_URL:=http://public.inteno.se/bcmhotproxy
else
PKG_SOURCE_URL:=git@public.inteno.se:bcmhotproxy
endif

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_RELEASE)-$(PKG_SOURCE_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

STAMP_PREPARED:=$(STAMP_PREPARED)_$(call confvar,CONFIG_MTD_REDBOOT_PARTS)

include $(INCLUDE_DIR)/package.mk

define Package/bcmhotproxy
	SECTION:=utils
	CATEGORY:=Base system
	TITLE:=Daemon That feeds broadcom driver calls to hotplug2
endef

define Package/bcmhotproxy/description
 This package contains a Daemon that will listen to link events from broadcoms adsl driver and etherent driver and proxy them to hotplug
endef

target=$(firstword $(subst -, ,$(BOARD)))

MAKE_FLAGS += TARGET="$(target)"
TARGET_CFLAGS += -Dtarget_$(target)=1 -Wall

define Package/bcmhotproxy/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/etc/
	$(INSTALL_DIR) $(1)/etc/init.d/

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bcmhotproxy $(1)/sbin/
endef

$(eval $(call BuildPackage,bcmhotproxy))
