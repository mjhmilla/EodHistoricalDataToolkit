#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT

EX="$1"
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -u ${EOD_FUNDAMENTAL_DATA} -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/"$EX".json -x "$EX" -g -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/updateGapsFundamentalData."$EX".log
cd ..

