#!/usr/bin/env bash


#"F"  
#"F"  
#"DEU"
#"LXS"
#"LXS"

declare -a ex=(  "US"  "US"   "US"   "US"   "US"   "US"     "US"  "US"     "US"   "US"   "US"   "LSE"  "F" )
declare -a pex=( "US"  "US"   "US"   "US"   "US"   "CO"     "US"  "US"     "US"   "US"   "US"   "LSE"  "XETRA")
declare -a hc=(  "USA" "USA"  "USA"  "USA"  "USA"  "DNK"    "USA" "USA"    "USA"  "USA"  "USA"  "GBR"  "DEU")
declare -a tkA=( "OWL" "DUOL" "PYPL" "TSLA" "OTLY" "NVO"    "LYB" "FI"     "CROX" "LULU" "CELH" "WOSG" "EVD")
declare -a tkB=( "OWL" "DUOL" "PYPL" "TSLA" "OTLY" "NOVO-B" "LYB" "FI"     "CROX" "LULU" "CELH" "WOSG" "EVD")



declare -a sl=("OWL" "DUOL" "OTLY" "WOSG")




scriptMode="$1"
echo "$1"

for i in "${!ex[@]}"
do
  echo "${tk[i]}.${ex[i]}".json
  useDefaultCalc=1
  for j in "${!sl[@]}"
  do
    if [ "${sl[j]}" = "${tkA[i]}" ] ; then
      useDefaultCalc=0
    fi
  done

  echo "----------------------------------------"
  echo "${tkA[i]}.${ex[i]}"
  echo "----------------------------------------"

  if [ $scriptMode == 0 ] ; then
    ./fetchFundamentalDataOfSingleTicker.sh  "${ex[i]}"  "${tkA[i]}.${ex[i]}" 
    ./fetchHistoricalDataOfSingleTicker.sh  "${ex[i]}"  "${tkA[i]}.${ex[i]}" 
  fi 

  if [ $scriptMode == 1 ] ; then
    if [ $useDefaultCalc == 1 ] ; then
      ./calculateSingleTicker.sh "${ex[i]}" "${hc[i]}" "${tkB[i]}.${pex[i]}.json"
    else 
      ./calculateSingleTickerOver3Years.sh "${ex[i]}" "${hc[i]}" "${tkB[i]}.${pex[i]}.json"
    fi 
  fi

  if [ $scriptMode == 2 ] ; then
    ./generateSingleTickerReport.sh "${ex[i]}" generateTickerReports.json "${tkB[i]}.${pex[i]}".json
  fi 

  if [ $scriptMode == 3 ] ; then
    cd ${EOD_TOOLKIT_HOME}/data/"${ex[i]}"/generateTickerReports/"${tkB[i]}_${pex[i]}"
    pdflatex report_"${tkB[i]}_${pex[i]}".tex
    pdflatex report_"${tkB[i]}_${pex[i]}".tex
  fi

  if [ $scriptMode == 4 ] ; then
    cp ${EOD_TOOLKIT_HOME}/data/"${ex[i]}"/generateTickerReports/"${tkB[i]}_${pex[i]}"/report_"${tkB[i]}_${pex[i]}".pdf ${EOD_TOOLKIT_HOME}/data/"${ex[i]}"/tickerList
  fi
done

 
