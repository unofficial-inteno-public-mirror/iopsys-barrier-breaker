platform_check_image_size() {
	[ "$(check_image_size "$1")" == "SIZE_OK" ] || {
		return 1
	}
	return 0
}

platform_check_image() {
	local from img_type

	[ "$ARGC" -gt 1 ] && return 1

	echo "Image platform check started ..." > /dev/console

	case "$1" in
		http://*|ftp://*) get_image "$1" "cat" > /tmp/firmware.bin; from=/tmp/firmware.bin;;
		*) from=$1;;
	esac

	[ "$(check_crc "$from")" == "CRC_OK" ] || {
		echo "CRC check failed" > /dev/console
		return 1
	}

	case "$(get_image_type "$from")" in
		"INTENO") img_type=2 ;;
		"CFE+FS") img_type=1 ;;
		"FS")     img_type=0 ;;
		*)
			echo "Unknown image type" > /dev/console
			return 1
	esac
	if [ $img_type -lt 2 ] && grep -q "rootfstype=ubifs" /proc/cmdline; then
		echo "Old image not compatible after upgrade to UBIFS" > /dev/console
		return 1
	fi
	echo $img_type > /tmp/CFE_FS

	if [ -e /etc/ssl/certs/opkg.pem ]; then
	    if ! check_sig "$from"; then
		echo "Signature of file is wrong. Aborting!" > /dev/console
		return 1
	    fi
	fi

	case "$(get_flash_type "$from")" in
		"NAND")
			echo 1 > /tmp/IS_NAND
			;;
		"NOR")
			echo 0 > /tmp/IS_NAND
			;;
		*)
			echo "Unknown flash type" > /dev/console
			return 1
	esac

	[ "$(check_image_size "$from")" == "SIZE_OK" ] || {
		echo "Image size is too large" > /dev/console
		return 1
	}

	if [ "$(get_image_chip_id "$from")" != "$(get_chip_id)" ]; then
		echo "Chip model of image does not match" > /dev/console
		return 1
	fi

	# Customer name check should be carried out
	# only if a regarding parameter set in config.
	# For now skip customer name check.
	if [ 1 -eq 0 ]; then
		[ -f /lib/db/version/iop_customer ] \
			&& [ "$(get_image_customer "$from")" != "$(cat /lib/db/version/iop_customer)" ] && {
			echo "Image customer doesn't match" > /dev/console
			return 1
		}
		# NOTE: expr interprets $(db get hw.board.hardware) as a
		# regexp which could give unexpected results if the harware
		# name contains any magic characters.
		expr "$(get_image_model_name "$from")" : "$(db get hw.board.hardware)" || {
			echo "Image model name doesn't match board hardware" > /dev/console
			return 1
		}
	fi

	echo "Image platform check completed" > /dev/console

	return 0
}

platform_pre_upgrade() {

	local from mtd_no
	local cfe_fs
	local is_nand
	local klink sname

	cfe_fs=$(cat /tmp/CFE_FS)
	is_nand=$(cat /tmp/IS_NAND)

	case "$1" in
		http://*|ftp://*) from=/tmp/firmware.bin;;
		*) from=$1;;
	esac

	if [ $is_nand -ne 1 ]; then
		v "This is not a nand image, aborting!"
		exit 1
	fi

	# Stop unnecessary processes
	for klink in $(ls /etc/rc.d/K[0-9]*); do
		sname=$(basename $(readlink -f $klink))
		case $sname in
			bcmhotproxy|boot|cgroups|dropbear|iwatchdog|telnet|umount)
				v "Not stopping $sname"
				;;
			*)
				v "Stopping $sname..."
				$klink stop
				;;
		esac
	done
	sleep 3s

	#DEBUG
	#ps > /dev/console
	#DEBUG

	if [ "$SAVE_OVERLAY" -eq 1 -a -z "$USE_REFRESH" ]; then
		v "Creating save overlay file marker"
		touch /SAVE_OVERLAY
	else
		v "Not saving overlay files"
		rm -f /SAVE_OVERLAY
	fi
	if [ "$SAVE_CONFIG" -eq 1 -a -z "$USE_REFRESH" ]; then
		v "Creating save config file marker"
		touch /SAVE_CONFIG
	else
		v "Not saving config files"
		rm -f /SAVE_CONFIG
	fi
	sync

	if [ $cfe_fs -eq 2 ]; then
		inteno_image_upgrade $from
	else
		# Old/Brcm format image
		if [ $cfe_fs -eq 1 ]; then
			v "Writing CFE ..."
			cfe_image_upgrade $from 0 131072
		fi

		v "Writing File System ..."
		mtd_no=$(find_mtd_no "rootfs_update")
		if [ $cfe_fs -eq 1 ]; then
			update_sequence_number $from 0 131072
			imagewrite -c -k 131072 /dev/mtd$mtd_no $from
		else
			update_sequence_number $from 0
			imagewrite -c /dev/mtd$mtd_no $from
		fi
	fi
	sync

	v "Setting bootline parameter to boot from newly flashed image"
	brcm_fw_tool set -u 0
	v "Current Software Upgrade Count: $(get_rootfs_sequence_number)"

	v "Upgrade completed!"
	rm -f $from
	[ -n "$DELAY" ] && sleep "$DELAY"
	v "Rebooting system ..."
	sync
	export REBOOT_REASON=upgrade
	reboot -f
}

