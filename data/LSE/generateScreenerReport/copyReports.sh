#!/usr/bin/env bash
declare -a arr=("screen_filter_TownPriest" "screen_filter_TownPriest_10_Energy" "screen_filter_TownPriest_15_Materials" "screen_filter_TownPriest_20_Industrials" "screen_filter_TownPriest_25_ConsumerDiscretionary" "screen_filter_TownPriest_30_ConsumerStaples" "screen_filter_TownPriest_35_HeathCare" "screen_filter_TownPriest_40_Financials" "screen_filter_TownPriest_45_InformationTechnology" "screen_filter_TownPriest_50_CommunicationServices" "screen_filter_TownPriest_55_Utilities" "screen_filter_TownPriest_60_RealEstate"
)



for i in "${arr[@]}"
do
    cd "$i"
    
    #From 
    #https://stackoverflow.com/questions/67416769/how-to-loop-through-all-files-of-a-specified-type-in-a-given-directory-in-bash

    ## create an array containing file names
    files=( $(find . -mindepth 1 -type f -iname "*.pdf" | sort ) )

    for file in "${files[@]}"; do
        cp "${file}" ../reports        
    done

    cd ..
done



