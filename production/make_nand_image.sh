#!/bin/bash -e
#
# Test script for the make_nand_image.lib routines.
#
# Usage example:
#  $ BOARDID=DG301R0 VOICEBOARDID=SI32176X2 BOARDSTUFF=0x1 production/make_nand_image.sh
#
# NVRAM Parameters:
#  BOARDID="STR" (mandatory)
#  VOICEBOARDID="STR" (mandatory)
#  NUMMACADDR=N (optional)
#  MACADDR="xx:xx:xx:xx:xx:xx" (optional)
#  TPNUM=N (optional)
#  PSISIZE=N (optional)
#  LOGSIZE=N (optional)
#  BACKUPPSI=0|1 (optional)
#  GPONSN="STR" (optional)
#  GPONPW="STR" (optional)
#  BOOTSTR="STR" (optional)
#  BOARDSTUFF=N (optional)
#  NVRAMOFS=N (optional, default=0x580)
#
# Note:
#  This file can be used as a template, e.g. when creating dedicated scripts
#  for the generation of production images.
#  Variables under "Helper definitions" is for this test and can be omitted at
#  will but the variables under "Required definitions" must all have valid
#  values before calling "create_images".
#
# (C) 2016, Inteno Broadband AB
#

. production/make_nand_image.lib


### Helper definitions ###

get_config_val () {
	local key=$1
	local val

	val=$(awk -F= "/CONFIG_$key/ {print \$NF}" $DOT_CONFIG)
	echo $val |sed 's/"//g'
}

TOP_DIR=$(pwd)
DOT_CONFIG=$TOP_DIR/.config
BCM_CHIP_ID=$(get_config_val BCM_CHIP_ID)
TARGET_BOARD=$(get_config_val TARGET_BOARD)
TARGET_MODEL_NAME=$(get_config_val TARGET_MODEL_NAME)
TARGET_CUSTOMER=$(get_config_val TARGET_CUSTOMER)
TARGET_VERSION=$(get_config_val TARGET_VERSION)
TOOL_DIR=$TOP_DIR/staging_dir/host
BUILD_DIR=$TOP_DIR/build_dir
TARGET_BUILD_DIR=$BUILD_DIR/target-*
IMAGE_BUILD_DIR=$BUILD_DIR/linux-$TARGET_BOARD


### Required definitions ###

# Tools (path to executable)
MKFS_JFFS2=$TOOL_DIR/bin/mkfs.jffs2
UBINIZE=$TOOL_DIR/bin/ubinize
EDITNVRAM=$(find $TARGET_BUILD_DIR/bcmkernel/ -type f -name editnvram)
CREATENFIMG=$(find $TARGET_BUILD_DIR/bcmkernel/ -type f -name createnfimg)

# Input image files
# - Unused files must be set to "none".
# - Release system is defined by KERNEL0 and UBIFS0.
# - ITT system is defined by either KERNEL1 and UBIFS1 or by ITTIMG (JFFS2),
#   if all three are set to none, ITT system is assumed to be JFFS2 but will
#   have to be generated separately, e.g. by OEM.
CFE=$IMAGE_BUILD_DIR/cfe${BCM_CHIP_ID}_nand.v
KERNEL0=$IMAGE_BUILD_DIR/vmlinux.lz
UBIFS0=$IMAGE_BUILD_DIR/root.ubifs
KERNEL1=none
UBIFS1=none
#ITTIMG=$TOP_DIR/itt_fs_image
ITTIMG=none

# Working directory
WORK_DIR=$BUILD_DIR/nand_image-$TARGET_BOARD

# Output file
OUT_DIR=$TOP_DIR/bin/$TARGET_BOARD
OUT_FILE=${TARGET_MODEL_NAME}_${TARGET_CUSTOMER}${TARGET_VERSION}_nand-img.tgz


### Run script ###
mkdir -p $WORK_DIR
create_images

