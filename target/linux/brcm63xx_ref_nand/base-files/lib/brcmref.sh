#!/bin/sh

bcm_dsl_annex() {
        ANNEX=`cat /proc/nvram/dslAnnex`
#       echo $ANNEX

        if [ -f "/etc/adsl/adsl_phy.bin" ]
        then
                echo "DSL firmware symlink set"
        else
                if [ "$ANNEX" = "A" ]; then
                        echo "DSL Annex A detected"
                        ln -s /etc/dsl/a_adsl_phy.bin /etc/adsl/adsl_phy.bin
                elif [ "$ANNEX" = "B" ]; then
                        echo "DSL Annex B detected"
                        ln -s /etc/dsl/b_adsl_phy.bin /etc/adsl/adsl_phy.bin
                else
                        echo "DSL Annex A default"
                        ln -s /etc/dsl/a_adsl_phy.bin /etc/adsl/adsl_phy.bin
                fi
        fi
}

brcm_insmod() {
	echo Loading brcm modules
        test -e /lib/modules/3.4.11-rt19/extra/chipinfo.ko     && insmod /lib/modules/3.4.11-rt19/extra/chipinfo.ko
	test -e /lib/modules/3.4.11-rt19/extra/bcmxtmrtdrv.ko  && insmod /lib/modules/3.4.11-rt19/extra/bcmxtmrtdrv.ko
        test -e /lib/modules/3.4.11-rt19/extra/bcm_ingqos.ko   && insmod /lib/modules/3.4.11-rt19/extra/bcm_ingqos.ko
	test -e /lib/modules/3.4.11-rt19/extra/bcm_bpm.ko      && insmod /lib/modules/3.4.11-rt19/extra/bcm_bpm.ko
	test -e /lib/modules/3.4.11-rt19/extra/pktflow.ko      && insmod /lib/modules/3.4.11-rt19/extra/pktflow.ko
	test -e /lib/modules/3.4.11-rt19/extra/pktcmf.ko       && insmod /lib/modules/3.4.11-rt19/extra/pktcmf.ko
	test -e /lib/modules/3.4.11-rt19/extra/bcmfap.ko       && insmod /lib/modules/3.4.11-rt19/extra/bcmfap.ko
	test -e /lib/modules/3.4.11-rt19/extra/bcmxtmcfg.ko    && insmod /lib/modules/3.4.11-rt19/extra/bcmxtmcfg.ko
	test -e /lib/modules/3.4.11-rt19/extra/adsldd.ko       && insmod /lib/modules/3.4.11-rt19/extra/adsldd.ko
	test -e /lib/modules/3.4.11-rt19/extra/i2c_bcm6xxx.ko  && insmod /lib/modules/3.4.11-rt19/extra/i2c_bcm6xxx.ko
	test -e /lib/modules/3.4.11-rt19/extra/bcm_enet.ko     && insmod /lib/modules/3.4.11-rt19/extra/bcm_enet.ko
	test -e /lib/modules/3.4.11-rt19/extra/nciTMSkmod.ko   && insmod /lib/modules/3.4.11-rt19/extra/nciTMSkmod.ko
	test -e /lib/modules/3.4.11-rt19/extra/bcmsw.ko        && insmod /lib/modules/3.4.11-rt19/extra/bcmsw.ko && ifconfig bcmsw up
#	test -e /lib/modules/3.4.11-rt19/extra/bcm_usb.ko      && insmod /lib/modules/3.4.11-rt19/extra/bcm_usb.ko
	sleep 1
	test -e /lib/modules/3.4.11-rt19/extra/bcmarl.ko       && insmod /lib/modules/3.4.11-rt19/extra/bcmarl.ko
#	/usr/bin/taskset 2>/dev/null
	test -e /lib/modules/3.4.11-rt19/extra/wfd.ko          && insmod /lib/modules/3.4.11-rt19/extra/wfd.ko
	test -e /lib/modules/3.4.11-rt19/extra/wlcsm.ko        && insmod /lib/modules/3.4.11-rt19/extra/wlcsm.ko
	test -e /lib/modules/3.4.11-rt19/extra/wlemf.ko        && insmod /lib/modules/3.4.11-rt19/extra/wlemf.ko
	test -e /lib/modules/3.4.11-rt19/extra/dhd.ko          && insmod /lib/modules/3.4.11-rt19/extra/dhd.ko  firmware_path=/etc/wlan/dhd mfg_firmware_path=/etc/wlan/dhd/mfg
	test -e /lib/modules/3.4.11-rt19/extra/wl.ko           && insmod /lib/modules/3.4.11-rt19/extra/wl.ko
	test -e /lib/modules/3.4.11-rt19/extra/dect.ko         && insmod /lib/modules/3.4.11-rt19/extra/dect.ko
	test -e /lib/modules/3.4.11-rt19/extra/dectshim.ko     && insmod /lib/modules/3.4.11-rt19/extra/dectshim.ko
	test -e /lib/modules/3.4.11-rt19/extra/pcmshim.ko      && insmod /lib/modules/3.4.11-rt19/extra/pcmshim.ko
	test -e /lib/modules/3.4.11-rt19/extra/endpointdd.ko   && insmod /lib/modules/3.4.11-rt19/extra/endpointdd.ko
	test -e /lib/modules/3.4.11-rt19/extra/p8021ag.ko      && insmod /lib/modules/3.4.11-rt19/extra/p8021ag.ko
	test -e /lib/modules/3.4.11-rt19/extra/bcmvlan.ko      && insmod /lib/modules/3.4.11-rt19/extra/bcmvlan.ko
#	test -e /lib/modules/3.4.11-rt19/extra/pwrmngtd.ko     && insmod /lib/modules/3.4.11-rt19/extra/pwrmngtd.ko
	test -e /lib/modules/3.4.11-rt19/rng-core.ko           && insmod /lib/modules/3.4.11-rt19/rng-core.ko
	test -e /lib/modules/3.4.11-rt19/extra/bcmtrng.ko      && insmod /lib/modules/3.4.11-rt19/extra/bcmtrng.ko
	echo brcm modules loaded
}

