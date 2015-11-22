#!/bin/bash

# this script exports latest changes from this repo into inteno targets and inteno feeds repos. 
# you can use this script to export latest changes from 3.4 to 3.5


git rev-parse --verify devel || { 
	echo "Devel branch does not exist!" >&2
	exit 1
}

iop-cleanup(){
	git rev-parse --verify "inteno-packages" 2>/dev/null && git branch -D "inteno-packages"
	git rev-parse --verify "inteno-targets" 2>/dev/null && git branch -D "inteno-targets"
	git rev-parse --verify "devel-export" 2>/dev/null && git branch -D "devel-export"
}

git checkout devel 
iop-cleanup 

git fetch origin 
git checkout -b "devel-export"
git reset --hard origin/devel

set -e 

git subtree split --prefix=target/linux -b inteno-targets
git checkout inteno-targets
git filter-branch -f --prune-empty --tree-filter 'rm -rf adm5120 adm8668 amazon ar7 ar71xx at91 atheros au1000 avr32 brcm2708 brcm47xx brcm63xx cns21xx cns3xxx cobalt coldfire ep93xx etrax gemini generic goldfish imx21 iop32x ixp4xx kirkwood lantiq leon Makefile malta mcs814x mpc52xx mpc83xx mpc85xx octeon omap24xx omap35xx omap4 orion ppc40x ppc44x ps3 pxa pxcab ramips rb532 rdc realview s3c24xx sibyte sparc ubicom32 uml x86 xburst' inteno-targets
git remote add intenotargets git@iopsys.inteno.se:inteno-targets.git
git push intenotargets inteno-targets:from-iop-3.4
git remote rm intenotargets

git checkout devel-export 

git subtree split --prefix=package/inteno -b inteno-packages
git checkout inteno-packages
git remote add intenopackages git@iopsys.inteno.se:inteno-packages.git
git push intenopackages inteno-packages:from-iop-3.4
git remote rm intenopackages

git checkout devel
iop-cleanup
