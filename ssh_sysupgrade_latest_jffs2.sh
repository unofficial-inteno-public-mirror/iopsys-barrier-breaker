#!/bin/sh
{ cd `dirname $0`
IMAGE=`ls -Art bin/*/*.w | tail -n1`
[ "$IMAGE" ] && [ -e "$IMAGE" ] && ./ssh_sysupgrade.sh $1 $IMAGE $2
}
