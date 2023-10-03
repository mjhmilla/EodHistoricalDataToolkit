#!/usr/bin/env bash
EX="$1"
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/exchange-symbol-list/ -u ${EOD_TICKERS} -x "$EX" -k ${EOD_API_TOKEN} -l ${EOD_TOOLKIT_HOME}/data/exchanges.json
cd ..

