declare -a fldr=("screen_filter_TownPriest" "screen_filter_TownPriest_10_Energy" "screen_filter_TownPriest_15_Materials" "screen_filter_TownPriest_20_Industrials" "screen_filter_TownPriest_25_ConsumerDiscretionary" "screen_filter_TownPriest_30_ConsumerStaples" "screen_filter_TownPriest_35_HeathCare" "screen_filter_TownPriest_40_Financials" "screen_filter_TownPriest_45_InformationTechnology" "screen_filter_TownPriest_50_CommunicationServices" "screen_filter_TownPriest_55_Utilities" "screen_filter_TownPriest_60_RealEstate")


for i in "${fldr[@]}"
do 
    echo "$i"
    mkdir "$i"
done

