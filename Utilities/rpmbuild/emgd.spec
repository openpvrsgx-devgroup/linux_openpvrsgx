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

#Just for now until the libs are ran using gcc rather than ld.
#This causes the omission of --build-id which causes debuginfo 
#generation to puke.
%define debug_package %{nil}
%define libversion 1.5.15.3226

Name: intel-emgd
Summary: Intel EMGD graphics libraries
Version: 3398
Release: 1.6%{?dist}
License: Intel Proprietary
Vendor: Intel
Group: System Environment/Libraries
BuildRoot: %{_tmppath}/%{name}-%{version}-build

Source0: %{name}-%{version}.tgz

%description
EMGD runtime graphics libraries 


%package -n xorg-x11-drv-intel-emgd
Summary: Xorg X11 EMGD video driver
Group: User Interface/X Hardware Support
Requires: xorg-x11-server-Xorg, libdrm
%description -n xorg-x11-drv-intel-emgd
X based EMGD

%package -n intel-emgd-driver-extras
Summary: EMGD VA, GLES, OVG, EGL
Group: System Environment/Libraries
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Provides: intel-emgd-driver-extras = %{version}-%{release}
Requires: mesa-dri-intel-emgd-driver = %{version}-%{release}
Requires: xorg-x11-drv-intel-emgd = %{version}-%{release}
%description -n intel-emgd-driver-extras
EMGD extra APIs such as video decode (VA), OpenGL ES, OpenVG, and EGL

%package -n mesa-dri-intel-emgd-driver
Summary: Mesa-based DRI drivers
Group: User Interface/X Hardware Support
Provides: mesa-dri-intel-emgd-driver = %{version}-%{release}
Requires: libdrm
Requires: xorg-x11-drv-intel-emgd = %{version}-%{release}
%description -n mesa-dri-intel-emgd-driver
Mesa-based EMGD DRI driver.

%prep
%setup -q

%build

%install
rm -Rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/etc/X11
mkdir -p $RPM_BUILD_ROOT%{_libdir}/xorg/modules/drivers/
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man4/

install -m 755 emgd_drv.so $RPM_BUILD_ROOT%{_libdir}/xorg/modules/drivers/emgd_drv.so
install -m 755 emgd_drv_video.so $RPM_BUILD_ROOT%{_libdir}/xorg/modules/drivers/emgd_drv_video.so

install emgd.4.gz $RPM_BUILD_ROOT%{_mandir}/man4/
#install -m 644 xorg-emgd.conf $RPM_BUILD_ROOT/etc/X11/xorg.conf


mkdir -p $RPM_BUILD_ROOT/%{_libdir}/dri/

install -m 644 emgd_dri.so $RPM_BUILD_ROOT%{_libdir}/dri/emgd_dri.so

install -m 644 libEMGD2d.so $RPM_BUILD_ROOT%{_libdir}/libEMGD2d.so.%{libversion}
install -m 644 libGLES_CM.so $RPM_BUILD_ROOT%{_libdir}/libGLES_CM.so.%{libversion}
install -m 644 libGLESv2.so $RPM_BUILD_ROOT%{_libdir}/libGLESv2.so.%{libversion}
install -m 644 libOpenVG.so $RPM_BUILD_ROOT%{_libdir}/libOpenVG.so.%{libversion}
install -m 644 libOpenVGU.so $RPM_BUILD_ROOT%{_libdir}/libOpenVGU.so.%{libversion}
install -m 644 libEMGDegl.so $RPM_BUILD_ROOT%{_libdir}/libEMGDegl.so.%{libversion}
install -m 644 libEGL.so $RPM_BUILD_ROOT%{_libdir}/libEGL.so.%{libversion}
install -m 644 libEMGDOGL.so $RPM_BUILD_ROOT%{_libdir}/libEMGDOGL.so.%{libversion}
install -m 644 libEMGDScopeServices.so $RPM_BUILD_ROOT%{_libdir}/libEMGDScopeServices.so.%{libversion}
install -m 644 libemgdsrv_init.so $RPM_BUILD_ROOT%{_libdir}/libemgdsrv_init.so.%{libversion}
install -m 644 libemgdsrv_um.so $RPM_BUILD_ROOT%{_libdir}/libemgdsrv_um.so.%{libversion}
install -m 644 libemgdglslcompiler.so $RPM_BUILD_ROOT%{_libdir}/libemgdglslcompiler.so.%{libversion}


pushd $RPM_BUILD_ROOT%{_libdir}
ln -s libEMGD2d.so.%{libversion} libEMGD2d.so
ln -s libGLES_CM.so.%{libversion} libGLES_CM.so
ln -s libGLESv2.so.%{libversion} libGLESv2.so
ln -s libOpenVG.so.%{libversion} libOpenVG.so
ln -s libOpenVGU.so.%{libversion} libOpenVGU.so
ln -s libEMGDegl.so.%{libversion} libEMGDegl.so
ln -s libEGL.so.%{libversion} libEGL.so
ln -s libEMGDOGL.so.%{libversion} libEMGDOGL.so
ln -s libEMGDScopeServices.so.%{libversion} libEMGDScopeServices.so
ln -s libemgdsrv_init.so.%{libversion} libemgdsrv_init.so
ln -s libemgdsrv_um.so.%{libversion} libemgdsrv_um.so
ln -s libemgdglslcompiler.so.%{libversion} libemgdglslcompiler.so
popd



%clean  
rm -Rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files -n xorg-x11-drv-intel-emgd
%defattr(-,root,root,-)
%{_libdir}/xorg/modules/drivers/emgd_drv.so
%{_libdir}/libEMGD2d.so
%{_libdir}/libEMGD2d.so.%{libversion}
%{_libdir}/libEMGDOGL.so
%{_libdir}/libEMGDOGL.so.%{libversion}
%{_libdir}/libEMGDScopeServices.so
%{_libdir}/libEMGDScopeServices.so.%{libversion}
%{_libdir}/libemgdsrv_um.so
%{_libdir}/libemgdsrv_um.so.%{libversion}
%{_libdir}/libemgdsrv_init.so
%{_libdir}/libemgdsrv_init.so.%{libversion}
%{_libdir}/libemgdglslcompiler.so
%{_libdir}/libemgdglslcompiler.so.%{libversion}
%doc %{_mandir}/man4/emgd*


%files -n intel-emgd-driver-extras
%defattr(-,root,root,-)
%{_libdir}/xorg/modules/drivers/emgd_drv_video.so
%{_libdir}/libGLES_CM.so
%{_libdir}/libGLES_CM.so.%{libversion}
%{_libdir}/libGLESv2.so
%{_libdir}/libGLESv2.so.%{libversion}
%{_libdir}/libOpenVG.so
%{_libdir}/libOpenVG.so.%{libversion}
%{_libdir}/libOpenVGU.so
%{_libdir}/libOpenVGU.so.%{libversion}
%{_libdir}/libEMGDegl.so
%{_libdir}/libEMGDegl.so.%{libversion}
%{_libdir}/libEGL.so
%{_libdir}/libEGL.so.%{libversion}

%files -n mesa-dri-intel-emgd-driver
%defattr(-,root,root,-)
%{_libdir}/dri/emgd_dri.so

