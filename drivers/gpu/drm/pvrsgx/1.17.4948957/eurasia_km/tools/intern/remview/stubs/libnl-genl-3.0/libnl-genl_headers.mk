########################################################################### ###
#@File
#@Title         Libnl-genl stub library Makefile.
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Dual MIT/GPLv2
# 
# The contents of this file are subject to the MIT license as set out below.
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
# Alternatively, the contents of this file may be used under the terms of
# the GNU General Public License Version 2 ("GPL") in which case the provisions
# of GPL are applicable instead of those above.
# 
# If you wish to allow use of your version of this file only under the terms of
# GPL, and not to allow others to use your version of this file under the terms
# of the MIT license, indicate your decision by deleting the provisions above
# and replace them with the notice and other provisions required by GPL as set
# out in the file called "GPL-COPYING" included in this distribution. If you do
# not delete the provisions above, a recipient may use your version of this file
# under the terms of either the MIT license or GPL.
# 
# This License is also included in this distribution in the file called
# "MIT-COPYING".
# 
# EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
# PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
### ###########################################################################

libnl-genl_stubs_headers_src := \
					netlink/attr.h \
					netlink/errno.h \
					netlink/handlers.h \
					netlink/list.h \
					netlink/msg.h \
					netlink/netlink.h \
					netlink/socket.h \
					netlink/genl/ctrl.h \
					netlink/genl/genl.h \
					netlink/genl/mngt.h
libnl-genl_stubs_headers_dstdir := $(MODULE_OUT)/include
libnl-genl_stubs_headers_targets := $(foreach _h,$(libnl-genl_stubs_headers_src),$(libnl-genl_stubs_headers_dstdir)/$(_h))

$(libnl-genl_stubs_headers_dstdir)/netlink:
	$(call make-directory,$@)

$(libnl-genl_stubs_headers_dstdir)/netlink/genl:
	$(call make-directory,$@)

# It doesn't appear to be possible to use a pattern matching rule to
# copy the headers, as we need to generate the names of the prerequisites,
# rather than just match a pattern.
$(libnl-genl_stubs_headers_dstdir)/netlink/attr.h: $(THIS_DIR)/netlink/attr.h | $(libnl-genl_stubs_headers_dstdir)/netlink
	$(CP) -f $< $@

$(libnl-genl_stubs_headers_dstdir)/netlink/errno.h: $(THIS_DIR)/netlink/errno.h | $(libnl-genl_stubs_headers_dstdir)/netlink
	$(CP) -f $< $@

$(libnl-genl_stubs_headers_dstdir)/netlink/handlers.h: $(THIS_DIR)/netlink/handlers.h | $(libnl-genl_stubs_headers_dstdir)/netlink
	$(CP) -f $< $@

$(libnl-genl_stubs_headers_dstdir)/netlink/list.h: $(THIS_DIR)/netlink/list.h | $(libnl-genl_stubs_headers_dstdir)/netlink
	$(CP) -f $< $@

$(libnl-genl_stubs_headers_dstdir)/netlink/msg.h: $(THIS_DIR)/netlink/msg.h | $(libnl-genl_stubs_headers_dstdir)/netlink
	$(CP) -f $< $@

$(libnl-genl_stubs_headers_dstdir)/netlink/netlink.h: $(THIS_DIR)/netlink/netlink.h | $(libnl-genl_stubs_headers_dstdir)/netlink
	$(CP) -f $< $@

$(libnl-genl_stubs_headers_dstdir)/netlink/socket.h: $(THIS_DIR)/netlink/socket.h | $(libnl-genl_stubs_headers_dstdir)/netlink
	$(CP) -f $< $@

$(libnl-genl_stubs_headers_dstdir)/netlink/genl/ctrl.h: $(THIS_DIR)/netlink/genl/ctrl.h | $(libnl-genl_stubs_headers_dstdir)/netlink/genl
	$(CP) -f $< $@

$(libnl-genl_stubs_headers_dstdir)/netlink/genl/genl.h: $(THIS_DIR)/netlink/genl/genl.h | $(libnl-genl_stubs_headers_dstdir)/netlink/genl
	$(CP) -f $< $@

$(libnl-genl_stubs_headers_dstdir)/netlink/genl/mngt.h: $(THIS_DIR)/netlink/genl/mngt.h | $(libnl-genl_stubs_headers_dstdir)/netlink/genl
	$(CP) -f $< $@

.PHONY: libnl-genl_stubs_headers
libnl-genl_stubs_headers: $(libnl-genl_stubs_headers_targets)
