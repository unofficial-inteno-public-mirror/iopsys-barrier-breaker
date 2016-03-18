#!/bin/bash

Color_Off='\033[0m'       # Text Reset

# Regular Colors
Black='\033[0;30m'        # Black
Red='\033[0;31m'          # Red
Green='\033[0;32m'        # Green
Yellow='\033[0;33m'       # Yellow
Blue='\033[0;34m'         # Blue
Purple='\033[0;35m'       # Purple
Cyan='\033[0;36m'         # Cyan
White='\033[0;37m'        # White

while getopts "v:h" opt; do
    case $opt in
	v)
	    verbose=$OPTARG
	    ;;
	h)
	    echo "some help! No options yet"
	    exit 1
	    ;;
	\?)
	    echo "Invalid option: -$OPTARG" >&2
	    exit 1
	    ;;
    esac
done

if [ -z "$EDITOR" ]
then
    if [ -f /usr/bin/vi ]; then
	EDITOR=vi
    else
	echo "env variable EDITOR needs to be set"
	exit 1
    fi
fi


# allow subshells to abort the whole program by exiting with "exit 99"
set -E
trap '[ "$?" -ne 99 ] || exit 99' ERR

print_git_update()
{
    echo "pkg -> ${PKG_NAME}"
    echo "	PKG_BUILD_DIR	=	${PKG_BUILD_DIR}"
    echo "	PKG_DIR		=	${PKG_DIR}"
    echo "	PKG_SOURCE	=	${PKG_SOURCE}"
    echo "	PKG_NAME	=	${PKG_NAME}"
    echo "	PKG_SOURCE_URL	=	${PKG_SOURCE_URL}"
    echo "	PKG_SOURCE_PROTO=	${PKG_SOURCE_PROTO}"
    echo "	PKG_SOURCE_VERSION=	${PKG_SOURCE_VERSION}"
    echo "	PKG_SOURCE	=	${PKG_SOURCE}"
    echo "	PKG_SOURCE_VERSION_FILE=${PKG_SOURCE_VERSION_FILE}"
}

is_git_same()
{
    git_last=$(cd ${PKG_BUILD_DIR}; git rev-parse HEAD)
    #echo "$PKG_NAME $git_last = ${PKG_SOURCE_VERSION}"
    if [ "$git_last" == "${PKG_SOURCE_VERSION}" ]
    then
	return 0
    fi
    return 1
}

update_this_pkg()
{
    mk_hash=$(get_makefile_hash)

    if [ "$mk_hash" != "${PKG_SOURCE_VERSION}" ]
    then
	echo "${PKG_NAME}:"
	echo "	build dir     = ${PKG_BUILD_DIR}"
	echo "	feed makefile = ${mk_hash}"
	echo "	stale hash    = ${PKG_SOURCE_VERSION}"
	echo "	build git     = $(cd ${PKG_BUILD_DIR}; git rev-parse HEAD)"
	echo "	Git hash in package makefile and the git hash recorded from last compile of"
	echo "	package is different. You probably want to recompile the package"
	echo "	to get an up to date version in ${PKG_BUILD_DIR}/.git_update"
	echo ""

	echo -n "	Shold we continue with the update anyway? [y/N]:"
	read answer
	echo ""

	case $answer in
	    y|Y)
		;;
	    n|N|*)
		return 1;;
	esac
    fi

    echo "${PKG_NAME}:"
    echo "	build dir     = ${PKG_BUILD_DIR}"
    echo "	pkg dir       = ${PKG_DIR}"
    echo "	feed makefile = ${PKG_SOURCE_VERSION}"
    echo "	build git     = $(cd ${PKG_BUILD_DIR}; git rev-parse HEAD)"
    echo "	package is at a different git commit in build compared to feed"
    echo -n "	Should we update the feed and top project to reflect the new version ? [y/N]:"
    read answer
    echo ""

    case $answer in
	y|Y)
	    return 0;;
	*)
	    echo ""
	    return 1;;
    esac
}

get_makefile_hash()
{
    if [ -n "$PKG_SOURCE_VERSION_FILE" ]
    then
	name="$PKG_SOURCE_VERSION_FILE"
    else
	name=Makefile
    fi
    grep "PKG_SOURCE_VERSION:=" ${PKG_DIR}/${name} | sed -e "s/\(^PKG_SOURCE_VERSION:=\)\(.*\)/\2/"
}

