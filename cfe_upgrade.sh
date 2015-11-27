#!/bin/sh
IMAGE=`basename $2`
echo "CFE upgrade host: $1 with file $IMAGE"
[ "$2" ] && [ -e "$2" ] &&  curl -i -F filedata=@$2  http://$1/upload.cgi && echo "upgrade done!"