brcm_env() {
	echo "Y" > /sys/module/printk/parameters/time
	echo Setting up brcm environment
	/bin/mount -a
	mknod /var/fuse c 10 229
	chmod a+rw /var/fuse
	mkdir -p /var/log /var/run /var/state/dhcp /var/ppp /var/udhcpd /var/zebra /var/siproxd /var/cache /var/tmp /var/samba /var/samba/share /var/samba/homes /var/samba/private /var/samba/locks

# Create FIFO devices

mknod /dev/initctl p

#Create character devices
mknod /dev/mem c 1 1
mknod /dev/kmem c 1 2
mknod /dev/null c 1 3
mknod /dev/port c 1 4
mknod /dev/zero c 1 5
mknod /dev/tty0 c 4 0
mknod /dev/tty1 c 4 1
mknod /dev/ttyS0 c 4 64
mknod /dev/ttyS1 c 4 65
mknod /dev/tty c 5 0
mknod /dev/console c 5 1
mknod /dev/ppp c 108 0
mknod /dev/printer0 c 180 0
mknod /dev/urandom c 1 9
mknod /dev/i2c-0 c 89 0
mknod /dev/mtd0 c 90 0
mknod /dev/mtd1 c 90 2
mknod /dev/mtd2 c 90 4
mknod /dev/mtd3 c 90 6
mknod /dev/ubi0 c 254 0
mknod /dev/ubi_ctrl c 10 63
mknod /dev/random c 1 8
mknod /dev/hwrandom c 10 183
mknod /dev/ptm c 128 1
mkdir /dev/pts/ 
mknod /dev/pts/0 c 136 0 
mknod /dev/pts/1 c 136 1 nod /dev/ubi_ctrl c 10 63

# Create Broadcom specific devices                                                                            
mknod /dev/dect c 197 0
mknod /dev/dectdbg c 197 1
mknod /dev/dectshim c 198 0
mknod /dev/bcmatm0 c 205 0
mknod /dev/brcmboard c 206 0
mknod /dev/bcmvdsl0 c 207 0
mknod /dev/bcmadsl0 c 208 0
mknod /dev/bcmadsl1 c 208 1
mknod /dev/bcmendpoint0 c 209 0
mknod /dev/bcmaal20 c 210 0
mknod /dev/bcmles0 c 211 0
mknod /dev/bcm c 212 0
mknod /dev/bounce c 213 0
mknod /dev/pcmshim0 c 217 0
mknod /dev/pmon c 214 0
mknod /dev/ac97 c 222 0
mknod /dev/slac c 223 0
mknod /dev/bcmprof c 224 0
mknod /dev/si3215 c 225 0
mknod /dev/bcmatmb0 c 226 0
mknod /dev/p8021ag0 c 227 0
mknod /dev/bcmxtmcfg0 c 228 0
mknod /dev/pktcmf c 232 0
mknod /dev/spu c 233 0
mknod /dev/bcmmoca0 c 234 0
mknod /dev/bcmmoca10 c 234 1
mknod /dev/bcm_user_ploam c 235 0
mknod /dev/bcm_omci c 236 0
mknod /dev/bcm_ploam c 237 0
mknod /dev/bcmvlan c 238 0
mknod /dev/laser_dev c 239 0
mknod /dev/pwrmngt c 240 0
mknod /dev/bcmfap c 241 0
mknod /dev/fcache c 242 0
mknod /dev/ingqos c 243 0
mknod /dev/bpm c 244 0
mknod /dev/bcmarl c 245 0
mknod /dev/chipinfo c 246 0
mknod /dev/bcmepon c 247 0
mknod /dev/bcm_user_tod c 248 0
mknod /dev/gmac c 249 0
mknod /dev/tms c 250 0
mknod /dev/ext_bonding c 255 0

mknod /dev/capi20 c 68 0
mknod /dev/isdninfo c 45 255
mknod /dev/ippp0 c 45 128
mknod /dev/ippp1 c 45 129
mknod /dev/isdnctrl0 c 45 64
mknod /dev/isdnctrl c 45 64
mknod /dev/isdn c 45 0
mknod /dev/linux-uk-proxy c 125 0
mknod /dev/linux-user-bde c 126 0

# Create block devices
mknod /dev/ram0 b 1 0
mknod /dev/ram1 b 1 1
mknod /dev/ram2 b 1 2
mknod /dev/ram3 b 1 3
ln -s ram1 /dev/ram

mknod /dev/mtdblock0 b 31 0
mknod /dev/mtdblock1 b 31 1
mknod /dev/mtdblock2 b 31 2
mknod /dev/mtdblock3 b 31 3
mknod /dev/mtdblock4 b 31 4
mknod /dev/mtdblock5 b 31 5
mknod /dev/mtdblock6 b 31 6
mknod /dev/mtdblock7 b 31 7

mknod /dev/sda  b 8 0
mknod /dev/sda1 b 8 1
mknod /dev/sda2 b 8 2
mknod /dev/sda3 b 8 3
mknod /dev/sda4 b 8 4
mknod /dev/sda5 b 8 5
mknod /dev/sda6 b 8 6
mknod /dev/sda7 b 8 7
mknod /dev/sda8 b 8 8
mknod /dev/sda9 b 8 9
mknod /dev/sdb  b 8 16
mknod /dev/sdb1 b 8 17
mknod /dev/sdb2 b 8 18
mknod /dev/sdb3 b 8 19
mknod /dev/sdb4 b 8 20
mknod /dev/sdb5 b 8 21
mknod /dev/sdb6 b 8 22
mknod /dev/sdb7 b 8 23
mknod /dev/sdb8 b 8 24
mknod /dev/sdb9 b 8 25
mknod /dev/sdc  b 8 32
mknod /dev/sdc1 b 8 33
mknod /dev/sdc2 b 8 34
mknod /dev/sdc3 b 8 35
mknod /dev/sdc4 b 8 36
mknod /dev/sdc5 b 8 37
mknod /dev/sdc6 b 8 38
mknod /dev/sdc7 b 8 39
mknod /dev/sdc8 b 8 40
mknod /dev/sdc9 b 8 41
mknod /dev/sdd  b 8 48
mknod /dev/sdd1 b 8 49
mknod /dev/sdd2 b 8 50
mknod /dev/sdd3 b 8 51
mknod /dev/sdd4 b 8 52
mknod /dev/sde  b 8 64
mknod /dev/sde1 b 8 65
mknod /dev/sde2 b 8 66
mknod /dev/sde3 b 8 67
mknod /dev/sde4 b 8 68
mknod /dev/sdf  b 8 80
mknod /dev/sdf1 b 8 81
mknod /dev/sdf2 b 8 82
mknod /dev/sdf3 b 8 83
mknod /dev/sdf4 b 8 84
mknod /dev/sdg  b 8 96
mknod /dev/sdg1 b 8 97
mknod /dev/sdg2 b 8 98
mknod /dev/sdg3 b 8 99
mknod /dev/sdg4 b 8 100
mknod /dev/sdh  b 8 112
mknod /dev/sdh1 b 8 113
mknod /dev/sdh2 b 8 114
mknod /dev/sdh3 b 8 115
mknod /dev/sdh4 b 8 116
}

