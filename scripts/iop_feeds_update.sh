#! /bin/bash

while getopts "d" opt; do
    case $opt in
        d)
	    if [[ tmp/.iop_bootstrap -nt feeds.conf ]]; then
		exit 0
	    fi
	    
            TIMESTAMP=1
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
    esac
done

cp .config .genconfig_config_bak

rm -rf package/feeds

./scripts/feeds update
./scripts/feeds install -f -p juci -a
./scripts/feeds install -f -p intenoprivate -a
./scripts/feeds install -f -p intenotargets -a
./scripts/feeds install -f -p intenopackages -a
./scripts/feeds install -f -p intenobroadcom -a
./scripts/feeds install -f -p intenoopenwrt -a
./scripts/feeds install -f -p intenotargets brcm63xx_ref_nand
./scripts/feeds install -f -p intenotargets brcm63xx_ref_nand_arm
./scripts/feeds install -a 
./scripts/feeds uninstall asterisk18
./scripts/feeds uninstall zstream
./scripts/feeds uninstall mtd-utils
./scripts/feeds install -f -p intenopackages mtd-utils
./scripts/feeds uninstall qrencode
./scripts/feeds install -f -p intenopackages qrencode

rm -rf package/feeds/oldpackages/libzstream # have to run this for now since uninstall is not working every time

cp .genconfig_config_bak .config
make defconfig

# record when we last run this script
touch tmp/.iop_bootstrap 

# always return true
exit 0


