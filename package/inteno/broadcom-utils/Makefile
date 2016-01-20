include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#export BUILD_DIR

PKG_NAME:=broadcom-utils
PKG_VERSION:=0
PKG_RELEASE:=1

PKG_SOURCE_VERSION:=2d83316221fe1e3be7d86c1d44d99b00b62f2373
PKG_SOURCE_PROTO:=git
ifeq ($(CONFIG_BCM_OPEN),y)
PKG_SOURCE_URL:=http://public.inteno.se/broadcom-utils
else
PKG_SOURCE_URL:=git@public.inteno.se:broadcom-utils
endif

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_RELEASE)-$(PKG_SOURCE_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

STAMP_PREPARED := $(STAMP_PREPARED)_$(call confvar,CONFIG_MTD_REDBOOT_PARTS)

############################################################################## #

include $(INCLUDE_DIR)/package.mk

define Package/broadcom-utils/Default
	SECTION:=utils
	CATEGORY:=Broadcom utilities
	TITLE:=Broadcom utilities
	DEPENDS:=bcmkernel
endef

define Package/broadcom-utils/description
	This package contains useful broadcom related utilities.
endef

target=$(firstword $(subst -, ,$(BOARD)))

_make_flags := $(MAKE_FLAGS)
_target_cflags := $(TARGET_CFLAGS)

############################################################################## #
# brcm_fw_tool                                                                 #
############################################################################## #

MAKE_FLAGS := $(_make_flags)
TARGET_CFLAGS := $(_target_cflags) -Dtarget_$(target)=1 -Wall
PKG_BUILD_DIR := $(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

ifdef CONFIG_MTD_REDBOOT_PARTS
	MAKE_FLAGS += FIS_SUPPORT=1
	TARGET_CFLAGS += -DFIS_SUPPORT=1
endif

define Package/brcm_fw_tool
	$(call Package/broadcom-utils/Default)
	TITLE:=Update utility for Broadcom nand firmware images
endef

define Package/brcm_fw_tool/description
	This package contains an utility useful to upgrade from other firmware or 
	older OpenWrt releases.
endef

define Build/brcm_fw_tool/prepare
	mkdir -p $(PKG_BUILD_DIR)/brcm_fw_tool
	$(CP) $(PKG_SOURCE_SUBDIR)/brcm_fw_tool/* $(PKG_BUILD_DIR)/brcm_fw_tool/
endef

define Package/brcm_fw_tool/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/brcm_fw_tool/brcm_fw_tool $(1)/sbin/
endef

############################################################################## #
# brcminfo                                                                     #
############################################################################## #

MAKE_FLAGS := $(_make_flags)
TARGET_CFLAGS := $(_target_cflags) -D$(CONFIG_TARGET_IBOARDID) -Dtarget_$(target)=1 -Wall
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

EXTRA_CFLAGS += -DBOS_OS_LINUXUSER

EXTRA_CFLAGS += -I$(STAGING_DIR)/usr/include/bcm963xx/bcmdrivers/broadcom/include/bcm963xx/
EXTRA_CFLAGS += -I$(STAGING_DIR)/usr/include/bcm963xx/bcmdrivers/opensource/include/bcm963xx/
EXTRA_CFLAGS += -I$(STAGING_DIR)/usr/include/bcm963xx/xChange/dslx_common/voice_res_gw/endpt/inc/
EXTRA_CFLAGS += -I$(STAGING_DIR)/usr/include/bcm963xx/xChange/dslx_common/voice_res_gw/inc
EXTRA_CFLAGS += -I$(STAGING_DIR)/usr/include/bcm963xx/xChange/dslx_common/voice_res_gw/codec
EXTRA_CFLAGS += -I$(STAGING_DIR)/usr/include/bcm963xx/xChange/dslx_common/xchg_common/bos/publicInc/
EXTRA_CFLAGS += -I$(STAGING_DIR)/usr/include/bcm963xx/xChange/dslx_common/voice_res_gw/casCtl/inc/
EXTRA_CFLAGS += -I$(STAGING_DIR)/usr/include/bcm963xx/xChange/dslx_common/xchg_drivers/inc

define Package/brcminfo
	$(call Package/broadcom-utils/Default)
	TITLE:=Broadcom voice driver info utility
endef

define Package/brcminfo/description
	Utility that prints information regarding voice configuration
endef

define Build/brcminfo/prepare
	mkdir -p $(PKG_BUILD_DIR)/brcminfo
	$(CP) $(PKG_SOURCE_SUBDIR)/brcminfo/* $(PKG_BUILD_DIR)/brcminfo/
endef

define Package/brcminfo/install
	$(INSTALL_DIR) $(1)/usr/bin
	cp $(PKG_BUILD_DIR)/brcminfo/brcminfo $(1)/usr/bin/
endef

############################################################################## #

$(eval $(call BuildPackage,brcminfo))
$(eval $(call BuildPackage,brcm_fw_tool))

