#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT


cd ${EOD_TOOLKIT_HOME}/build
./fetch -f ${EOD_TOOLKIT_HOME}/data/forex/ -u ${EOD_FOREX} -k ${EOD_API_TOKEN} -r ${EOD_TOOLKIT_HOME}/config/forex.csv -g -v



