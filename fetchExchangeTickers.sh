#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT

EX="$1"
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/ -u ${EOD_TICKERS} -x "$EX" -k ${EOD_API_TOKEN} -n "$EX".json
cd ..

