#!/bin/sh
# this script is called by openvpn after closing TUN/TAP device, through --down
# params: tun_dev tun_mtu link_mtu ifconfig_local_ip ifconfig_remote_ip [ init | restart ]

. /lib/functions.sh

VPN=""
# $dev is set by the caller

find_interface() {
	interface=$1
	config_get ifname $interface ifname
	if [ $ifname == $dev ]; then
		VPN=$interface
		uci delete network.$VPN.ipaddr
		uci delete network.$VPN.netmask
	fi
}

main() {
	echo $0 params: $*
	#tun_dev=$1
	#tun_mtu=$2
	#link_mtu=$3
	#ifconfig_local_ip=$4
	#ifconfig_remote_ip=$5
	#mode=$6

	### update network config ###
	# remove all vpn related ips and routes
	config_load network
	config_foreach find_interface interface

	uci delete network.${VPN}_remote
	uci delete network.${VPN}_peer
	uci delete network.${VPN}_defroute1
	uci delete network.${VPN}_defroute2

	ubus call uci commit '{"config":"network"}'

}

main $@
