#!/bin/sh
{ cd `dirname $0`
IMAGE=`ls -Art bin/brcm63xx_ref_nand/*.y | tail -n1`
[ "$IMAGE" ] && [ -e "$IMAGE" ] && ./ssh_sysupgrade.sh $1 $IMAGE $2
}
