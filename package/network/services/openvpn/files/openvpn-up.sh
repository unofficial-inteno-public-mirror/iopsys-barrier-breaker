#!/bin/sh
# this script is called by openvpn after TUN/TAP device creation, through --up
# params: tun_dev tun_mtu link_mtu ifconfig_local_ip ifconfig_remote_ip [ init | restart ]

. /lib/functions.sh

VPN=""
# $dev is set by the caller

find_interface() {
	interface=$1
	config_get ifname $interface ifname
	if [ $ifname == $dev ]; then
		VPN=$interface
	fi
}

find_zone() {
	local zone=$1
	config_get masq $zone masq 0

	if [ $masq == "1" ] ; then
		config_get network $zone network
		# if VPN is not already in zone.networks list, add it
		echo $network | grep -q -E "$VPN$"\|"$VPN "
		if [ $? != "0" ] ; then
			uci add_list firewall.$zone.network=$VPN
			ubus call uci commit '{"config":"firewall"}'
		fi
	fi
}

add_vpnroutes() {
	# config_load network

	# available vars: $remote_1, $route_net_gateway, $route_vpn_gateway
	# $ifconfig_local, $ifconfig_remote, $trusted_ip

	# route to the IP providing the tunneling service
	uci set network.${VPN}_remote=route
	uci set network.${VPN}_remote.interface=wan
	uci set network.${VPN}_remote.target=$trusted_ip
	uci set network.${VPN}_remote.netmask=255.255.255.255
	uci set network.${VPN}_remote.gateway=$route_net_gateway

	# route to the other end of the tunnel
	uci set network.${VPN}_peer=route
	uci set network.${VPN}_peer.interface=$VPN
	uci set network.${VPN}_peer.target=$route_vpn_gateway
	uci set network.${VPN}_peer.netmask=255.255.255.255
	uci set network.${VPN}_peer.gateway=0.0.0.0

	# first half of the default route through the tunnel
	uci set network.${VPN}_defroute1=route
	uci set network.${VPN}_defroute1.interface=$VPN
	uci set network.${VPN}_defroute1.target=0.0.0.0
	uci set network.${VPN}_defroute1.netmask=128.0.0.0
	uci set network.${VPN}_defroute1.gateway=$route_vpn_gateway

	# second half of the default route through the tunnel
	uci set network.${VPN}_defroute2=route
	uci set network.${VPN}_defroute2.interface=$VPN
	uci set network.${VPN}_defroute2.target=128.0.0.0
	uci set network.${VPN}_defroute2.netmask=128.0.0.0
	uci set network.${VPN}_defroute2.gateway=$route_vpn_gateway

}

main() {
	#tun_dev=$1
	#tun_mtu=$2
	#link_mtu=$3
	#ifconfig_local_ip=$4
	#ifconfig_remote_ip=$5
	#mode=$6

	### update network config ###
	# if a network interface based on $dev exists, use its name
	# else add network.vpn
	config_load network
	config_foreach find_interface interface
	if [ -z $VPN ]; then
		VPN="vpn"
		uci set network.$VPN=interface
		uci set network.$VPN.ifname=$dev
	fi
	uci set network.$VPN.proto=static
	uci set network.$VPN.ipaddr=$ifconfig_local
	uci set network.$VPN.netmask=255.255.255.255

	add_vpnroutes
	ubus call uci commit '{"config":"network"}'

	### update firewall config ###
	# if the masquerading zone (usually wan)
	# does not include vpn in the network list, add it
	config_load firewall
	config_foreach find_zone zone

}

main $@
