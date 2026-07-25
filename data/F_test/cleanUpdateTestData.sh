#!/usr/bin/env bash

hex="$1"
echo "Home Exchange: $1" 

listName="$2"
echo "List Name: $2"

rm -rf calculateData/*
rm -rf historicalData/*
rm -rf fundamentalData/*
rm -rf generateScreenerData/*
rm -rf generateTickerReports/*

echo "----------------------------------------"
count=0

while IFS=, read -r ticker ex
do
  echo "$count. ${ticker}.${ex}"

  cp ${EOD_TOOLKIT_HOME}/data/"${hex}"/calculateData/"${ticker}.${ex}".json calculateData/
  cp ${EOD_TOOLKIT_HOME}/data/"${hex}"/fundamentalData/"${ticker}.${ex}".json fundamentalData/
  cp ${EOD_TOOLKIT_HOME}/data/"${hex}"/historicalData/"${ticker}.${ex}".json historicalData/  
  cp -a ${EOD_TOOLKIT_HOME}/data/"${hex}"/generateTickerReports/"${ticker}_${ex}" generateTickerReports/

  count=$((count+1))

done < "${listName}"