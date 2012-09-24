LOCAL_SO_PATH=./plugins/omap4plus
COEFF_PATH=$LOCAL_SO_PATH/coeffs/.libs
DAPM_PATH=$LOCAL_SO_PATH/dapm/.libs
LINUX_PATH=~/source/linux.git
ABE_GEN_PATH=../abefw.git

#	-vfw $LINUX_PATH/firmware/omap4_abe

./src/socfw omap4_abe_new \
	-p $COEFF_PATH/dl1.so \
	-p $COEFF_PATH/dl2l.so \
	-p $COEFF_PATH/dl2r.so \
	-p $COEFF_PATH/sdt.so \
	-p $COEFF_PATH/dmic.so \
	-p $COEFF_PATH/amic.so \
	-p $DAPM_PATH/abe-dl1dl2-twl6040.so \
	-vfw $ABE_GEN_PATH/omap_abe_fw \
	-vcn $ABE_GEN_PATH/omap_abe_map
