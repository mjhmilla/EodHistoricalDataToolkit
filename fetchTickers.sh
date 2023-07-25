#!/usr/bin/env bash
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/ -u ${EOD_TICKERS} -x STU -k ${EOD_API_TOKEN} -n STU.json
cd ..

