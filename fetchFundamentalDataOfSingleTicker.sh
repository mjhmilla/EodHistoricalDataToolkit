#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT

EX="$1"
TK="$2"

#rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/

#mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"
#mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/



cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -i "$TK" -u ${EOD_FUNDAMENTAL_DATA} -d ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/"$EX".json -x "$EX" -g -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData."$EX".log
cd ..