insert_hash_in_feed_makefile()
{
    if [ -n "$PKG_SOURCE_VERSION_FILE" ]
    then
	name="$PKG_SOURCE_VERSION_FILE"
    else
	name=Makefile
    fi

    git_last=$(cd ${PKG_BUILD_DIR}; git rev-parse HEAD)

    sed -i -e "s/\(^PKG_SOURCE_VERSION:=\).*/\1${git_last}/" ${PKG_DIR}/${name}
    (cd ${PKG_DIR}; git add ${name})
}


# BUG: fix if only local branche name!
branch_uptodate()
{
    # $1 git repo
    # $2 if it exist dont abort do the pull
    (cd $1
     git remote update 2>&1 >/dev/null

     LOCAL=$(git rev-parse @)
     REMOTE=$(git rev-parse @{u})
     BASE=$(git merge-base @ @{u})

     if [ -z "$REMOTE" ]; then
	 BRANCH=$(basename $(git symbolic-ref -q HEAD))
	 echo "You need to setup a tracking branch for $BRANCH"
	 exit 99
     fi

     if [ $LOCAL = $REMOTE ]; then
	 return
     elif [ $LOCAL = $BASE ]; then
	 if [ -n "$2" ]
	 then
	     echo "Doing automatic pull on [ $1 ]"
	     if git pull
	     then
		 return
	     else
		 echo "Something wrong with pull. aborting, repo at"
		 echo "    [ $1 ]"
		 exit 99
	     fi
	 else
	     echo "Local repo behind remote:"
	     echo "do git pull at repo"
	     echo "    [ $1 ]"
	     exit 99
	 fi
     elif [ $REMOTE = $BASE ]; then
	 echo "Local repo ahead of remote. A push is needed"
	 echo "Repo is at: $1"
	 echo ""
	 echo -n "Should we try a push ? [Y/n]:"
	 read answer
	 echo ""

	 case $answer in
	     n|N|q|Q)
		 exit 99;;
	     *)
		 echo -e "${Yellow}"
		 if ! git push origin HEAD
		 then
		     echo -e "${Color_Off}"
		     exit 99
		 fi
		 echo -e "${Color_Off}Push done."
		 ;;
	 esac

     else
	 echo "Diverged. not sure what you did but there is no tracking branch. "
	 echo "repo at [ $1 ]. fix it so that there is a tracking branch remote."
	 echo "Often this is related to sombody having commited to the same branch"
	 echo "on the server so a simple push wont work, try a 'git rebase'."
	 exit 99
     fi
    )
}

