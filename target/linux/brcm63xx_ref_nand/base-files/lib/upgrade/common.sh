#!/bin/sh

RAM_ROOT=/tmp/root

ldd() { LD_TRACE_LOADED_OBJECTS=1 $*; }
libs() { ldd $* | awk '{print $3}'; }

install_file() { # <file> [ <file> ... ]
	for file in "$@"; do
		dest="$RAM_ROOT/$file"
		[ -f $file -a ! -f $dest ] && {
			dir="$(dirname $dest)"
			mkdir -p "$dir"
			cp $file $dest
		}
	done
}

install_bin() { # <file> [ <symlink> ... ]
	src=$1
	files=$1
	[ -x "$src" ] && files="$src $(libs $src)"
	install_file $files
	[ -e /lib/ld.so.1 ] && {
		install_file /lib/ld.so.1
	}
	shift
	for link in "$@"; do {
		dest="$RAM_ROOT/$link"
		dir="$(dirname $dest)"
		mkdir -p "$dir"
		[ -f "$dest" ] || ln -s $src $dest
	}; done
}

supivot() { # <new_root> <old_root>
	/bin/mount | grep "on $1 type" 2>&- 1>&- || /bin/mount -o bind $1 $1
	mkdir -p $1$2 $1/proc $1/sys $1/dev $1/tmp $1/overlay && \
	/bin/mount -o noatime,move /proc $1/proc && \
	pivot_root $1 $1$2 || {
		/bin/umount -l $1 $1
		return 1
	}

	/bin/mount -o noatime,move $2/sys /sys
	/bin/mount -o noatime,move $2/dev /dev
	/bin/mount -o noatime,move $2/tmp /tmp
	/bin/mount -o noatime,move $2/overlay /overlay 2>&-
	return 0
}

run_ramfs() { # <command> [...]
	install_bin /bin/busybox /bin/ash /bin/sh /bin/mount /bin/umount	\
		/sbin/pivot_root /usr/bin/wget /sbin/reboot /bin/sync /bin/dd	\
		/bin/grep /bin/cp /bin/mv /bin/tar /usr/bin/md5sum "/usr/bin/["	\
		/bin/dd /bin/vi /bin/ls /bin/cat /usr/bin/awk /usr/bin/hexdump	\
		/bin/sleep /bin/zcat /usr/bin/bzcat /usr/bin/printf /usr/bin/wc \
		/bin/cut /usr/bin/printf /bin/sync /bin/mkdir /bin/rmdir	\
		/bin/rm /usr/bin/basename /bin/kill /bin/chmod

	install_bin /sbin/mtd
	install_bin /sbin/ubi
	install_bin /sbin/mount_root
	install_bin /sbin/snapshot
	install_bin /sbin/snapshot_tool
	install_bin /usr/sbin/ubiupdatevol
	install_bin /usr/sbin/ubiattach
	install_bin /usr/sbin/ubiblock
	install_bin /usr/sbin/ubiformat
	install_bin /usr/sbin/ubidetach
	install_bin /usr/sbin/ubirsvol
	install_bin /usr/sbin/ubirmvol
	install_bin /usr/sbin/ubimkvol
	for file in $RAMFS_COPY_BIN; do
		install_bin ${file//:/ }
	done
	install_file /etc/resolv.conf /lib/*.sh /lib/functions/*.sh /lib/upgrade/*.sh $RAMFS_COPY_DATA

	[ -L "/lib64" ] && ln -s /lib $RAM_ROOT/lib64

	supivot $RAM_ROOT /mnt || {
		echo "Failed to switch over to ramfs. Please reboot."
		exit 1
	}

	/bin/mount -o remount,ro /mnt
	/bin/umount -l /mnt

	grep /overlay /proc/mounts > /dev/null && {
		/bin/mount -o noatime,remount,ro /overlay
		/bin/umount -l /overlay
	}

	# spawn a new shell from ramdisk to reduce the probability of cache issues
	exec /bin/busybox ash -c "$*"
}

kill_remaining() { # [ <signal> ]
	local sig="${1:-TERM}"
	echo -n "Sending $sig to remaining processes ... "

	local my_pid=$$
	local my_ppid=$(cut -d' ' -f4  /proc/$my_pid/stat)
	local my_ppisupgraded=
	grep -q upgraded /proc/$my_ppid/cmdline >/dev/null && {
		local my_ppisupgraded=1
	}

	local stat
	for stat in /proc/[0-9]*/stat; do
		[ -f "$stat" ] || continue

		local pid name state ppid rest
		read pid name state ppid rest < $stat
		name="${name#(}"; name="${name%)}"

		local cmdline
		read cmdline < /proc/$pid/cmdline

		# Skip kernel threads
		[ -n "$cmdline" ] || continue

		if [ $$ -eq 1 ] || [ $my_ppid -eq 1 ] && [ -n "$my_ppisupgraded" ]; then
			# Running as init process, kill everything except me
			if [ $pid -ne $$ ] && [ $pid -ne $my_ppid ]; then
				echo -n "$name "
				kill -$sig $pid 2>/dev/null
			fi
		else
			case "$name" in
				# Skip essential services
				*procd*|*ash*|*init*|*watchdog*|*ssh*|*dropbear*|*telnet*|*login*|*hostapd*|*wpa_supplicant*|*nas*|*cwmpd*|*ice*) : ;;

				# Killable process
				*)
					if [ $pid -ne $$ ] && [ $ppid -ne $$ ]; then
						echo -n "$name "
						kill -$sig $pid 2>/dev/null
					fi
				;;
			esac
		fi
	done
	echo ""
}

