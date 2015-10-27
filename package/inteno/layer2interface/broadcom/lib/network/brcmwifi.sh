wlmngr_setupTPs() {
        local pid_nic0=$(pgrep wl0-kthrd)
        local pid_nic1=$(pgrep wl1-kthrd)
        local pid_dhd0=$(pgrep dhd0_dpc)
        local pid_dhd1=$(pgrep dhd1_dpc)
        local pid_wfd0=$(pgrep wfd0-thrd)
        local pid_wfd1=$(pgrep wfd1-thrd)
        local pid_wl0=${pid_dhd0:-$pid_nic0}
        local pid_wl1=${pid_dhd1:-$pid_nic1}

        # set affinity
        if [ -n "$pid_wl0" -a -n "$pid_wl1" ]; then
                # bind to TP0
                taskset -p 1 $pid_wl0
                # bind to TP1
                taskset -p 2 $pid_wl1
        else
                if [ "$pid_wl0" == "$pid_dhd0" ]; then
                        # bind to TP0
                        taskset -p 1 $pid_wl0
                else
                        # bind to TP1
                        taskset -p 2 $pid_wl0
                fi
        fi

        # set priority
        local pid pids
        pids="$pid_wl0 $pid_wl1 $pid_wfd0 $pid_wfd1"
        for pid in $pids; do
                chrt -rp 5 $pid
        done
}

wlmngr_stopServices() {
	local idx=$1

	killall -q -9 lld2d 2>/dev/null
	killall -q -9 wps_ap 2>/dev/null
	killall -q -9 wps_enr 2>/dev/null
	killall -q -15 wps_monitor 2>/dev/null # Kill Child Thread first, -15: SIGTERM to force to remove WPS IE
	killall -15 bcmupnp 2>/dev/null # -15: SIGTERM, force bcmupnp to send SSDP ByeBye message out (refer to CR18802) 
	rm -rf /var/bcmupnp.pid

	killall -q -9 nas 2>/dev/null
	killall -q -9 eapd 2>/dev/null

	killall -q -9 acsd 2>/dev/null	

	nvram unset acs_ifnames # remove wlidx only

	killall -q -15 wapid
	killall -q -15 hspotap
	killall -q -15 bsd
	killall -q -15 ssd
	killall -q -9 vis-datacollector
	killall -q -9 vis-dcon
}

wlmngr_WlConfDown() {
	local idx=$1
	wlconf wl$idx down
}

wlmngr_setSsid() {
#	local idx=$1

#	for vif in $(nvram get "wl$idx"_vifs); do
#		wlctl -i wl$idx ssid -C $vifno $(nvram get "$vif"_ssid)
#	done
	return
}

wlmngr_wlIfcDown() {
	local idx=$1

	for vif in wl$idx $(nvram get "wl$idx"_vifs); do
		ifconfig $vif down
	done
}

wlmngr_doWlConf() {
	local idx=$1;

	#wlctl -i wl$idx nreqd $nreqd
	wlconf wl$idx up

}

wlmngr_setupMbssMacAddr() {
	local idx=$1
	local hwaddr

	for vif in $(nvram get "wl$idx"_vifs); do
		hwaddr=$(nvram get "$vif"_hwaddr)
		ifconfig $vif hw ether $hwaddr 2>/dev/null
		wlctl -i $vif cur_etheraddr $hwaddr 2>/dev/null
	done
}

#enableBSD() {
#	nvram_set bsd_role=0
#	[ act_wl_cnt == 0 ] && return FALSE
#	for i in wl0 wl1; do
#		if [ m_instance_wl[i].m_wlVar.wlEnbl == TRUE && m_instance_wl[i].m_wlVar.bsdRole > 0 ]; then
#			nvram set bsd_role=m_instance_wl[i].m_wlVar.bsdRole
#			nvram set bsd_pport=m_instance_wl[i].m_wlVar.bsdPport
#			nvram set bsd_hpport=m_instance_wl[i].m_wlVar.bsdHport
#			nvram set bsd_helper=m_instance_wl[i].m_wlVar.bsdHelper
#			nvram set bsd_primary=m_instance_wl[i].m_wlVar.bsdPrimary
#			return TRUE
#		fi
#	done
#	return FALSE
#}

#enableSSD() {
#	nvram set ssd_enable=0
#	[ act_wl_cnt == 0 ] && return FALSE
#	for i in wl0 wl1; do
#		if [ m_instance_wl[i].m_wlVar.wlEnbl == TRUE && m_instance_wl[i].m_wlVar.ssdEnable > 0 ]; then
#			nvram set ssd_enable=m_instance_wl[i].m_wlVar.ssdEnable
#			return TRUE
#		fi
#	done
#	return FALSE
#}

