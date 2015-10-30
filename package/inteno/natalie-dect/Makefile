#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=natalie-dect
PKG_VERSION:=11.19.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=git@iopsys.inteno.se:natalie-dect-11.19
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=2015e8106c7d541dd038381c2845bd8462d74a30
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/image.mk
include $(INCLUDE_DIR)/kernel.mk



BCM_KERNEL_VERSION=`cat $(BUILD_DIR)/bcmkernel/bcm963xx/kernel/linux-*/include/config/kernel.release`

export BUILD_DIR
export BCM_KERNEL_VERSION


define Package/natalie-dect
  SECTION:=net
  CATEGORY:=Base system
  TITLE:=Dect stack
  URL:=
  DEPENDS:=+bcmkernel
endef

define Package/natalie-dect/description
	Kernel dect driver
endef


define Package/natalie-dect/install

	mkdir -p $(1)/lib/modules/$(BCM_KERNEL_VERSION)/extra/
	$(CP) $(PKG_BUILD_DIR)/NatalieFpCvm6362/Src/Projects/NatalieV3/FpCvm/Linux6362/dects.ko $(1)/lib/modules/$(BCM_KERNEL_VERSION)/extra/dect.ko

endef

$(eval $(call BuildPackage,natalie-dect))
