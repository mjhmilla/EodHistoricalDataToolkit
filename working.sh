./calculate.sh STU
./rank.sh STU rankByValueRCashFlowExcessRoic.csv
./preprocessing.sh STU rankByValueRCashFlowExcessRoic.csv rankByValueRCashFlowExcessRoic_ranking.json
./postprocessingFilter.sh STU plotByValueRCashFlowExcessRoic.csv rankByValueRCashFlowExcessRoic_report.json filterByTicker_MagnificientSeven.json
cd data/STU/postprocess
pdflatex reportFilter.tex 
cd ../../..