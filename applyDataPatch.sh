#!/usr/bin/env bash

EX="$1"

PF="$2"

cd build
./applyPatch -p ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PF" -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -x "$EX" -u ${EOD_FUNDAMENTAL_DATA} -k ${EOD_API_TOKEN} -i -v| tee ${EOD_TOOLKIT_HOME}/data/"$EX"/applyPatch."$EX".log
cd ..