on_a_branch()
{
    local repo=$1
    local type=$2
    (
	cd $repo
	name=$(git symbolic-ref -q HEAD)
	if [ -z "$name" ]
	then
	    echo "git $type repo [ $repo ] is detached."

	    branches=($(git branch -r --contains $(git rev-parse HEAD)))
	    if [ 0 == ${#branches[@]} ]
	    then
		echo "It needs to be on a branch but git could not find any associated branch"
		echo ""
		echo "you need to make sure that the commit is not on a detached branch"
		echo "and that the branch exist in the remote repo also. it can not be a local name"
		echo "as it is about to get pushed so it can be part of system release"
		exit 99
	    fi

	    echo "It needs to be on a branch. Please select one or quit if it is not in list."
	    echo ""

	    i=0
	    for branch in ${branches[*]}
	    do
		echo "$i: $branch"
		i=$((i + 1))
	    done

	    echo ""
	    echo -n "Select what branch to checkout. Q/q or N/n to quit? "
	    read answer

	    case $answer in
		q|Q|n|N)
		    echo "Aborting!"
		    exit 99;;
	    esac

	    echo -e "${Yellow}"
	    pwd
	    echo "git checkout ${branches[$answer]}"
	    if ! git checkout -t ${branches[$answer]}
	    then
		local_branch=$(basename ${branches[$answer]})
		if ! git checkout ${local_branch}
		then
		    echo -e "${Color_Off}"
		    echo "update_git aborting! something was wrong changing to branch ${branches[$answer]}"
		    echo "go to [ $repo ] and fix it."
		    exit 99
		fi
	    fi
	    echo -e "${Color_Off}"
	fi
    )
}

git_repos_uptodate()
{
    on_a_branch ${PKG_BUILD_DIR} package
    on_a_branch ${PKG_DIR} feed
    on_a_branch ${PWD} top
    branch_uptodate ${PKG_BUILD_DIR}
    branch_uptodate ${PKG_DIR} do_pull
    branch_uptodate ${PWD} do_pull
}

get_feed_name()
{

    echo $1 |sed  -e "s|.*feeds/\([^/]*\).*|\1|"

    # rest=$(dirname $1)
    # base=$(basename $1)
    # prev=$base

    # while [ -n "$rest" ]
    # do
    # 	if [ "$base" == "feeds" ]
    # 	then
    # 	    echo "$prev"
    # 	fi
    # done
}

create_message()
{
    FORMAT="commit %H%n\
Author: %aN <%aE>%n\
Date: %ai%n\
%n\
%w(80,4,4)%s%n
%b%n\
%w()Base directory -> ${repo_PATH}/"
    local FROM=${PKG_SOURCE_VERSION}
    local TO=$(cd ${PKG_BUILD_DIR}; git rev-parse HEAD)

    local commits=$(cd ${PKG_BUILD_DIR};git rev-list ${FROM}..${TO})

    local feed=$(get_feed_name ${PKG_DIR})

    echo "Update feed [ $feed ] package [ $PKG_NAME ]"
    echo ""
    echo "-------------------------------------------------------------------------------"
    (cd ${PKG_BUILD_DIR}; git log --graph --oneline ${FROM}..${TO})
    echo "-------------------------------------------------------------------------------"

    for commit in $commits
    do
	(cd ${PKG_BUILD_DIR}; git show --stat --pretty=format:"$FORMAT"  $commit)
	echo "-------------------------------------------------------------------------------"
    done
}

edit_file()
{
    echo -en "${Red}"
    echo "Here is the commit message we are going to use!"
    echo "-------------------------------------------------------------------------------"
    echo -en "${Color_Off}"
    cat $1
    echo -en "${Red}"
    echo "-------------------------------------------------------------------------------"
    echo -en "${Color_Off}"

    echo -n "Do you want to edit the message [y/N]? "
    read answer

    case $answer in
	y|Y)
	       $EDITOR $1;;
    esac
}


commit_feed()
{
    template=$(readlink -f $1)
    edit_file $template

    echo -e "${Yellow}"
    (
	cd ${PKG_DIR}
	if git commit -F $template
	then
	    if git push origin HEAD
	    then
		echo -e "${Color_Off} Feed Updated!"
		return
	    else
		echo -e "${Color_Off}"
		echo "something wrong push feed git ${PKG_DIR}"
		exit 99
	    fi
	else
	    echo -e "${Color_Off}"
	    echo "something wrong committing to feed git ${PKG_DIR}"
	    exit 99
	fi
    )
}

commit_feeds_config()
{
    template=$(readlink -f $1)
    edit_file $template

    echo -e "${Yellow}"
    if git commit -F $template
    then
	if git push origin HEAD
	then
	    echo -e "${Color_Off}Feeds.conf updated!"
	    return
	else
	    echo -e "${Color_Off}"
	    echo "something wrong push change to feeds.conf"
	    echo "try \"git remote update ; git stash ;git rebase; git push;git stash pop\""
	    exit 99
	fi
    else
	echo -e "${Color_Off}"
	echo "something wrong committing to feed git"
	exit 99
    fi
}

insert_hash_in_feeds_config()
{
    local feed=$(get_feed_name ${PKG_DIR})
    local TO=$(cd ${PKG_DIR}; git rev-parse HEAD)

    sed -i feeds.conf -e "/${feed}/ s/\(.*\)[;^].*/\1^${TO}/"
    git add feeds.conf
}

check_packages()
{
    echo -e "${Green}_______________________________________________________________________________${Color_Off}"
    echo "Now checking if any changes has been done to the packages."
    echo -e "${Green}_______________________________________________________________________________${Color_Off}"

    # First scan all files in build dir for packages that have .git directories.
    all_pkgs=$(find build_dir/ -name ".git")

    for pkg in `echo "$all_pkgs"`
    do
	pkg=$(dirname $pkg)

	# check if the git in build is at same commit id as the feed makefile points out
	if [ -e ${pkg}/.git_update ]
	then
	    source ${pkg}/.git_update
	fi

#	print_git_update

	if [ -n "${PKG_NAME}" ]
	then
	    if ! is_git_same
	    then
		if update_this_pkg
		then
		    #  print_git_update
		    git_repos_uptodate
		    insert_hash_in_feed_makefile
		    create_message >tmp/msg
		    commit_feed tmp/msg
		    insert_hash_in_feeds_config
		    commit_feeds_config tmp/msg
		fi
	    fi
	fi
    done
}

