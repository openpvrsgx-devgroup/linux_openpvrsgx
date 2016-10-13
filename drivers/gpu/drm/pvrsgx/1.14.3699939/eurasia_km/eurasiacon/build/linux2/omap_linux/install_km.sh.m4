########################################################################### ###
##@Title         Target install script
##@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
##@License       Strictly Confidential.
### ###########################################################################

STANDARD_HEADER
INTERPRETER

##	Set up variables.
##
ifelse(SUPPORT_DRI_DRM,1,,[[
DISPLAY_CONTROLLER(DISPLAY_KERNEL_MODULE)
]])

BUFFER_CLASS_DEVICE(bc_example)


## Configure how to create install_km.sh
##
STANDARD_KERNEL_MODULES
