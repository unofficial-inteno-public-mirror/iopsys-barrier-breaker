#
# Copyright (C) 2015 Inteno
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=wifimngr
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE_VERSION:=c8d30103f49af2db81691385dc44316a2207ada1
PKG_SOURCE_PROTO:=git
ifeq ($(CONFIG_BCM_OPEN),y)
PKG_SOURCE_URL:=http://public.inteno.se/wifimngr
else
PKG_SOURCE_URL:=git@public.inteno.se:wifimngr
endif

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_RELEASE)-$(PKG_SOURCE_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

LDFLAGS+= \
	-Wl,-rpath-link=$(STAGING_DIR)/usr/lib \
	-Wl,-rpath-link=$(STAGING_DIR)/lib

define Package/wifimngr
	CATEGORY:=Network
	DEPENDS:=+libpthread +libstdcpp +librt +libjson alljoyn +ubus +libuci
	TITLE:=WiFi Manager
endef

define Package/wifimngr/description
	wifimngr manages wireless driver
endef

define Package/wifimngr/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wifimngr $(1)/sbin/
endef

define Package/wifimngr/postinst
	/etc/init.d/alljoyn restart
	pidof alljoyn-daemon >/dev/null && wifimngr &
endef

$(eval $(call BuildPackage,wifimngr))
