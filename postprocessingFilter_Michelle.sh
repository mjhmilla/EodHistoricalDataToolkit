#!/usr/bin/env bash

EX="STU"
PLOT="plotByValueRCashFlowExcessRoic.csv"
RANK="rankByValueRCashFlowExcessRoic_report.json"
FILTER="filterByTicker_Guardian.json"
cd build
./postprocess -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PLOT" -r ${EOD_TOOLKIT_HOME}/data/"$EX"/preprocess/"$RANK" -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -a ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -s ${EOD_TOOLKIT_HOME}/data/"$EX"/"$FILTER" -t report_Michelle_Guardian.tex -n -1 -y 2022 -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocessing_Michelle_Guardian."$EX".log
cd ..

cd ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/
pdflatex report_Michelle_Guardian.tex
cd ${EOD_TOOLKIT_HOME}

EX="STU"
PLOT="plotByValueRCashFlowExcessRoic.csv"
RANK="rankByValueRCashFlowExcessRoic_report.json"
FILTER="filterByTicker_WS.json"
cd build
./postprocess -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PLOT" -r ${EOD_TOOLKIT_HOME}/data/"$EX"/preprocess/"$RANK" -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -a ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -s ${EOD_TOOLKIT_HOME}/data/"$EX"/"$FILTER" -t report_Michelle_WS.tex -n -1 -y 2022 -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocessing_Michelle_WS."$EX".log
cd ..

cd ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/
pdflatex report_Michelle_WS.tex
cd ${EOD_TOOLKIT_HOME}


EX="STU"
PLOT="plotByValueRCashFlowExcessRoic.csv"
RANK="rankByValueRCashFlowExcessRoic_report.json"
FILTER="filterByTicker_Pyr.json"
cd build
./postprocess -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PLOT" -r ${EOD_TOOLKIT_HOME}/data/"$EX"/preprocess/"$RANK" -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -a ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -s ${EOD_TOOLKIT_HOME}/data/"$EX"/"$FILTER" -t report_Michelle_Pyr.tex -n -1 -y 2022 -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocessing_Michelle_Pyr."$EX".log
cd ..

cd ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/
pdflatex report_Michelle_Pyr.tex
cd ${EOD_TOOLKIT_HOME}


EX="STU"
PLOT="plotByValueRCashFlowExcessRoic.csv"
RANK="rankByValueRCashFlowExcessRoic_report.json"
FILTER="filterByTicker_AGF.json"
cd build
./postprocess -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PLOT" -r ${EOD_TOOLKIT_HOME}/data/"$EX"/preprocess/"$RANK" -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -a ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -s ${EOD_TOOLKIT_HOME}/data/"$EX"/"$FILTER" -t report_Michelle_AGF.tex -n -1 -y 2022 -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocessing_Michelle_AGF."$EX".log
cd ..

cd ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/
pdflatex report_Michelle_AGF.tex
cd ${EOD_TOOLKIT_HOME}


