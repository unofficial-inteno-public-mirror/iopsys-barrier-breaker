#!/bin/sh
{ cd `dirname $0`
	ROOT=build_dir/target-mips_uClibc-0.9.33.2/root-brcm63xx_ref_nand/
	ROOT_OLD=tmp/root_old/
	rm -rf "$ROOT_OLD"
	mkdir -p "$ROOT_OLD"
	cp -a "$ROOT"* "$ROOT_OLD"
}
