---------------------------------------------------------------
The provided .spec files may be used to create EMGD RPM packages for 
installation on Fedora or MeeGo.

RPMs are an alternative to the install.sh Linux installer script,
though the installer script is recommended.
---------------------------------------------------------------


Copy the driver and the spec files to the target system
(the system where you will install the RPMs)

Create your RPM building environment
 mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
 echo "%_topdir $HOME/rpmbuild" >> ~/.rpmmacros
 echo "%_tmppath $HOME/rpmbuild/tmp" >> ~/.rpmmacros
 
Setup the Userspace Source
Create a folder named intel-emgd-<build version> and copy the userspace 
binaries for your Linux distribution. For Fedora 11:
 cp F11/driver/Xorg-xserver-1.6.4/* intel-emgd-<build version>
Compress the folder and copy to SOURCE.
 tar czvf intel-emgd-<build version>.tgz intel-emgd-<build version>
 cp intel-emgd-<build version>.tgz ~/rpmbuild/SOURCES/

Setup the Kernel Module Source
Create a folder named intel-emgd-kmod-<build version>. Copy the kernel module 
source by extracting emgd_drm.tgz and copying to intel-emgd-kmod-<build-version>
Compress the folder and copy to SOURCE.
 tar czvf intel-emgd-kmod-<build version>.tgz intel-emgd-kmod-<build version>
 cp intel-emgd-kmod-<build version>.tgz ~/rpmbuild/SOURCES/

Copy spec files to SPECS
 cp emgd.spec emgd-kmod.spec ~/rpmbuild/SPECS

**Note: To build emgd-kmod, you will need the kernel headers installed.
        These are usually packages named kernel-devel or something similar.

Build the RPMs
 cd ~/rpmbuild/SPECS
 rpmbuild -ba emgd.spec
 rpmbuild -ba emgd-kmod.spec



