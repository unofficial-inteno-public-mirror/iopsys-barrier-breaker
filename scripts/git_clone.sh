#! /bin/bash

URL="$1"
SUBDIR="$2"
MIRROR="$3"
VERSION="$4"
REWRITE="$5"

repo=$(basename ${URL})
server=$(dirname ${URL})


#echo "repo = ${repo}"
#echo "server = ${server}"
#echo "Mirror = ${MIRROR}"

#first try to use mirror
if [ -n "${MIRROR}" ]
then

    # is this an inteno server ? in that case do not use the mirror
    use_mirror=0

    if [[ $URL != *inteno.se* ]]
    then
	use_mirror=1
    fi

    if [ "$REWRITE" = y ]
    then
	use_mirror=1
    fi

    if [ $use_mirror = 1 ]
    then
	echo "trying to clone from mirror ${MIRROR}/${repo}"
	if git clone ${MIRROR}:${repo} ${SUBDIR} --recursive
	then
	    old="$PWD"
	    cd ${SUBDIR}
	    if git checkout ${VERSION}
	    then
		exit 0
	    fi
	    # checkout failed mirror is not correct
	    cd $old
	    rm -rf cd ${SUBDIR}
	fi
    fi
fi

#if not try the original
echo "No working mirror cloning from ${URL}"
git clone ${URL} ${SUBDIR} --recursive
