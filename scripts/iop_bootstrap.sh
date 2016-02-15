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
