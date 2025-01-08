#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT

EX="$1"

cd build
./scan -l ${EOD_TOOLKIT_HOME}/data/exchanges.json -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/ -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/scan."$EX".log
cd ..


