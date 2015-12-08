include $(TOPDIR)/rules.mk

PKG_NAME:=power-management
PKG_RELEASE:=1
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/power-management
  CATEGORY:=Utilities
  TITLE:=Power management tool
endef

define Package/power-management/description
	Control Broadcom power options
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/power-management/install
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,power-management))