run_hooks() {
	local arg="$1"; shift
	for func in "$@"; do
		eval "$func $arg"
	done
}

ask_bool() {
	local default="$1"; shift;
	local answer="$default"

	[ "$INTERACTIVE" -eq 1 ] && {
		case "$default" in
			0) echo -n "$* (y/N): ";;
			*) echo -n "$* (Y/n): ";;
		esac
		read answer
		case "$answer" in
			y*) answer=1;;
			n*) answer=0;;
			*) answer="$default";;
		esac
	}
	[ "$answer" -gt 0 ]
}

v() {
	[ "$VERBOSE" -ge 1 ] && echo "$@" > /dev/console
}

rootfs_type() {
	/bin/mount | awk '($3 ~ /^\/$/) && ($5 !~ /rootfs/) { print $5 }'
}

update_sequence_number() {
	seqn=$(ls -l /root/../ | grep cferam | awk -F' ' '{print$9}' | cut -d'.' -f 2)
	if [ $2 -eq 1 ]; then
		/sbin/brcm_fw_tool -s "$seqn" -w update $1
	else
		/sbin/brcm_fw_tool -s "$seqn" update $1
	fi
}

check_crc() {
	/sbin/brcm_fw_tool check $1
}

get_flash_type() {
	/sbin/brcm_fw_tool -t check $1
}

get_image_type() {
	/sbin/brcm_fw_tool -i check $1
}

get_image_chip_id() {
	/sbin/brcm_fw_tool -c check $1
}

get_image_board_id() {
	/sbin/brcm_fw_tool -r check $1
}

get_image_model_name() {
	/sbin/brcm_fw_tool -m check $1
}

get_image_customer() {
	/sbin/brcm_fw_tool -o check $1
}


check_image_size() {
	/sbin/brcm_fw_tool -y check $1
}

get_image() { # <source> [ <command> ]
	local from="$1"
	local conc="$2"
	local cmd

	case "$from" in
		http://*|ftp://*) cmd="wget -O- -q";;
		*) cmd="cat";;
	esac
	if [ -z "$conc" ]; then
		local magic="$(eval $cmd $from 2>/dev/null | dd bs=2 count=1 2>/dev/null | hexdump -n 2 -e '1/1 "%02x"')"
		case "$magic" in
			1f8b) conc="zcat";;
			425a) conc="bzcat";;
		esac
	fi

	eval "$cmd $from 2>/dev/null ${conc:+| $conc}"
}

get_magic_word() {
	(get_image "$@" | dd bs=2 count=1 | hexdump -v -n 2 -e '1/1 "%02x"') 2>/dev/null
}

get_magic_long() {
	(get_image "$@" | dd bs=4 count=1 | hexdump -v -n 4 -e '1/1 "%02x"') 2>/dev/null
}

refresh_mtd_partitions() {
	mtd refresh rootfs
}

