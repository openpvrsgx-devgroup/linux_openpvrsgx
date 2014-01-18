#----------------------------------------------------------------------------
# Copyright (c) 2002-2010, Intel Corporation.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#----------------------------------------------------------------------------

%define debug_package %{nil}
%define kernel_version  %(uname -r)
%define modpath /lib/modules/%{kernel_version}/kernel/drivers/gpu/drm/emgd

Name: intel-emgd-kmod
Summary: Intel EMGD kernel module
Version: 3398
Release: 1%{?dist}
License: GPL v2
Vendor: Intel
Group: System Environment/Kernel
BuildRoot: %{_tmppath}/%{name}-%{version}
Source0: %{name}-%{version}.tgz
Requires: /boot/vmlinuz-%{kernel_version}, /sbin/depmod


%description
Intel EMGD kernel module for kernel %{kernel_version}

%prep
%setup -q

%build
make

%install
install -m 755 -d $RPM_BUILD_ROOT%{modpath}
install -m 744 emgd.ko $RPM_BUILD_ROOT%{modpath}


%clean  
rm -Rf $RPM_BUILD_ROOT

%post 
/sbin/depmod -av %{kernel_version} >/dev/null 2>&1 

%postun 
/sbin/depmod -av %{kernel_version} >/dev/null 2>&1 

%files 
%defattr(-,root,root,-)
%{modpath}/emgd.ko

