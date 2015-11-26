./scripts/feeds update
./scripts/feeds install -a 
./scripts/feeds uninstall asterisk18
./scripts/feeds install -f -p juci -a
./scripts/feeds install -f -p intenoprivate -a
./scripts/feeds install -f -p intenotargetsi -a
./scripts/feeds install -p intenotargets brcm63xx_ref_nand
./scripts/feeds install -p intenotargets brcm63xx_ref_nand_arm
