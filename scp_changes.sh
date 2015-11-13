#!/bin/sh
{ cd `dirname $0`
	ROOT=build_dir/target-mips_uClibc-0.9.33.2/root-brcm63xx_ref_nand/
	ROOT_OLD=tmp/root_old/
	ROOT_COPY=tmp/root_copy/
	if [ ! -d "$ROOT" ]; then
		echo "$ROOT does not exist"
		echo "please build the project first"
		exit 1;
	fi
	if [ ! -d "$ROOT_OLD" ]; then
		echo "$ROOT_OLD does not exist"
		echo "you didn't make a backup of previous buildroot"
		echo "please run ./scp_changes_reset.sh"
		exit 1;
	fi
	FILES=`diff -rq "$ROOT" "$ROOT_OLD" 2>&1 | sed -ne "s?^Files .* and $ROOT_OLD\\(.*\\) differ?\\1?p" -ne "s?^Only in $ROOT: \\(.*\\)?/\\1?p"`
	if [ "$1" = "-p" ]; then
		echo "files that would be copied:"
		echo $FILES
		exit 0
	fi
	for f in $FILES
	do
		mkdir -p "$ROOT_COPY`dirname $f`"
		cp -af "$ROOT$f" "$ROOT_COPY$f"
	done
	if [ -d "$ROOT_COPY" ]; then
		echo "scp changed files to $1"
		pushd "$ROOT_COPY" 2>&1 >/dev/null
		scp -r * root@$1:/
		RETVAL=$?
		popd 2>&1 >/dev/null
		rm -rf "$ROOT_COPY"
		if [ "$RETVAL" -eq 0 ]; then
			./scp_changes_reset.sh
		else
			echo "scp error"
			exit $RETVAL
		fi
	else
		echo "no change"
	fi
}