# now handle the target git. we have only one


feeds_hash()
{
    grep -v "^#" feeds.conf | grep " $1" | sed -e "s/.*[;^]\(.*\)/\1/"
}

insert_feed_hash_in_feeds_config()
{
    local feed=$1
    local TO=$(cd feeds/${feed}; git rev-parse HEAD)

    sed -i feeds.conf -e "/ ${feed}/ s/\(.*\)[;^].*/\1^${TO}/"
    git add feeds.conf
}

create_feed_message()
{
    local feed=$1
    local FROM=$2
    local TO=$3

    local FORMAT="commit %H%n\
Author: %aN <%aE>%n\
Date: %ai%n\
%n\
%w(80,4,4)%s%n
%b%n\
%w()Base directory -> feeds/$feed/"


    local commits=$(cd feeds/$feed;git rev-list ${FROM}..${TO})

    echo "Update feed [ $feed ]"
    echo ""
    echo "-------------------------------------------------------------------------------"
    (cd feeds/$feed; git log --graph --oneline ${FROM}..${TO})
    echo "-------------------------------------------------------------------------------"

    for commit in $commits
    do
	(cd feeds/$feed; git show --stat --pretty=format:"$FORMAT"  $commit)
	echo "-------------------------------------------------------------------------------"
    done
}




check_feeds()
{
    echo -e "${Green}_______________________________________________________________________________${Color_Off}"
    echo "Now checking if any changes has been done to the feeds."
    echo -e "${Green}_______________________________________________________________________________${Color_Off}"

    feeds=$(grep -v "^#" feeds.conf| awk '{print $2}')
    for feed in `echo $feeds`
    do
	feed_hash=$(feeds_hash $feed)
	in_git=$(cd feeds/$feed; git rev-parse HEAD)

	if [ "$feed_hash" != "$in_git" ]
	then

	    name=$(cd feeds/$feed;git symbolic-ref -q HEAD)
	    if [ -z "$name" ]
	    then
		echo "Feed feeds/${feed} is at a git commit which is different from feeds.conf"
		on_a_branch feeds/${feed} feed

		#redo the test here and see if the feeds.conf and git is still different.
		in_git=$(cd feeds/$feed; git rev-parse HEAD)
		if [ "$feed_hash" = "$in_git" ]
		then
		    continue
		fi
	    fi

	    LOCAL=$(cd feeds/$feed;git rev-parse @)
	    REMOTE=$(cd feeds/$feed;git rev-parse @{u})
	    BASE=$(cd feeds/$feed;git merge-base @ @{u})

	    # if we are behind the remote automatically do a pull
	    if [ $LOCAL = $BASE ]; then
		(cd feeds/$feed ; git pull 1>/dev/null)

		#redo the test here and see if the feeds.conf and git is still different.
		in_git=$(cd feeds/$feed; git rev-parse HEAD)
		if [ "$feed_hash" = "$in_git" ]
		then
		    continue
		fi
	    fi

	    echo "Feed feeds/${feed} is at different commit than what is in feeds.conf"
	    echo -n "Should we update feeds.conf to reflect the new version ? [y/N]:"
	    read answer

	    case $answer in
		n|N)
		    continue;;
	    esac
	    branch_uptodate feeds/${feed}
	    create_feed_message ${feed} $feed_hash $in_git  >tmp/msg
	    insert_feed_hash_in_feeds_config ${feed}
	    commit_feeds_config tmp/msg
	fi
    done
}

feeds_at_top()
{
    git remote update 2>/dev/null 1>/dev/null
    LOCAL=$(git rev-parse @)
    REMOTE=$(git rev-parse @{u})

    if [ $LOCAL = $REMOTE ]
    then
	return
    fi

    local_name=$(git rev-parse --abbrev-ref @ )
    remote_name=$(git rev-parse --abbrev-ref @{u} )
    
    echo "Top repo local branch \"$local_name\" is not at same point as remote \"$remote_name\""
    echo "This update script will update the feeds.conf file and for that to work it needs to"
    echo "be up to date with the remote."
    echo ""
    echo "please run:"
    echo "  git pull"
    echo "  scripts/iop_feeds_update.sh"
    echo ""
    echo "do not forget the bootstrap. but do not run make it can delete your package in build"
    exit 0
}

feeds_at_top
check_packages
check_feeds
