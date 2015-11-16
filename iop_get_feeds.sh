./scripts/feeds update
./scripts/feeds install -a 
./scripts/feeds uninstall asterisk18
./scripts/feeds install -f -p juci -a
./scripts/feeds install -f -p intenoprivate
./scripts/feeds install -p intenoprivate brcm63xx_ref_nand
./scripts/feeds install -p intenoprivate brcm63xx_ref_nand_arm
