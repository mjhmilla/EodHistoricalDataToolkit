#!/usr/bin/env bash
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/ -u ${EOD_EXCHANGES} -k ${EOD_API_TOKEN} -n exchanges.json
cd ..
