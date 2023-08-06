#!/usr/bin/env bash
EX="$1"
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -u ${EOD_FUNDAMENTAL_DATA} -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/"$EX".json -x "$EX" -g -v -s 19492 | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData."$EX".log
cd ..

