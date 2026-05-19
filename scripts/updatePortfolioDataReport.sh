#!/usr/bin/env bash

EX="F"
HC="F"
TK="ABEA.F.json"
TKP="GOOGL.US.json"
PLOT_SUMMARY="plotSummary.json"
PLOT_OVERVIEW="plotOverview.json"

./fetchFundamentalDataOfSingleTicker.sh "$EX" "$TK"
./fetchHistoricalDataOfSingleTicker.sh "$EX" "$TK"
./calculateSingleTicker.sh "$EX" "$HC" "$TKP"
./generateSingleTickerReport.sh "$EX" "$PLOT_SUMMARY" "$PLOT_OVERVIEW" "$TKP"

EX="F"
HC="F"
TK="HEI.F.json"
TKP="HEI.XETRA.json"
PLOT_SUMMARY="plotSummary.json"
PLOT_OVERVIEW="plotOverview.json"

./fetchFundamentalDataOfSingleTicker.sh "$EX" "$TK"
./fetchHistoricalDataOfSingleTicker.sh "$EX" "$TK"
./calculateSingleTicker.sh "$EX" "$HC" "$TKP"
./generateSingleTickerReport.sh "$EX" "$PLOT_SUMMARY" "$PLOT_OVERVIEW" "$TKP"

EX="F"
HC="F"
TK="DLY.F.json"
TKP="LYB.US.json"
PLOT_SUMMARY="plotSummary.json"
PLOT_OVERVIEW="plotOverview.json"

./fetchFundamentalDataOfSingleTicker.sh "$EX" "$TK"
./fetchHistoricalDataOfSingleTicker.sh "$EX" "$TK"
./calculateSingleTicker.sh "$EX" "$HC" "$TKP"
./generateSingleTickerReport.sh "$EX" "$PLOT_SUMMARY" "$PLOT_OVERVIEW" "$TKP"

EX="F"
HC="F"
TK="ADM.F.json"
TKP="ADM.US.json"
PLOT_SUMMARY="plotSummary.json"
PLOT_OVERVIEW="plotOverview.json"

./fetchFundamentalDataOfSingleTicker.sh "$EX" "$TK"
./fetchHistoricalDataOfSingleTicker.sh "$EX" "$TK"
./calculateSingleTicker.sh "$EX" "$HC" "$TKP"
./generateSingleTickerReport.sh "$EX" "$PLOT_SUMMARY" "$PLOT_OVERVIEW" "$TKP"

EX="F"
HC="F"
TK="NPS.F.json"
TKP="NPS.F.json"
PLOT_SUMMARY="plotSummary.json"
PLOT_OVERVIEW="plotOverview.json"

./fetchFundamentalDataOfSingleTicker.sh "$EX" "$TK"
./fetchHistoricalDataOfSingleTicker.sh "$EX" "$TK"
./calculateSingleTicker.sh "$EX" "$HC" "$TKP"
./generateSingleTickerReport.sh "$EX" "$PLOT_SUMMARY" "$PLOT_OVERVIEW" "$TKP"



