#! /bin/bash

DEV_ID=0
CHANNELS=1

FILE_PATH=../data/video/
FILE_LIST=

for((i=0;i<${CHANNELS};i++))
do
	file="sample_720p.h264,"
	FILE_LIST=${FILE_LIST}${FILE_PATH}${file}
done
#echo ${FILE_LIST}

LOGDIR=./log
if [ ! -d ${LOGDIR} ]; then mkdir -p ${LOGDIR}; fi

./dech264_to_rgb 		-devID=${DEV_ID}			\
							-channels=${CHANNELS}		\
							-fileList=${FILE_LIST}		\
							-endlessLoop=0
							#2>&1 | tee ./log/log.txt

