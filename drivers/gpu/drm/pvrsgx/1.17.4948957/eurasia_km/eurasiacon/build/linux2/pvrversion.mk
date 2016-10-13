########################################################################### ###
#@Title         Extract info from pvrversion.h
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Strictly Confidential.
### ###########################################################################

# Version information
PVRVERSION_H	:= $(TOP)/include4/pvrversion.h

# scripts.mk uses these to set the install script's version suffix
PVRVERSION_MAJ        := $(shell perl -ne '/\sPVRVERSION_MAJ\s+(\w+)/          and print $$1' $(PVRVERSION_H))
PVRVERSION_MIN        := $(shell perl -ne '/\sPVRVERSION_MIN\s+(\w+)/          and print $$1' $(PVRVERSION_H))
PVRVERSION_FAMILY     := $(shell perl -ne '/\sPVRVERSION_FAMILY\s+"(\S+)"/     and print $$1' $(PVRVERSION_H))
PVRVERSION_BRANCHNAME := $(shell perl -ne '/\sPVRVERSION_BRANCHNAME\s+"(\S+)"/ and print $$1' $(PVRVERSION_H))
PVRVERSION_BUILD      := $(shell perl -ne '/\sPVRVERSION_BUILD\s+(\w+)/        and print $$1' $(PVRVERSION_H))

PVRVERSION := "${PVRVERSION_FAMILY}_${PVRVERSION_BRANCHNAME}\@${PVRVERSION_BUILD}"
