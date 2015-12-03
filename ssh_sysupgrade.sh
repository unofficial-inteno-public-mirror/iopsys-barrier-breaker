#!/bin/sh
IMAGE=`basename $2`
echo "sysupgrade host: $1 with file $IMAGE"
[ "$2" ] && [ -e "$2" ] && scp $2 root@$1:/tmp/ && ssh root@$1 "sysupgrade $3 /tmp/$IMAGE" && echo "sysupgrade done!"
