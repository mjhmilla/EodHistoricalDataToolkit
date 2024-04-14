
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>

#include <chrono>
#include "date.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"

struct TickerMetricData{
  std::vector< date::sys_days > dates;
  std::string ticker;
  std::vector< std::vector< double > > metrics;
};

struct MetricTable{
  date::sys_days dateStart;
  date::sys_days dateEnd;
  std::vector< std::string > tickers;
  std::vector< std::vector< double > > metrics;
  std::vector< std::vector< size_t > > metricRank;
  std::vector< size_t > metricRankSum;
  std::vector< size_t > rank;
};

//Fun example to get the ranked index of an entry
//https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes

template <typename T>
std::vector< size_t > rank(const std::vector< T > &v, bool sortAscending){
  std::vector< size_t > idx(v.size());
  std::iota(idx.begin(),idx.end(),0);
  if(sortAscending){
    std::stable_sort(idx.begin(),idx.end(),
                    [&v](size_t i1, size_t i2){return v[i1] < v[i2];}); 
  }else{
    std::stable_sort(idx.begin(),idx.end(),
                    [&v](size_t i1, size_t i2){return v[i1] > v[i2];}); 
  }
  return idx;
} 



//==============================================================================
int calcDifferenceInDaysBetweenTwoDates(const std::string &dateA,
                                        const char* dateAFormat,
                                        const std::string &dateB,
                                        const char* dateBFormat){

  std::istringstream dateStream(dateA);
  dateStream.exceptions(std::ios::failbit);
  date::sys_days daysA;
  dateStream >> date::parse(dateAFormat,daysA);

  dateStream.clear();
  dateStream.str(dateB);
  date::sys_days daysB;
  dateStream >> date::parse(dateBFormat,daysB);

  int daysDifference = (daysA-daysB).count();

  return daysDifference;

};

//==============================================================================
int calcIndexOfClosestDateInHistorcalData(const std::string &targetDate,
                                const char* targetDateFormat,
                                nlohmann::ordered_json &historicalData,
                                const char* dateSetFormat,
                                bool verbose){


  int indexA = 0;
  int indexB = historicalData.size()-1;

  int indexAError = calcDifferenceInDaysBetweenTwoDates(targetDate,
                targetDateFormat, historicalData[indexA]["date"],dateSetFormat);
  int indexBError = calcDifferenceInDaysBetweenTwoDates(targetDate,
                targetDateFormat, historicalData[indexB]["date"],dateSetFormat);
  int index      = std::round((indexB+indexA)*0.5);
  int indexError = 0;
  int changeInError = historicalData.size()-1;

  while( std::abs(indexB-indexA)>1 
      && std::abs(indexAError)>0 
      && std::abs(indexBError)>0
      && changeInError > 0){

    int indexError = calcDifferenceInDaysBetweenTwoDates(targetDate,
            targetDateFormat, historicalData[index]["date"],dateSetFormat);

    if( indexError*indexAError >= 0){
      indexA = index;
      changeInError = std::abs(indexAError-indexError);
      indexAError=indexError;
    }else if(indexError*indexBError > 0){
      indexB = index;
      changeInError = std::abs(indexBError-indexError);
      indexBError=indexError;
    }
    if(std::abs(indexB-indexA) > 1){
      index      = std::round((indexB+indexA)*0.5);
    }
  }

  if(std::abs(indexAError) <= std::abs(indexBError)){
    return indexA;
  }else{
    return indexB;
  }


};




