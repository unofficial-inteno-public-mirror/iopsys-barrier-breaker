#! /bin/bash

REWRITE="$1"

all=$(find . -name ".git_update")
mirrors=$(ssh git@public.inteno.se 2>/dev/null | awk '/ R / {print $NF}'| tr '\r' ' ')

#echo $mirrors

in_mirror()
{
    for shard in $mirrors
    do
#	echo "$shard = $1"
	if [ $shard = $1 -o ${shard}.git = $1 ]
	then
	    return 0;
	fi
    done

    return 1
}

print_help()
{
    
    cat <<EOF
login to public.inteno.se and setup an mirror

  ssh god@public.inteno.se
  cd /home/git/repositories
  sudo su git
  git clone --mirror ${PKG_SOURCE_URL}

logout, exit 2 times

to make it possible to clone do

  git clone git@public.inteno.se:gitolite-admin gitolite-admin-public
  vim gitolite-admin-public/conf/gitolite.conf

add the following to the file
---
 repo $repo
    R      = @all

---
Do not miss that the config needs to end with an empty line.
and remove the .git from the repo name to avoid a warning.
EOF
}


for pkg in ${all}
do
    . $pkg 2>/dev/null
    if [ "git" = "$PKG_SOURCE_PROTO" ]
    then
#	echo "Package $PKG_NAME is using git at $PKG_SOURCE_URL"
	repo=$(basename ${PKG_SOURCE_URL})
	server=$(dirname ${PKG_SOURCE_URL})

	if [[ $PKG_SOURCE_URL != *inteno.se* ]]
	then
	    
	    # If rewrite is set, we are an inteno developer. Otherwise skip this part.
	    if [ "$REWRITE" = y ]; then
		if ! in_mirror $repo
		then
		    echo -n "no mirror for $PKG_NAME. "
		    print_help
		    echo "-------------------------------------------------------------------------------"
		else
		    # BUG: make sure that the mirror contains the correct commit.
		    # the checkout does it and we need to warn that the mirror is not used here
		    # so the user see it.
		    true
		    #		echo "Mirror for $PKG_NAME. "
		fi
	    fi
	fi
      fi
done
