#
# spec file for package kernel-3.14.y-fslc-imx6-sr
#
# Copyright (c) 2014-2015 Josua Mayer <josua.mayer97@gmail.com>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://www.solid-run.com/community/
#

#BuildArch: armv7l armv7hl

%define baseversion 3.14.54
%define localversion -fslc-imx6-sr

Name: kernel-3.14.y-fslc-imx6-sr
Summary: 3.14 LTS Kernel for Freescale i.MX6 devices
Url: https://github.com/SolidRun/linux-fslc/tree/3.14-1.0.x-mx6-sr
Version: 3.14.54
Release: 2
License: GPL-2.0
Group: System/Kernel
Source: kernel-3.14.y-fslc-imx6-sr_3.14.54pkg2.tar.gz

BuildRequires: bc lzop
BuildRequires: module-init-tools

Provides: kernel = %{version}-%{release}
Provides: kernel-uname-r = %{baseversion}%{localversion}
Requires: dtb-3.14.y-fslc-imx6-sr = %{version}

# helper for binary userspace
Provides: galcore = 5.0.11.25762

%description
This package provides the community-maintained 3.14 LTS Kernel for Freescale i.MX6 devices.

%package -n dtb-3.14.y-fslc-imx6-sr
Summary: DeviceTree Binaries
Group: System/Boot
Obsoletes: dtb-imx6
Conflicts: dtb-imx6
%description -n dtb-3.14.y-fslc-imx6-sr
This package contains the DeviceTree binaries for Freescale i.MX6 devices.

%package devel
Summary: Kernel development files
Group: Development/Languages/C and C++
Requires: %{name} = %{version}-%{release}
%description devel
This package provides the required development files to build modules for this kernel.

%package api-devel
Summary: Public Kernel API
Group: Development/Languages/C and C++
%description api-devel
This package provides the public kernel headers, to build userspace applications against this specific kernel.

%prep
%setup -q -n kernel-3.14.y-fslc-imx6-sr-3.14.54pkg2

# build in subdirectory, out-of-tree
mkdir build

# merge defautl defconfig with provided one
cd build; ../linux/scripts/kconfig/merge_config.sh -m ../linux/arch/arm/configs/imx_v7_cbi_hb_defconfig ../defconfig; cd ..

# set LOCALVERSION
cd build; ../linux/scripts/config --set-str LOCALVERSION %{localversion}; cd ..

# initialize build tree
make -C linux O="$PWD/build" olddefconfig

%build
cd build

# build all
%{__make} %{?_smp_mflags} zImage modules dtbs

# compress vmlinux
gzip -n -k -9 vmlinux

%install
cd build

# kernel
install -v -m644 -D arch/arm/boot/zImage %{buildroot}/boot/zImage-%{baseversion}%{localversion}
install -v -m644 -D .config %{buildroot}/boot/config-%{baseversion}%{localversion}
install -v -m644 -D System.map %{buildroot}/boot/System.map-%{baseversion}%{localversion}

# vmlinux.gz, for mkinitrd to be happy
install -v -m644 -D vmlinux.gz %{buildroot}/boot/vmlinux-%{baseversion}%{localversion}.gz

# modules
%{__make} INSTALL_MOD_PATH=%{buildroot} modules_install

# module dependencies
/sbin/depmod -a -b %{buildroot} -e %{baseversion}%{localversion}

# delete installed firmware for now
rm -rf %{buildroot}/lib/firmware

# TODO: correct/delete installed symlinks to source