jffs2_copy_config() {
	if grep rootfs_data /proc/mtd >/dev/null; then
		# squashfs+jffs2
		mtd -e rootfs_data jffs2write "$CONF_TAR" rootfs_data
	else
		# jffs2
		mtd jffs2write "$CONF_TAR" rootfs
	fi
}

default_do_upgrade() {
	sync
	local from
	local cfe_fs=0
	local is_nand=0
	local fwpath

	fwpath=$(uci -q get system.@upgrade[0].fw_upload_path)
	[ -n "$fwpath" ] || fwpath="/tmp/firmware.bin"

	[ $(cat /tmp/CFE_FS) -eq 1 ] && cfe_fs=1
	[ $(cat /tmp/IS_NAND) -eq 1 ] && is_nand=1

	case "$1" in
		http://*|ftp://*) from="$fwpath";;
		*) from=$1;;
	esac

	if [ $cfe_fs -eq 1 ]; then
		mtd_cfe="$(grep "\"CFE\"" /proc/mtd | awk -F: '{print $1}')"
		if [ $is_nand -eq 0 ]; then
			v "Writing CFE ..."
			dd if=$from of=/tmp/cfe bs=64k count=1
			dd if=/dev/$mtd_cfe of=/tmp/cfe bs=1 count=1k skip=1408 seek=1408 conv=notrunc
			mtd write /tmp/cfe CFE
		fi
	fi

	if [ $is_nand -eq 1 ]; then
		v "Setting bootline parameter to boot from newly flashed image"
		/sbin/brcm_fw_tool set -u 0
		v "Current Software Upgrade Count: $(ls /cferam* | awk -F'.' '{print$NF}')"
		if [ "$SAVE_CONFIG" -eq 1 -a -z "$USE_REFRESH" ]; then
			v "Creating save config file marker"
			touch /SAVE_CONFIG
			sync
		else
			v "Not saving config files"
			touch /SAVE_CONFIG
			rm /SAVE_CONFIG
			sync
		fi
		[ $cfe_fs -eq 1 ] && v "Writing CFE + File System ..." || v "Writing File System ..."
        v "-> Kill JFFS2 garbage collector ..."
        kill -KILL $(ps |grep jffs2_gcd_mtd |grep -v grep |awk '{print $1}') &> /dev/console
        v "-> Display meminfo ..."
        cat /proc/meminfo > /dev/console
        v "-> Display procs ..."
        ps > /dev/console
		v "-> Disable printk interrupt ..."
        echo 0 >/proc/sys/kernel/printk_with_interrupt_enabled
		v "-> Will reboot the system after writing finishes ..."
		/sbin/brcm_fw_tool -V -q write $from &> /dev/console
		v "Upgrade syscall failed for some reason ..."
	else
		if [ "$SAVE_CONFIG" -eq 1 -a -z "$USE_REFRESH" ]; then
			v "Writing File System with Saved Config ..."
			if [ $cfe_fs -eq 1 ]; then
				mtd -j "$CONF_TAR" write $from -i 0x00010000 linux
			else
				mtd -j "$CONF_TAR" write $from linux
			fi
		else
			v "Writing File System ..."
			if [ $cfe_fs -eq 1 ]; then
				mtd write $from -i 0x00010000 linux
			else
				mtd write $from linux
			fi
		fi
	fi
}

do_upgrade() {
	v "Performing system upgrade ..."
	if type 'platform_do_upgrade' >/dev/null 2>/dev/null; then
		platform_do_upgrade "$ARGV"
	else
		default_do_upgrade "$ARGV"
	fi

	[ "$SAVE_CONFIG" -eq 1 -a -n "$USE_REFRESH" ] && {
		v "Refreshing partitions ..."
		if type 'platform_refresh_partitions' >/dev/null 2>/dev/null; then
			platform_refresh_partitions
		else
			refresh_mtd_partitions
		fi
		if type 'platform_copy_config' >/dev/null 2>/dev/null; then
			platform_copy_config
		else
			jffs2_copy_config
		fi
	}
	v "Upgrade completed"
	[ -n "$DELAY" ] && sleep "$DELAY"
	ask_bool 1 "Reboot" && {
		v "Rebooting system..."
		reboot -f
		sleep 5
		echo b 2>/dev/null >/proc/sysrq-trigger
	}
}
