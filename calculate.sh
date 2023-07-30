#!/usr/bin/env bash
cd build
./calculate -f ${EOD_TOOLKIT_HOME}/data/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/historicalData/ -x STU -o ${EOD_TOOLKIT_HOME}/data/analysisData/ -v
cd ..
