#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT

cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/ -u ${EOD_EXCHANGES} -k ${EOD_API_TOKEN} -n exchanges.json
cd ..
