cd $1
NEW_HASH=`git log -n 1 --pretty=format:"%H"`
cd ../../..
sed -r -i  "s/^PKG_SOURCE_VERSION:=.*/PKG_SOURCE_VERSION:=$NEW_HASH/g" package/$2/Makefile
git commit package/$2/Makefile -m "$2: update package"
