#!/bin/sh
{ cd `dirname $0`
IMAGE=`ls -Art bin/brcm63xx_ref_nand/*.w | tail -n1`
[ "$IMAGE" ] && [ -e "$IMAGE" ] && ./cfe_upgrade.sh $1 $IMAGE
}
