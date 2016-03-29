#!/bin/bash

if [ "$(whoami)" != "root" ]; then
	echo "Please run this script as root!"
	exit 1
fi

packages_all="bison flex g++ g++-multilib zlib1g-dev gettext gawk svn-buildpackage libncurses5-dev ncurses-term git automake gtk-doc-tools liblzo2-dev uuid-dev execstack"
packages_x64="libc6-dev-i386 lib32z1"
packages_npm="npm nodejs yui-compressor"

curdir=$(pwd)

#===============#
# Prerequisites #
#===============#

mysh=$(ls -hl /bin/sh | awk -F'[ ,/]' '{print$NF}')
if [ "$mysh" != "bash" ]; then
	echo "On Debian based systems, e.g. Ubuntu, /bin/sh must point to bash instead of $mysh"
	read -p "Do you approve this change (y/n): " ans
	if [ "$ans" == "y" ]; then
		rm -f /bin/sh
		ln -s bash /bin/sh
	else
		echo "Warning! You haven't pointed /bin/sh to bash."
		cd $curdir
		exit 1
	fi
fi

echo "The packages below must be installed"
echo $packages_all
read -p "Do you approve insallation of these packages (y/n): " ans
if [ "$ans" == "y" ]; then
	apt-get install $packages_all
else
	cd $curdir
	exit 1
fi

if [ "$(uname -m | awk '{print$1}')" == "x86_64" ]; then
	echo "You are running a Linux Distribution based on 64-bit kernel"
	echo "The packages below must be installed"
	echo $packages_x64
	read -p "Do you approve insallation of these packages (y/n): " ans
	if [ "$ans" == "y" ]; then
		apt-get install $packages_x64
	else
		cd $curdir
		exit 1
	fi
fi

echo "The packages below must be installed in order to be able to compile JUCI"
echo $packages_npm
read -p "Do you approve insallation of these packages (y/n): " ans
if [ "$ans" == "y" ]; then
	apt-get install $packages_npm
	npm install
	npm install -g grunt-cli
	npm install -g mocha
	npm install -g bower
	npm install -g uglify-js
	npm install -g less

	if [ "$(which node)" == "" ]; then
		NODEJS=$(which nodejs)
		if [ "$NODEJS" != "" ]; then
			read -p "Found nodejs executable at $(which nodejs), but no path to 'node'. Do you want to create a symlink? (y/n): " ans
			if [ "$ans" == "y" ]; then
				ln -s "$NODEJS" "/usr/bin/node"
			fi
		fi
	fi

	user=$(who | head -1 | awk '{print$1}')
	[ -n $user ] && chown -R $user:$user /home/$user/.npm/
else
	cd $curdir
	exit 1
fi

# Get the Broadcom toolchain from here and unpack the mips/arm package to /opt:
echo -n "Checking for Broadcom MIPS toolchain: "
if [ -d /opt/toolchains/crosstools-mips-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21 ]; then
	install_mips=0
	echo "YES"
else
	install_mips=1
	echo "NO"
fi

echo -n "Checking for Broadcom ARM toolchain: "
if [ -d /opt/toolchains/crosstools-arm-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21-NPTL ]; then
	install_arm=0
	echo "YES"
else
	install_arm=1
	echo "NO"
fi

if [ $install_mips -eq 1 -o $install_arm -eq 1 ]; then
	read -p "Do you approve insallation of missing toolchains (y/n): " ans
	if [ "$ans" == "y" ]; then
		echo "Downloading toolchain"
	else
		cd $curdir
		exit 1
	fi

	cd ~
	wget http://iopsys.inteno.se/iopsys/toolchain/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21.Rel1.2-full.tar.bz2
	tar jxf crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21.Rel1.2-full.tar.bz2
	cd /

	if [ $install_mips -eq 1 ]; then
		echo "Installing MIPS toolchain"
		tar jxf ~/crosstools-mips-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21.Rel1.2.tar.bz2
	fi

	if [ $install_arm -eq 1 ]; then
		echo "Installing ARM toolchain"
		tar jxf ~/crosstools-arm-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21-NPTL.Rel1.2.tar.bz2
	fi

	rm -f ~/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21-sources.tar.bz2
	rm -f ~/crosstools-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21.Rel1.2-full.tar.bz2
	rm -f ~/crosstools-mip*-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21.Rel1.2.tar.bz2
	rm -f ~/crosstools-arm-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21-NPTL.Rel1.2.tar.bz2
fi

gcc_ver=$(ls -l /usr/bin/gcc-[0-9]* | head -1 | awk '{print$NF}' | cut -d'-' -f2)
if [ "$gcc_ver" != "4.8" ]; then
	echo "Your current gcc version is $gcc_ver, but it must be changed to 4.8"
	read -p "Do you approve this change (y/n): " ans
	if [ "$ans" == "y" ]; then
		apt-get install gcc-4.8
		apt-get install g++-4.8
		apt-get install gcc-4.8-multilib

		update-alternatives --install /usr/bin/g++ c++ /usr/bin/g++-4.8 100
		update-alternatives --install /usr/bin/g++ c++ /usr/bin/g++-5 90

		update-alternatives --install /usr/bin/gcc cc /usr/bin/gcc-4.8 100
		update-alternatives --install /usr/bin/gcc cc /usr/bin/gcc-5 90

		update-alternatives --install /usr/bin/cpp cpp /usr/bin/cpp-4.8 100
		update-alternatives --install /usr/bin/cpp cpp /usr/bin/cpp-5 90

		update-alternatives --set c++ /usr/bin/g++-4.8
		update-alternatives --set cc  /usr/bin/gcc-4.8
		update-alternatives --set cpp /usr/bin/cpp-4.8
		ln -s /etc/alternatives/cc /usr/bin/cc 

		echo "The deafult gcc version has now been changed from $gcc_ver to 4.8"
	else
		cd $curdir
		exit 1
	fi
fi

echo ""
echo ""
echo "You have successfully installed and configred prerequisites to be able to build an iopsys firmware"
echo ""
echo ""

cd $curdir
