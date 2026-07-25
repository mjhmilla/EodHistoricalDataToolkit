#!/usr/bin/env bash


#"F"  
#"F"  
#"DEU"
#"LXS"
#"LXS"

#declare -a ex=(  "US"  "US"   "US"   "US"   "US"   "US"     "US"  "US"     "US"   "US"   "US"   "LSE"  "F" )
#declare -a pex=( "US"  "US"   "US"   "US"   "US"   "CO"     "US"  "US"     "US"   "US"   "US"   "LSE"  "XETRA")
#declare -a hc=(  "USA" "USA"  "USA"  "USA"  "USA"  "DNK"    "USA" "USA"    "USA"  "USA"  "USA"  "GBR"  "DEU")
#declare -a tkA=( "OWL" "DUOL" "PYPL" "TSLA" "OTLY" "NVO"    "LYB" "FI"     "CROX" "LULU" "CELH" "WOSG" "EVD")
#declare -a tkB=( "OWL" "DUOL" "PYPL" "TSLA" "OTLY" "NOVO-B" "LYB" "FI"     "CROX" "LULU" "CELH" "WOSG" "EVD")



#declare -a sl=("OWL" "DUOL" "OTLY" "WOSG")




scriptMode="$1"
echo "Mode: $1"
listName="$2"
echo "List Name: $2"
folderName="$3"
echo "Dest. Folder: $3"

count=1

while IFS=, read -r ex pex hc tkA tkB y
do

  echo "----------------------------------------"
  echo "$count. ${tkA}.${ex}"

  if [ $scriptMode == 0 ] ; then
    ./fetchFundamentalDataOfSingleTicker.sh  "${ex}"  "${tkA}.${ex}" 
    ./fetchHistoricalDataOfSingleTicker.sh  "${ex}"  "${tkA}.${ex}" 
  fi 

  if [ $scriptMode == 1 ] ; then
    if [ $y == 5 ] ; then
      ./calculateSingleTicker.sh "${ex}" "${hc}" "${tkB}.${pex}.json"
    fi
    if [ $y == 3 ] ; then 
      ./calculateSingleTickerOver3Years.sh "${ex}" "${hc}" "${tkB}.${pex}.json"
    fi 
  fi

  if [ $scriptMode == 2 ] ; then
    ./generateSingleTickerReport.sh "${ex}" generateTickerReports.json "${tkB}.${pex}".json
  fi 

  if [ $scriptMode == 3 ] ; then
    cd ${EOD_TOOLKIT_HOME}/data/"${ex}"/generateTickerReports/"${tkB}_${pex}"
    pdflatex report_"${tkB}_${pex}".tex
    pdflatex report_"${tkB}_${pex}".tex
  fi

  if [ $scriptMode == 4 ] ; then
    if [ ! -d ${EOD_TOOLKIT_HOME}/data/"${folderName}" ]; then 
      mkdir ${EOD_TOOLKIT_HOME}/data/"${folderName}"
    fi

    cp ${EOD_TOOLKIT_HOME}/data/"${ex}"/generateTickerReports/"${tkB}_${pex}"/report_"${tkB}_${pex}".pdf ${EOD_TOOLKIT_HOME}/data/"${folderName}"
  fi
  count=$((count+1))
done < "${listName}"
 
