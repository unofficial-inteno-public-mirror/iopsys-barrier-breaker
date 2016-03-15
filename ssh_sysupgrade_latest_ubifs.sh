#!/bin/sh
{ cd `dirname $0`
IMAGE=`ls -Art bin/*/*.y | tail -n1`
[ "$IMAGE" ] && [ -e "$IMAGE" ] && ./ssh_sysupgrade.sh $1 $IMAGE $2
}
