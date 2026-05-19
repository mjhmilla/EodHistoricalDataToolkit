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



