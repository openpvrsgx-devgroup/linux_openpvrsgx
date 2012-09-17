FW_PATH=./fw/.libs
TASK_PATH=./tasks/.libs
LINUX_PATH=~/source/linux.git

if [ $# -lt 1 ]; then
	echo Usage: $0 version 
	echo Usage: where version is FW version e.g. 009530
	exit 127
fi

FW_VERSION=$FW_PATH/fw$1.so
TASK_VERSION=$TASK_PATH/abe$1.so

./src/abegen \
	-f $FW_VERSION \
	-t $TASK_VERSION