wlmngr_startServices() {
	local idx=$1

	#bcmupnp -D
	lld2d br-lan # $(nvram get lan_ifname)
	eapd
	nas
	acsd

#	wlmngr_startWsc() {
#		return
#	}

#	wlmngr_startSes() {
#		if [ m_instance_wl[idx].m_wlVar.wlSesEnable == FALSE ]; then
#			return
#		fi
#		ses -f
#	}
#	wlmngr_startSesCl() {
#		if [ m_instance_wl[idx].m_wlVar.wlSesClEnable == FALSE ]; then
#			return
#		fi
#		ses_cl -f
#	}

#	wapid

#	wlmngr_BSDCtrl() {
#		killall -q -15 bsd 2>/dev/null
#		if [ "$(enableBSD)" == "TRUE" ]; then
#			bsd&
#		fi
#	}

#	wlmngr_SSDCtrl(){
#		killall -q -15 ssd 2>/dev/null
#		if [ "$(enableSSD)" == "TRUE" ]; then
#			ssd&
#		fi
#	}
}

wlmngr_startWsc()
{
	local idx=$1
	local wlunit

	wlunit=$(nvram get wl_unit)

	[ -n $wlunit ] && nvram set wl_unit=0

	nvram set wps_mode=enabled # "enabled/disabled"
	nvram set wl_wps_config_state=1 # "1/0"
	nvram set wl_wps_reg="enabled"
	nvram set lan_wps_reg=enabled #"enabled/disabled"
	nvram set wps_uuid=0x000102030405060708090a0b0c0d0ebb
	nvram set wps_device_name=BroadcomAP
	nvram set wps_mfstring=Broadcom
	nvram set wps_modelname=Broadcom
	nvram set wps_modelnum=123456
	nvram set boardnum=1234
	nvram set wps_timeout_enable=0
	#nvram get wps_config_method
	nvram set wps_version2=enabled # extra
	nvram set lan_wps_oob=disabled # extra
	nvram set lan_wps_reg=enabled # extra


	if [ "$(nvram get wps_version2)" == "enabled" ]; then
		nvram set _wps_config_method=sta-pin
	else
		nvram set _wps_config_method=pbc
	fi
		
	nvram set wps_config_command=0
	nvram set wps_status=0
	nvram set wps_method=1
	nvram set wps_proc_mac=""

	if [ "$(nvram get wps_restart)" == "1" ]; then
		nvram set wps_restart=0
	else
		nvram set wps_restart=0
		nvram set wps_proc_status=0
	fi

	nvram set wps_sta_pin=00000000
	nvram set wps_currentband=""
	nvram set wps_autho_sta_mac="00:00:00:00:00:00"

	wps_monitor&
}

wlmngr_issueWpsCmd() {
	return
}

wlmngr_WlConfStart() {
	local idx=$1
	wlconf wl$idx up
}

wlmngr_wlIfcUp() {
	local idx=$1

	for vif in wl$idx $(nvram get "wl$idx"_vifs); do
		if [ "$(nvram get "$vif"_bss_enabled)" == "1" ]; then
			ifconfig $vif up 2>/dev/null
			wlctl -i $vif bss up # extra
		else
			ifconfig $vif down
		fi
	done
}

wlmngr_doWds() {
	return
}

wlmngr_doQoS() {
	local idx=$1

	for vif in wl$idx $(nvram get "wl$idx"_vifs); do
		ebtables -t nat -D POSTROUTING -o $vif -p IPV4 -j wmm-mark 2>/dev/null
		ebtables -t nat -D POSTROUTING -o $vif -p IPV6 -j wmm-mark 2>/dev/null
		ebtables -t nat -D POSTROUTING -o $vif -p 802_1Q -j wmm-mark --wmm-marktag vlan 2>/dev/null
		if [ "$(nvram get "$vif"_bss_enabled)" == "1" ]; then
			ebtables -t nat -A POSTROUTING -o $vif -p IPV4 -j wmm-mark 2>/dev/null
			ebtables -t nat -A POSTROUTING -o $vif -p IPV6 -j wmm-mark 2>/dev/null
			ebtables -t nat -A POSTROUTING -o $vif -p 802_1Q -j wmm-mark --wmm-marktag vlan 2>/dev/null
		fi
	done
}

wlmngr_finalize() {
	local idx=$1

	wlctl -i wl$idx phy_watchdog 1
	wlctl -i wl$idx fcache 1

	# send ARP packet with bridge IP and hardware address to device
	# this piece of code is -required- to make br0's mac work properly
	# in all cases
	sendarp -s br-lan -d br-lan
}

wlmngr_issueServiceCmd() {
	return
}

wlmngr_HspotCtrl() {
	killall -q -15 hspotap 2>/dev/null
	if [ "$(enableHspot)" == "TRUE" ]; then
		hspotap&
	fi
}

wlmngr_postStart() {
	return
}
