#!/bin/sh

git remote -v | grep -q http && return # do not continue if this is an open SDK environment

profile=$(grep CONFIG_BCM_KERNEL_PROFILE .config | cut -d'=' -f2 | tr -d '"')
sdkversion=$(grep "CONFIG_BRCM_SDK_VER.*=y" .config | awk -F'[_,=]' '{print$5}')
curdir=$(pwd)

build_bcmkernel_consumer() {
	local tarfile bcmkernelcommith
	bcmkernelcommith=$(grep -w "PKG_SOURCE_VERSION:" $curdir/feeds/feed_inteno_broadcom/bcmkernel/$sdkversion.mk | cut -d'=' -f2)
	# do not build bcmopen sdk if it was already built before
	ssh inteno@iopsys.inteno.se "ls public/www/iopsys/consumer/bcmopen-$profile-$bcmkernelcommith.tar.gz" && return
	cd ./build_dir/target-*_uClibc-0.9.33.*/bcmkernel-3.4-$sdkversion/bcm963xx/release
	sh do_consumer_release -p $profile -y
	tarfile='out/bcm963xx_*_consumer.tar.gz'
	[ $(ls -1 $tarfile |wc -l) -ne 1 ] && echo "Too many tar files: '$tarfile'" && return
	scp $tarfile inteno@ihgsp.inteno.se:/home/inteno/public/www/iopsys/consumer/bcmopen-$profile-$bcmkernelcommith.tar.gz
	rm -f $tarfile
	cd $curdir
}

build_natalie_consumer() {
	# create natalie-dect open version tar file
	local natalieversion nataliecommith
	grep -q "CONFIG_TARGET_NO_DECT=y" .config && return
	natalieversion=$(grep -w "PKG_VERSION:" ./feeds/feed_inteno_packages/natalie-dect/Makefile | cut -d'=' -f2)
	nataliecommith=$(grep -w "PKG_SOURCE_VERSION:" ./feeds/feed_inteno_packages/natalie-dect/Makefile | cut -d'=' -f2)
	ssh inteno@iopsys.inteno.se "ls public/www/iopsys/consumer/natalie-dect-$profile-$natalieversion-$nataliecommith.tar.gz" && return
	cd ./build_dir/target-*_uClibc-0.9.33.*/natalie-dect-$natalieversion/
	mkdir natalie-dect-open-$natalieversion
	cp NatalieFpCvm6362/Src/Projects/NatalieV3/FpCvm/Linux6362/dects.ko natalie-dect-open-$natalieversion/dect.ko
	tar -czv  natalie-dect-open-$natalieversion/ -f natalie-dect-$profile-$natalieversion-$nataliecommith.tar.gz
	scp natalie-dect-$profile-$natalieversion-$nataliecommith.tar.gz inteno@iopsys.inteno.se:/home/inteno/public/www/iopsys/consumer/
	cp natalie-dect-$profile-$natalieversion-$nataliecommith.tar.gz $curdir/
	rm -rf natalie-dect-open-$natalieversion
	rm -f natalie-dect-$profile-$natalieversion-$nataliecommith.tar.gz
	cd $curdir
}

build_endptcfg_consumer() {
	# create endptcfg open version tar file
	local endptversion endptcommith
	grep -q "CONFIG_TARGET_NO_VOICE=y" .config && return
	endptversion=$(grep -w "PKG_VERSION:" ./feeds/feed_inteno_packages/endptcfg/Makefile | cut -d'=' -f2)
	endptcommith=$(grep -w "PKG_SOURCE_VERSION:" ./feeds/feed_inteno_packages/endptcfg/Makefile | cut -d'=' -f2)
	ssh inteno@iopsys.inteno.se "ls public/www/iopsys/consumer/endptcfg-$profile-$endptversion-$endptcommith.tar.gz" && return
	cd ./build_dir/target-*_uClibc-0.9.33.*/endptcfg-$endptversion/
	mkdir endptcfg-open-$endptversion
	cp endptcfg endptcfg-open-$endptversion/
	tar -czv  endptcfg-open-$endptversion/ -f endptcfg-$profile-$endptversion-$endptcommith.tar.gz
	scp endptcfg-$profile-$endptversion-$endptcommith.tar.gz inteno@iopsys.inteno.se:/home/inteno/public/www/iopsys/consumer/
	cp endptcfg-$profile-$endptversion-$endptcommith.tar.gz $curdir/
	rm -rf endptcfg-open-$endptversion
	rm -f endptcfg-$profile-$endptversion-$endptcommith.tar.gz
	cd $curdir
}

build_ice_consumer() {
	# create ice-client open version tar file
	local iceversion icebasever icerelease icecommith
	icecommith=$(grep -w "PKG_SOURCE_VERSION:" ./feeds/feed_inteno_packages/ice-client/Makefile | head -1 | cut -d'=' -f2)
	icebasever=$(grep -w "BASE_PKG_VERSION:" ./feeds/feed_inteno_packages/ice-client/Makefile | cut -d'=' -f2)
	icerelease=$(grep -w "PKG_RELEASE:" ./feeds/feed_inteno_packages/ice-client/Makefile | cut -d'=' -f2)
	iceversion=$icebasever$icerelease
	ssh inteno@iopsys.inteno.se "ls public/www/iopsys/consumer/ice-client-$profile-$iceversion-$icecommith.tar.gz" && return
	cd ./build_dir/target-*_uClibc-0.9.33.*/ice-client-$icebasever/ipkg-brcm*
	tar -czv  ice-client -f ice-client-$profile-$iceversion-$icecommith.tar.gz
	scp ice-client-$profile-$iceversion-$icecommith.tar.gz inteno@iopsys.inteno.se:/home/inteno/public/www/iopsys/consumer/
	cp ice-client-$profile-$iceversion-$icecommith.tar.gz $curdir/
	rm -f ice-client-$profile-$iceversion-$icecommith.tar.gz
	cd $curdir
}

build_bcmkernel_consumer
build_natalie_consumer
build_endptcfg_consumer
build_ice_consumer
