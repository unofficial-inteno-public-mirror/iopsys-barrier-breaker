
define git_update
	echo -e ""\
	"PKG_BUILD_DIR=$(subst $(TOPDIR)/,,$(PKG_BUILD_DIR))\n"\
	"PKG_SOURCE=$(PKG_SOURCE)\n"\
	"PKG_NAME=$(PKG_NAME)\n"\
	"PKG_SOURCE_URL=$(PKG_SOURCE_URL)\n"\
	"PKG_SOURCE_PROTO=$(PKG_SOURCE_PROTO)\n"\
	"PKG_SOURCE_VERSION=$(PKG_SOURCE_VERSION)\n"\
	"PKG_SOURCE=$(PKG_SOURCE)\n"\
	"PKG_DIR=$(subst $(TOPDIR)/,,${CURDIR})\n"\
	"PKG_SOURCE_VERSION_FILE=$(PKG_SOURCE_VERSION_FILE)\n"\
	> $(PKG_BUILD_DIR)/.git_update

endef

Hooks/Prepare/Post += git_update

