#!/usr/bin/env bash
FREE_RTOS_SRC=.
PROJ_DIR=$HOME/workset/qemu_cortexM3_Eth/freertos
PORT_FOLDER=ARM_CM3
PORT_FOLDER_PATH=$FREE_RTOS_SRC/portable/GCC/$PORT_FOLDER


check_folder()
{
	local folder="$1"
if [ -d "${folder}" ]; then
	echo "${folder} exists"
else 
	echo "${folder} doesnt exsit"
fi

}


check_folder "${FREE_RTOS_SRC}"
check_folder "${PROJ_DIR}"
check_folder "${PORT_FOLDER_PATH}"

cp -vr $FREE_RTOS_SRC/*.c $PROJ_DIR/
cp -vr $FREE_RTOS_SRC/include/*.h $PROJ_DIR/include/

mkdir -p $PROJ_DIR/portable
cp -vr $PORT_FOLDER_PATH $PROJ_DIR/portable