# DeviceTree
install -v -m755 -d %{buildroot}/boot/dtb
install -v -m644 arch/arm/boot/dts/*-cubox-i.dtb %{buildroot}/boot/dtb/
install -v -m644 arch/arm/boot/dts/*-hummingboard.dtb %{buildroot}/boot/dtb/
install -v -m644 arch/arm/boot/dts/*-hummingboard2.dtb %{buildroot}/boot/dtb/

# initrd placeholder for ghost file
touch %{buildroot}/boot/initrd-%{baseversion}%{localversion}

# devel files
sh ../install_devel_files.sh arm %{buildroot}/usr/src/kernel-%{baseversion}%{localversion}

# source and build symlinks
ln -sfv /usr/src/kernel-%{baseversion}%{localversion} %{buildroot}/lib/modules/%{baseversion}%{localversion}/source
ln -sfv /usr/src/kernel-%{baseversion}%{localversion} %{buildroot}/lib/modules/%{baseversion}%{localversion}/build

# public API headers
make INSTALL_HDR_PATH=%{buildroot}/usr/include/kernel-%{baseversion}%{localversion} headers_install
mv %{buildroot}/usr/include/kernel-%{baseversion}%{localversion}/include/* %{buildroot}/usr/include/kernel-%{baseversion}%{localversion}/
rmdir %{buildroot}/usr/include/kernel-%{baseversion}%{localversion}/include
find %{buildroot}/usr/include/kernel-%{baseversion}%{localversion} -name '*.cmd' -delete

%post
if [ "x$1" = "x1" ]; then
	# this means this package wasn't installed before
	# create zImage and initrd symlinks if they dont exist
	# we dont want to overwrite another kernel
	if [ ! -e /boot/zImage ] && [ ! -e /boot/initrd ]; then
		ln -sv zImage-%{baseversion}%{localversion} /boot/zImage
		ln -sv initrd-%{baseversion}%{localversion} /boot/initrd
	fi
fi
if [ "$1" -gt  "1" ]; then
	# This means the package is beeing updated
	# the version may have changed and the boot file name with it
	# check if current symlinks point to -cubox-i
	# if they do, force an update on them

	# decision will be based on zImage only
	if [ -L "/boot/zImage" ] && [[ `readlink /boot/zImage` = zImage-*%{localversion} ]]; then
		ln -sfv zImage-%{baseversion}%{localversion} /boot/zImage
		ln -sfv initrd-%{baseversion}%{localversion} /boot/initrd
	fi
fi
%if 0%{?suse_version} == 1310
# HACK - enable fdt boot
if [ -f /boot/boot.script ] && [ -x /usr/bin/mkimage ]; then
	sed -i "s;setenv should_use_fdt '0';setenv should_use_fdt '1';g" /boot/boot.script
	sed -i "s;setenv should_load_fdt '0';setenv should_load_fdt '1';g" /boot/boot.script
	mkimage -A arm -O linux -T script -C none -a 0 -e 0 -d /boot/boot.script /boot/boot.scr
fi
%endif

# create initrd
# TODO: use /usr/lib/module-init-tools/weak-modules2
if [ -x "/sbin/mkinitrd" ]; then
	/sbin/mkinitrd -k /boot/zImage-%{baseversion}%{localversion} -i /boot/initrd-%{baseversion}%{localversion}
fi

%preun
if [ "x$1" = "x0" ]; then
	# this means this package is beeing uninstalled
	# delete symlinks to this kernel if any
	if [ -L "/boot/zImage" ] && [ "x`readlink /boot/zImage`" = "xzImage-%{baseversion}%{localversion}" ]; then
		rm /boot/zImage
	fi
	if [ -L "/boot/initrd" ] && [ "x`readlink /boot/initrd`" = "xinitrd-%{baseversion}%{localversion}" ]; then
		rm /boot/initrd
	fi
fi

%files
%defattr(-,root,root)
/boot/zImage-%{baseversion}%{localversion}
/boot/config-%{baseversion}%{localversion}
/boot/System.map-%{baseversion}%{localversion}
/boot/vmlinux-%{baseversion}%{localversion}.gz
%exclude /lib/modules/%{baseversion}%{localversion}/source
%exclude /lib/modules/%{baseversion}%{localversion}/build
/lib/modules/%{baseversion}%{localversion}
%ghost /boot/initrd-%{baseversion}%{localversion}

%files -n dtb-3.14.y-fslc-imx6-sr
%defattr(-,root,root)
/boot/dtb

%files devel
%defattr(-,root,root)
/usr/src/kernel-%{baseversion}%{localversion}
/lib/modules/%{baseversion}%{localversion}/source
/lib/modules/%{baseversion}%{localversion}/build

%files api-devel
%defattr(-,root,root)
/usr/include/kernel-%{baseversion}%{localversion}

%changelog