id_upgrade_reconfig() {

    local basemac=$(cat /proc/nvram/BaseMacAddr | tr '[a-z]' '[A-Z]')
    local boardid=$(cat /proc/nvram/BoardId)
    local vendorid=${basemac:0:8}
    if [ "$boardid" == "963268BU" ]; then
        if [ "$vendorid" == "00 22 07" ]; then
            echo "Setting new boardid and voiceboardid"
            echo DG301R0 > /proc/nvram/BoardId
            echo SI32176X2 > /proc/nvram/VoiceBoardId
            echo "00 00 00 01" >/proc/nvram/ulBoardStuffOption
            sync
            sleep 3
            /sbin/brcm_fw_tool set -x 17 -p 0
        fi
    fi
}

check_num_mac_address() {
    local nummac=$(cat /proc/nvram/NumMacAddrs)
    local boardid=$(cat /proc/nvram/BoardId)
    if [ "$boardid" != "CG300R0" ]; then
        if [ "$nummac" != "00 00 00 08 " ]; then
            echo "Setting NumMacAddrs to 8"
            echo "00 00 00 08" >/proc/nvram/NumMacAddrs
            sync
        fi
    fi
}

brcm_env
id_upgrade_reconfig
bcm_dsl_annex
check_num_mac_address
brcm_insmod