//==============================================================================
void writeMetricTableToCsvFile(
      std::vector< MetricTable > &metricTableSet,
      std::vector< std::vector< std::string> > &listOfRankingMetrics,
      std::string &fundamentalFolder,
      std::string &historicalFolder,
      std::string &rankFolderOutput,
      std::string &outputFileName,
      bool verbose){

  std::string rankingResultFilePath = rankFolderOutput;
  rankingResultFilePath.append(outputFileName);

  std::ofstream rankingFile;
  rankingFile.open(rankingResultFilePath);

  if(verbose){
    std::cout<<"Writing Metric Tables to csv" << std::endl;
  }

  for(auto const &tableEntry: metricTableSet){
    date::year_month_day ymdStart = tableEntry.dateStart;
    date::year_month_day ymdEnd = tableEntry.dateEnd;

    rankingFile << ymdStart.year()<<"-"<<ymdStart.month()<<"-"<<ymdStart.day()
                <<'\n';
    rankingFile << ymdEnd.year()<<"-"<<ymdEnd.month()<<"-"<<ymdEnd.day()
                <<'\n';

    rankingFile <<"Ticker"<<","<<"ranking"<<","<<"metricRankSum";
    for(size_t j=0; j<listOfRankingMetrics.size();++j){
      rankingFile << "," << listOfRankingMetrics[j][0]<<"_rank";
      rankingFile << "," << listOfRankingMetrics[j][0]<<"_value";          
    }
    rankingFile << ","<<"Country"
                << ","<<"Name"
                << ","<<"PercentInsiders"
                << ","<<"PercentInstitutions"
                << ","<<"MarketCapitalizationMln"
                << ","<<"IPODate"                
                << ","<<"Price"
                << ","<<"TargetPrice"
                << ","<<"StrongBuy"
                << ","<<"Buy"
                << ","<<"Hold"
                << ","<<"Sell"
                << ","<<"StrongSell"
                << ","<<"URL";
    rankingFile <<'\n';

    if(verbose){
      std::cout << ymdStart.year()<<"-"<<ymdStart.month()<<"-"<<ymdStart.day()
                << " to "
                << ymdEnd.year()<<"-"<<ymdEnd.month()<<"-"<<ymdEnd.day()
                << std::endl;
    }

    for(size_t i=0; i<tableEntry.tickers.size();++i){

      if(verbose){
        std::cout << '\t' << i << '\t' << tableEntry.tickers[i] << std::endl;
      }

      rankingFile     <<      tableEntry.tickers[i];
      rankingFile     <<","<< tableEntry.rank[i];
      rankingFile     <<","<< tableEntry.metricRankSum[i];
      for(size_t j=0; j<tableEntry.metrics[i].size();++j){
        rankingFile   <<","<<tableEntry.metricRank[i][j];
        rankingFile   <<","<<tableEntry.metrics[i][j];
      }

      //Add additional context data
      
      nlohmann::ordered_json fundamentalData;  
      bool loadedFundData =JsonFunctions::loadJsonFile(tableEntry.tickers[i], 
                                fundamentalFolder, fundamentalData, verbose);        

      nlohmann::ordered_json historicalData;
      bool loadedHistData =JsonFunctions::loadJsonFile(tableEntry.tickers[i], 
                                historicalFolder, historicalData, verbose); 
                                


      bool addContext= (loadedFundData && loadedHistData);        

      if(addContext){

        //Get the stock price at the end of the analysis period
        int lowestDateError = std::numeric_limits< int >::max();
        std::stringstream ss;
        ss << static_cast<int>(ymdEnd.year())
           <<"-"
           << static_cast<unsigned>(ymdEnd.month())
           <<"-"
           << static_cast<unsigned>(ymdEnd.day());
        std::string endDate = ss.str();

        int indexClosest= calcIndexOfClosestDateInHistorcalData(endDate,"%Y-%m-%d",
                            historicalData,"%Y-%m-%d",verbose);
        double endPrice  = JsonFunctions::getJsonFloat(
                              historicalData[indexClosest]["adjusted_close"]);        

        std::string tempString;
        JsonFunctions::getJsonString(
            fundamentalData["General"]["AddressData"]["Country"],
            tempString);            
        rankingFile << "," << tempString;
        JsonFunctions::getJsonString(
            fundamentalData["General"]["Name"],
            tempString);            
        rankingFile << "," << tempString;            
        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["SharesStats"]["PercentInsiders"]);            
        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["SharesStats"]["PercentInstitutions"]);            
        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["Highlights"]["MarketCapitalizationMln"]);      
        JsonFunctions::getJsonString(
            fundamentalData["General"]["IPODate"],
            tempString);       
        rankingFile << "," << tempString;  
        rankingFile << "," << endPrice;
        rankingFile << "," << JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["TargetPrice"]);
        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["StrongBuy"]); 
        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["Buy"]); 
        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["Hold"]); 
        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["Sell"]); 
        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["StrongSell"]); 
        JsonFunctions::getJsonString(
            fundamentalData["General"]["WebURL"],
            tempString);            
        rankingFile << "," << tempString;
      }
      rankingFile <<'\n';
    }
    rankingFile <<'\n';
  }
  rankingFile.close();
}


//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string analysisFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string rankingFilePath;
  std::string reportFolder;
  bool analyzeYears=true;
  bool analyzeQuarters=false;
  bool ignoreNegativeValues=true; 
  int maxDifferenceInDays = 15; //a half month at most.

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will produce reports in the form of text "
    "and tables about the companies that are best ranked."
    ,' ', "0.0");

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");
    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> reportFolderOutput("o","report_folder_path", 
      "The path to the folder will contain the reports.",
      true,"","string");
    cmd.add(reportFolderOutput);

    TCLAP::ValueArg<std::string> analysisFolderInput("a","analysis_folder_path", 
      "The path to the folder contains the analysis json files "
      "produced by the calculate method",
      true,"","string");
    cmd.add(analysisFolderInput);

    TCLAP::ValueArg<std::string> rankingFilePathInput("r","rank_file_path", 
      "The path to the ranking file.",
      true,"","string");
    cmd.add(rankingFilePathInput);


    TCLAP::ValueArg<std::string> fundamentalFolderInput("f",
      "fundamental_data_folder_path", 
      "The path to the folder that contains the fundamental data json files from "
      "https://eodhistoricaldata.com/ to analyze",
      true,"","string");
    cmd.add(fundamentalFolderInput);

    TCLAP::ValueArg<std::string> historicalFolderInput("p",
      "historical_data_folder_path", 
      "The path to the folder that contains the historical (price)"
      " data json files from https://eodhistoricaldata.com/ to analyze",
      true,"","string");
    cmd.add(historicalFolderInput);

    TCLAP::SwitchArg quarterlyAnalysisInput("q","quarterly",
      "Analyze quarterly data. Caution: this is not yet been tested.", false);
    cmd.add(quarterlyAnalysisInput);    



    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode          = exchangeCodeInput.getValue();    
    analysisFolder        = analysisFolderInput.getValue();
    fundamentalFolder     = fundamentalFolderInput.getValue();
    historicalFolder      = historicalFolderInput.getValue();    
    rankingFilePath       = rankingFilePathInput.getValue();
    reportFolder          = reportFolderOutput.getValue();
    analyzeQuarters       = quarterlyAnalysisInput.getValue();
    analyzeYears          = !analyzeQuarters;

    verbose             = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Analysis Input Folder" << std::endl;
      std::cout << "    " << analysisFolder << std::endl;

      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;

      std::cout << "  Ranking File Path" << std::endl;
      std::cout << "    " << rankingFilePath << std::endl;

      std::cout << "  Report Folder" << std::endl;
      std::cout << "    " << reportFolder << std::endl;

      std::cout << "  Analyze Quarters (untested)" << std::endl;
      std::cout << "    " << analyzeYears << std::endl;

      if(analyzeQuarters){
        std::cout << "Exiting: analyze quarters has not been tested "
                  << std::endl;
        std::abort();                  
      }
    }


  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  return 0;
}
