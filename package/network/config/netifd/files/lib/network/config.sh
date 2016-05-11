#!/bin/sh
# Copyright (C) 2011 OpenWrt.org

. /usr/share/libubox/jshn.sh

find_config() {
	local device="$1"
	local ifdev ifl3dev ifobj
	for ifobj in `ubus list network.interface.\*`; do
		interface="${ifobj##network.interface.}"
		(
			json_load "$(ifstatus $interface)"
			json_get_var ifdev device
			json_get_var ifl3dev l3_device
			if [[ "$device" = "$ifdev" ]] || [[ "$device" = "$ifl3dev" ]]; then
				echo "$interface"
				exit 0
			else
				exit 1
			fi
		) && return
	done
}

unbridge() {
	return
}

ubus_call() {
	json_init
	local _data="$(ubus -S call "$1" "$2")"
	[ -z "$_data" ] && return 1
	json_load "$_data"
	return 0
}


fixup_interface() {
	local config="$1"
	local ifname type device l3dev

	config_get type "$config" type
	config_get ifname "$config" ifname
	config_get device "$config" device "$ifname"
	[ "bridge" = "$type" ] && ifname="br-$config"
	config_set "$config" device "$ifname"
	ubus_call "network.interface.$config" status || return 0
	json_get_var l3dev l3_device
	[ -n "$l3dev" ] && ifname="$l3dev"
	json_init
	config_set "$config" ifname "$ifname"
	config_set "$config" device "$device"
}

scan_interfaces() {
	config_load network
	config_foreach fixup_interface interface
}

prepare_interface_bridge() {
	local config="$1"

	[ -n "$config" ] || return 0
	ubus call network.interface."$config" prepare
}

setup_interface() {
	local iface="$1"
	local config="$2"

	[ -n "$config" ] || return 0
	ubus call network.interface."$config" add_device "{ \"name\": \"$iface\" }"
}

do_sysctl() {
	[ -n "$2" ] && \
		sysctl -n -e -w "$1=$2" >/dev/null || \
		sysctl -n -e "$1"
}

get_network_of() {

	find_network() {
		local config="$1"
		local iface="$2"
		local net=
		ifname="$(uci get network.$config.ifname)"
		for if in $ifname; do
			if [ "$if" == "$iface" ]; then
				net=$config
				break
			fi
		done
		[ -n "$net" ] && echo $net
	}

	config_load network
	config_foreach find_network interface $1
}

test_default_route() {
	local defroute="$(ip r | grep default | awk '{print$3}')"
	local def6route="$(ip -f inet6 r | grep default | awk '{print$3}')"
	local ping6dev="$(ip -f inet6 r | grep default | awk '{print$5}')"

	if [ -n "$defroute" ]; then
		ping -q -w 1 -c 1 $defroute >/dev/null 2>&1 && return 0
	elif [ -n "$def6route" ] && [ -n "$ping6dev" ]; then
		ndisc6 -1 $def6route $ping6dev >/dev/null 2>&1 && return 0
	fi
	return 1
}

interfacename() {
	local PORT_ORDER=$(db get hw.board.ethernetPortOrder)
	local PORT_NAMES=$(db get hw.board.ethernetPortNames)
	local cnt=1
	local idx=0

	# get index of interface name
	for i in $PORT_ORDER; do
	    if [ "$i" == "$1" ]; then
		idx=$cnt
	    fi
	    cnt=$((cnt+1))
	done

	# get port name from index
	cnt=1
	for i in $PORT_NAMES; do
	    if [ "$cnt" == "$idx" ]; then
		echo $i
	    fi
	    cnt=$((cnt+1))
	done

	# for wifi use default
	case "$1" in
	    *wl*) echo "WLAN" ;;
	esac
}

get_port_number() {
	local ports="0 1 2 3 4 5 6 7 8"
	local port="$1"
	local ifname

	for p in $ports; do
		ifname="$(ethswctl getifname $p | awk '{print$NF}')"
		if [ "$ifname" == "$port" ]; then
			echo "$p"
			break
		fi
	done
}

# function wait_for_dns
# params: url
# call example: wait_for_dns http://user:password@example.com/path/firmware.y2
# waits for dnsmasq to start
wait_for_dns() {
        local url=$1
        local wait_time=30
        local wait_interval=2

        url=${url#*://}		# remove http://
        url=${url%%/*}		# remove /path/firmware.y2
        url=${url#*@}		# remove user:password@
				# url is only example.com

        echo -n "Waiting for dns ..."
        while [ true ] ; do
                host -t a $url &> /dev/null # try DNS lookup for url
                [ $? == 0 ] && break
                sleep $wait_interval
                echo -n "."
                wait_time=$((wait_time - wait_interval))
                [ "$wait_time" -le "0" ] && { echo " failed"; return 1;} # timer expired
        done
        echo " ok"
        return 0
}
# end of wait_for_dns
