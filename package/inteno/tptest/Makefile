include $(TOPDIR)/rules.mk

PKG_NAME:=tptest
PKG_VERSION:=1.3
PKG_RELEASE:=0

PKG_SOURCE_VERSION:=4dfab45a92328226c8182347df50e86a5d72ca5f
PKG_SOURCE_PROTO:=git
ifeq ($(CONFIG_BCM_OPEN),y)
PKG_SOURCE_URL:=http://public.inteno.se/tptest
else
PKG_SOURCE_URL:=git@public.inteno.se:tptest
endif

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_RELEASE)-$(PKG_SOURCE_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

TARGET_LDFLAGS+= \
	-Wl,-rpath-link=$(STAGING_DIR)/usr/lib \
	-Wl,-rpath-link=$(STAGING_DIR)/lib

TARGET_CFLAGS+= \
	-DUNIX -DLINUX

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \

define Package/tptest
	CATEGORY:=Utilities
	TITLE:=TPTEST speed test utility
endef

define Package/tptest/description
	TPTEST speed test utility
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) LINUX_DIR=$(LINUX_DIR) MAKE_OPTS=$(MAKE_OPTS) LDFLAGS="$(TARGET_LDFLAGS)" CFLAGS="$(TARGET_CFLAGS) -I$(LINUX_DIR)/include -I$(STAGING_DIR)/usr/include"
endef

define Package/tptest/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tptest $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/speedtest.config $(1)/etc/config/speedtest
endef

$(eval $(call BuildPackage,tptest))
