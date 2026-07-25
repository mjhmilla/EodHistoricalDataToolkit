declare -a arrFH=("fundamentalData" "historicalData")
declare -a arrT=("ABEA.F.json" "FB2A.F.json" "NVD.F.json" "TL0.F.json" "APC.F.json" "AMZ.F.json" "MSF.F.json")
declare -a arrH=("GOOGL.US.json" "META.US.json" "NVDA.US.json" "TSLA.US.json" "AAPL.US.json" "AMZN.US.json" "MSFT.US.json")

for i in "${arrFH[@]}"
do 
    echo "$i"
    for j in "${arrT[@]}"
    do 
        cp F/"$i"/"$j" F_test/"$i"/
    done
    for j in "${arrH[@]}"
    do 
        cp F/"$i"/"$j" F_test/"$i"/
    done
done

for j in "${arrH[@]}"
do 
    cp F/calculateData/"$j" F_test/calculateData/
done