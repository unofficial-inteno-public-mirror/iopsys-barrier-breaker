./scripts/feeds update
#./scripts/feeds uninstall libmicroxml
#./scripts/feeds uninstall libqrencode
#./scripts/feeds uninstall net-snmp
./scripts/feeds install -f -p juci -a
./scripts/feeds install -f -p intenoprivate -a
./scripts/feeds install -f -p intenotargets -a
./scripts/feeds install -f -p inteno -a
./scripts/feeds install -f -p intenotargets brcm63xx_ref_nand
./scripts/feeds install -f -p intenotargets brcm63xx_ref_nand_arm
./scripts/feeds install -a 
./scripts/feeds uninstall asterisk18
./scripts/feeds uninstall zstream
