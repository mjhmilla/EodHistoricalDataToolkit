#!/usr/bin/env bash

nameOfList="tickerListAtilla.csv"

while IFS=, read -r ex pex hc tkA tkB y
do
    echo "$ex|$pex|$hc|$tkA|$tkB|$y"
done < "${nameOfList}"