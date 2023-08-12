#!/usr/bin/env bash

EX="$1"

rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/

mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"
mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/




cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -u ${EOD_FUNDAMENTAL_DATA} -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/"$EX".json -x "$EX" -g -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData."$EX".log
cd ..

