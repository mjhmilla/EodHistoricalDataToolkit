
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>
#include <filesystem>

#include <chrono>
#include "date.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"
#include "UtilityFunctions.h"



//==============================================================================
void writeMetricTableSortedByTickerToTextFile(
      nlohmann::ordered_json &metricTableSet,
      std::vector< std::vector< std::string> > &listOfRankingMetrics,
      std::string &reportFolderOutput,
      std::string &outputFileName,
      int numberOfIntervalsToAnalyze,
      bool verbose){

  std::string reportFilePath = reportFolderOutput;
  reportFilePath.append(outputFileName);

  std::ofstream reportFile;
  reportFile.open(reportFilePath);

  if(verbose){
    std::cout<<"Writing report to text" << std::endl;
  }

  unsigned int indexEntry=0;
  for(auto const &tableEntry: metricTableSet){

    //
    //Read in the starting and ending dates
    //
    std::string dateStringA;
    JsonFunctions::getJsonString(tableEntry[0]["dateStart"],dateStringA);
    std::istringstream dateSStreamA(dateStringA);
    dateSStreamA.exceptions(std::ios::failbit);
    date::year_month_day ymdStart;
    dateSStreamA >> date::parse("%Y-%m-%d",ymdStart);

    std::string dateStringB;
    JsonFunctions::getJsonString(tableEntry[0]["dateEnd"],dateStringB);
    std::istringstream dateSStreamB(dateStringB);
    dateSStreamB.exceptions(std::ios::failbit);
    date::year_month_day ymdEnd;
    dateSStreamB >> date::parse("%Y-%m-%d",ymdEnd);

    if(verbose){
      std::cout << ymdStart.year()<<"-"<<ymdStart.month()<<"-"<<ymdStart.day()
                << " to "
                << ymdEnd.year()<<"-"<<ymdEnd.month()<<"-"<<ymdEnd.day()
                << std::endl;
    }

    for(size_t i=1; i<tableEntry.size();++i){

      std::string ticker;
      JsonFunctions::getJsonString(tableEntry[i]["PrimaryTicker"],ticker);
      
      int ranking = static_cast<int>(
                      JsonFunctions::getJsonFloat(tableEntry[i]["Ranking"]));

      int metricRankSum = static_cast<int>(
                      JsonFunctions::getJsonFloat(tableEntry[i]["MetricRankSum"]));

      reportFile << i << '\t' << ticker << std::endl << std::endl;

      //Write the summary table header
      int colW=12;
      int colS=8;
      std::string emptyCol(""),emptyColBar(""),emptyColAlign("");
      while(emptyCol.size()<colW){
        emptyCol.append(" ");
      }
      while(emptyColBar.size()<(colS-1)){
        emptyColBar.append(" ");  
        emptyColAlign.append(" ");
      }
      emptyColBar.append("|");


      reportFile  << emptyCol << emptyCol << emptyColAlign << 
                     "Ranking"   << std::endl;
      reportFile  << emptyCol << emptyCol << emptyColBar << emptyColAlign
                  << "MetricRankSum" << std::endl;

      if(verbose){
        std::cout << '\t' << i << '\t' << ticker << std::endl;
      }

      for(size_t j=0; j<listOfRankingMetrics.size();++j){
        std::string metricName = listOfRankingMetrics[j][0];

        std::string metricNameValue = metricName;
        metricNameValue.append("_value");
        std::string metricNameRank = metricName;
        metricNameRank.append("_rank");
        
        reportFile << emptyCol << emptyCol ;
        for(size_t k=0; k < (2*j+2); ++k){
          reportFile << emptyColBar;
        }
        reportFile << emptyColAlign << metricNameRank << std::endl;
        reportFile << emptyCol << emptyCol ;
        for(size_t k=0; k < (2*j+3); ++k){
          reportFile << emptyColBar;
        }
        reportFile << emptyColAlign << metricNameValue << std::endl;
      }

      //Now go through the entire metric table and report all data on this
      //ticker
      for(auto const &iterTable: metricTableSet){
        for(size_t idxTbl=1; idxTbl<iterTable.size();++idxTbl){
          std::string iterTicker;
          JsonFunctions::getJsonString(iterTable[idxTbl]["PrimaryTicker"],
                                      iterTicker);
          //Append the data                                          
          if(ticker.compare(iterTicker)== 0){
            std::string dateStart;
            JsonFunctions::getJsonString(iterTable[0]["dateStart"],dateStart);
            std::string dateEnd;
            JsonFunctions::getJsonString(iterTable[0]["dateEnd"],dateEnd);
            reportFile << std::setw(colW) << std::setfill(' ') << dateStart;
            reportFile << std::setw(colW) << std::setfill(' ') << dateEnd;
            
            int ranking = static_cast<int>(
              JsonFunctions::getJsonFloat(iterTable[idxTbl]["Ranking"]));

            int metricRankSum = static_cast<int>(
              JsonFunctions::getJsonFloat(iterTable[idxTbl]["MetricRankSum"]));                

            reportFile << std::setw(colS)<<std::setfill(' ') << ranking;
            reportFile << std::setw(colS)<<std::setfill(' ') << metricRankSum;

            for(size_t j=0; j<listOfRankingMetrics.size();++j){
              std::string metricName = listOfRankingMetrics[j][0];

              std::string metricNameValue = metricName;
              metricNameValue.append("_value");
              std::string metricNameRank = metricName;
              metricNameRank.append("_rank");

              int metricRank = static_cast<int>(
                JsonFunctions::getJsonFloat(iterTable[idxTbl][metricNameRank]));
              double metricValue =
                JsonFunctions::getJsonFloat(iterTable[idxTbl][metricNameValue]);

              reportFile << std::setw(colS)<<std::setfill(' ') << metricRank;
              reportFile << std::setw(colS)<<std::setfill(' ') 
                         << std::setprecision(2)
                         << metricValue;

            }
            reportFile << std::endl;

          }                                          
        }
      }
                              
      //bool addContext= (loadedFundData && loadedHistData && loadedCalculateData);  
    }


    ++indexEntry;
    if(numberOfIntervalsToAnalyze > 0 
      && indexEntry >= numberOfIntervalsToAnalyze){
      break;
    }
  }
  reportFile.close();

};
//==============================================================================
//This function writes data on each company to file sorted by period and by
//the overall rank to two files: a human readable csv file, and a machine
//readable json file.
void writeReportTableToFile(
      nlohmann::ordered_json &metricTableSet,
      std::vector< std::vector< std::string> > &listOfRankingMetrics,
      std::string &fundamentalFolder,
      std::string &historicalFolder,
      std::string &calculateDataFolder,
      std::string &reportFolderOutput,
      std::string &outputFileNameWithoutExtension,
      int numberOfIntervalsToAnalyze,
      bool verbose){

  std::string outputFileNameJson = outputFileNameWithoutExtension;
  outputFileNameJson.append(".json");
  std::string rankingResultFilePathJson = reportFolderOutput;
  rankingResultFilePathJson.append(outputFileNameJson);

  nlohmann::ordered_json jsonReport;

    
  std::string outputFileNameCsv = outputFileNameWithoutExtension;
  outputFileNameCsv.append(".csv");
  std::string rankingResultFilePathCsv = reportFolderOutput;
  rankingResultFilePathCsv.append(outputFileNameCsv);

  std::ofstream rankingFile;
  rankingFile.open(rankingResultFilePathCsv);

  if(verbose){
    std::cout<<"Writing Metric Tables to csv" << std::endl;
  }

  unsigned int indexEntry=0;
  for(auto const &tableEntry: metricTableSet){
    //
    //Read in the starting and ending dates
    //
    std::string dateStringA;
    JsonFunctions::getJsonString(tableEntry[0]["dateStart"],dateStringA);
    std::istringstream dateSStreamA(dateStringA);
    dateSStreamA.exceptions(std::ios::failbit);
    date::year_month_day ymdStart;
    dateSStreamA >> date::parse("%Y-%m-%d",ymdStart);


    std::string dateStringB;
    JsonFunctions::getJsonString(tableEntry[0]["dateEnd"],dateStringB);
    std::istringstream dateSStreamB(dateStringB);
    dateSStreamB.exceptions(std::ios::failbit);
    date::year_month_day ymdEnd;
    dateSStreamB >> date::parse("%Y-%m-%d",ymdEnd);


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
                << ","<<"ProducingExcess"
                << ","<<"ReturnOnInvestedCapital"
                << ","<<"CostOfCapital"
                << ","<<"PercentInsiders"
                << ","<<"PercentInstitutions"
                << ","<<"MarketCapitalizationMln"
                << ","<<"Beta"
                << ","<<"IPODate"                
                << ","<<"Price"
                << ","<<"TargetPrice"
                << ","<<"StrongBuy"
                << ","<<"Buy"
                << ","<<"Hold"
                << ","<<"Sell"
                << ","<<"StrongSell"
                << ","<<"URL"
                << ","<<"International/Domestic"
                << ","<<"Sector"
                << ","<<"GicSector"
                << ","<<"GicGroup"
                << ","<<"GicIndustry"
                << ","<<"GicSubIndustry"
                << ","<<"HomeCategory";
    rankingFile <<'\n';

    if(verbose){
      std::cout << ymdStart.year()<<"-"<<ymdStart.month()<<"-"<<ymdStart.day()
                << " to "
                << ymdEnd.year()<<"-"<<ymdEnd.month()<<"-"<<ymdEnd.day()
                << std::endl;
    }

    for(size_t i=1; i<tableEntry.size();++i){

      nlohmann::ordered_json jsonTickerEntry=nlohmann::ordered_json::object();
      jsonTickerEntry.push_back({"dateStart",dateStringA});
      jsonTickerEntry.push_back({"dateEnd",dateStringB});

      std::string ticker;
      JsonFunctions::getJsonString(tableEntry[i]["PrimaryTicker"],ticker);
      int ranking = static_cast<int>(
                      JsonFunctions::getJsonFloat(tableEntry[i]["Ranking"]));

      int metricRankSum = static_cast<int>(
                      JsonFunctions::getJsonFloat(tableEntry[i]["MetricRankSum"]));

      bool here=false;
      if(ticker.compare("MBI.STU")==0){
        here=true;
      }

      rankingFile << ticker;
      rankingFile << "," << ranking;
      rankingFile << "," << metricRankSum;

      jsonTickerEntry.push_back({"PrimaryTicker",ticker});
      jsonTickerEntry.push_back({"Ranking",ranking});
      jsonTickerEntry.push_back({"MetricRankSum",metricRankSum});

      if(verbose){
        std::cout << '\t' << i << '\t' << ticker << std::endl;
      }

      for(size_t j=0; j<listOfRankingMetrics.size();++j){
        std::string metricName = listOfRankingMetrics[j][0];

        std::string metricNameValue = metricName;
        metricNameValue.append("_value");
        std::string metricNameRank = metricName;
        metricNameRank.append("_rank");
        
        int metricRank = static_cast<int>(
                  JsonFunctions::getJsonFloat(tableEntry[i][metricNameRank]));
        double metricValue =
                  JsonFunctions::getJsonFloat(tableEntry[i][metricNameValue]);

        rankingFile   <<","<< metricRank;
        rankingFile   <<","<< metricValue;

        jsonTickerEntry.push_back({metricNameRank,metricRank});
        jsonTickerEntry.push_back({metricNameValue,metricValue});        
      }

      //Add additional context data
      
      nlohmann::ordered_json fundamentalData;  
      bool loadedFundData =JsonFunctions::loadJsonFile(ticker, 
                                fundamentalFolder, fundamentalData, verbose);        

      nlohmann::ordered_json historicalData;
      bool loadedHistData =JsonFunctions::loadJsonFile(ticker, 
                                historicalFolder, historicalData, verbose); 
                                
      nlohmann::ordered_json calculateData;
      bool loadedCalculateData =JsonFunctions::loadJsonFile(ticker, 
                                calculateDataFolder, calculateData, verbose); 


      bool addContext= (loadedFundData && loadedHistData && loadedCalculateData);        

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

        int indexClosest= 
          FinancialAnalysisToolkit::calcIndexOfClosestDateInHistorcalData(
            endDate,"%Y-%m-%d",historicalData,"%Y-%m-%d",verbose);
            
        double endPrice  = JsonFunctions::getJsonFloat(
                              historicalData[indexClosest]["adjusted_close"]);        

        //Get the index of the closest analysis period. There are not many
        date::sys_days dayStart(ymdStart);
        date::sys_days dayEnd(ymdEnd);

        std::string calculateDate("");
        bool calculateItemFound=false;
        auto calculateItem = calculateData.begin();

        do{          
                    
          calculateDate=calculateItem.key();
          std::stringstream calculateDateStream(calculateDate);
          date::sys_days day;
          calculateDateStream >> date::parse("%Y-%m-%d",day);
          int dayErrorStart = (day-dayStart).count();
          int dayErrorEnd = (day-dayEnd).count();

          if(dayErrorEnd*dayErrorStart <= 0){
            calculateItemFound=true;
          }else{
            ++calculateItem;              
          }
        }while( !calculateItemFound && calculateItem != calculateData.end());



        //Write the data to file
        std::string tempString;
        JsonFunctions::getJsonString(
            fundamentalData[GEN]["AddressData"]["Country"],
            tempString);            
        rankingFile << "," << tempString;
        
        jsonTickerEntry.push_back({"Country",tempString});

        JsonFunctions::getJsonString(
            fundamentalData[GEN]["CurrencyCode"],tempString);            
        rankingFile << "," << tempString;
        
        jsonTickerEntry.push_back({"CurrencyCode",tempString});


        JsonFunctions::getJsonString(
            fundamentalData[GEN]["Name"],
            tempString);            
        rankingFile << "," << tempString; 

        jsonTickerEntry.push_back({"Name",tempString});


        double roic = JsonFunctions::getJsonFloat(
            calculateData[calculateDate]["returnOnInvestedCapital"]);
        double costOfCapital = JsonFunctions::getJsonFloat(
            calculateData[calculateDate]["costOfCapital"]);
        
        if(!std::isnan(roic) && !std::isnan(costOfCapital)){
          if(roic > costOfCapital){
            rankingFile << ","<<"Yes";
          }else{
            rankingFile << ","<<"No";
          }
        }else{
          rankingFile <<"," << ",";
        }
        rankingFile <<","<<roic;
        rankingFile <<","<<costOfCapital;

        jsonTickerEntry.push_back({"returnOnInvestedCapital",roic});
        jsonTickerEntry.push_back({"costOfCapital",costOfCapital});


        rankingFile << "," << JsonFunctions::getJsonFloat(
            fundamentalData["SharesStats"]["PercentInsiders"]); 

        jsonTickerEntry.push_back({"PercentInsiders",
            JsonFunctions::getJsonFloat(
                fundamentalData["SharesStats"]["PercentInsiders"])});
    

        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["SharesStats"]["PercentInstitutions"]); 

        jsonTickerEntry.push_back({"PercentInstitutions",
            JsonFunctions::getJsonFloat(
                fundamentalData["SharesStats"]["PercentInstitutions"])});            

        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["Highlights"]["MarketCapitalizationMln"]); 

        jsonTickerEntry.push_back({"MarketCapitalizationMln",
            JsonFunctions::getJsonFloat(
            fundamentalData["Highlights"]["MarketCapitalizationMln"])});              

        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData[TECH]["Beta"]); 

        jsonTickerEntry.push_back({"Beta",
            JsonFunctions::getJsonFloat(
            fundamentalData[TECH]["Beta"])});


        JsonFunctions::getJsonString(
            fundamentalData[GEN]["IPODate"],
            tempString);     
        rankingFile << "," << tempString;  
        jsonTickerEntry.push_back({"IPODate",tempString});   
        
        JsonFunctions::getJsonString(
          fundamentalData[GEN]["International/Domestic"],
          tempString);
        rankingFile << "," << tempString;
        jsonTickerEntry.push_back({"International/Domestic",tempString});

        JsonFunctions::getJsonString(
          fundamentalData[GEN]["Sector"],
          tempString);
        rankingFile << "," << tempString;
        jsonTickerEntry.push_back({"Sector",tempString});

        JsonFunctions::getJsonString(
          fundamentalData[GEN]["GicSector"],
          tempString);
        rankingFile << "," << tempString;
        jsonTickerEntry.push_back({"GicSector",tempString});

        JsonFunctions::getJsonString(
          fundamentalData[GEN]["GicGroup"],
          tempString);
        rankingFile << "," << tempString;
        jsonTickerEntry.push_back({"GicGroup",tempString});

        JsonFunctions::getJsonString(
          fundamentalData[GEN]["GicIndustry"],
          tempString);
        rankingFile << "," << tempString;
        jsonTickerEntry.push_back({"GicIndustry",tempString});

        JsonFunctions::getJsonString(
          fundamentalData[GEN]["GicSubIndustry"],
          tempString);
        rankingFile << "," << tempString;
        jsonTickerEntry.push_back({"GicSubIndustry",tempString});

        JsonFunctions::getJsonString(
          fundamentalData[GEN]["HomeCategory"],
          tempString);
        rankingFile << "," << tempString;
        jsonTickerEntry.push_back({"HomeCategory",tempString});

        rankingFile << "," << endPrice;
        jsonTickerEntry.push_back({"adjusted_close",endPrice});   


        rankingFile << "," << JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["TargetPrice"]);
        
        jsonTickerEntry.push_back({"AnalystRatings_TargetPrice",
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["TargetPrice"])});


        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["StrongBuy"]); 

        jsonTickerEntry.push_back({"AnalystRatings_StrongBuy",
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["StrongBuy"])});


        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["Buy"]); 

        jsonTickerEntry.push_back({"AnalystRatings_Buy",
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["Buy"])});


        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["Hold"]); 

        jsonTickerEntry.push_back({"AnalystRatings_Hold",
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["Hold"])});

        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["Sell"]); 

        jsonTickerEntry.push_back({"AnalystRatings_Sell",
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["Sell"])});


        rankingFile << "," << 
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["StrongSell"]); 

        jsonTickerEntry.push_back({"AnalystRatings_StrongSell",
            JsonFunctions::getJsonFloat(
            fundamentalData["AnalystRatings"]["StrongSell"])});

        JsonFunctions::getJsonString(
            fundamentalData[GEN]["WebURL"],
            tempString);           

        rankingFile << "," << tempString;

        jsonTickerEntry.push_back({"WebURL",tempString});

        JsonFunctions::getJsonString(
            fundamentalData[GEN]["Description"],
            tempString);

        jsonTickerEntry.push_back({"Description",tempString});


      }
      rankingFile <<'\n';
      jsonReport.push_back(jsonTickerEntry);
    }
    rankingFile <<'\n';
    ++indexEntry;
    if(numberOfIntervalsToAnalyze > 0 
      && indexEntry >= numberOfIntervalsToAnalyze){
      break;
    }
  }

  rankingFile.close();

  std::ofstream outputFileStream(rankingResultFilePathJson,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << jsonReport;
  outputFileStream.close();  

};


//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string calculateDataFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string rankingConfigurationFile;  
  std::string rankingFile;
  std::string reportFolder;
  bool analyzeYears=true;
  bool analyzeQuarters=false;
  bool ignoreNegativeValues=true; 
  int maxDifferenceInDays = 15; //a half month at most.
  int numberOfYearsToReport = 2;

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

    TCLAP::ValueArg<std::string> calculateDataFolderInput("a","calculate_folder_path", 
      "The path to the folder contains the output "
      "produced by the calculate method",
      true,"","string");
    cmd.add(calculateDataFolderInput);

    TCLAP::ValueArg<std::string> rankingFileInput("r","rank_file_path", 
      "The path to the ranking file.",
      true,"","string");
    cmd.add(rankingFileInput);

    TCLAP::ValueArg<std::string> rankingConfigurationFileInput("c",
      "ranking_configuration_file_path", 
      "The path to the ranking configuration file.",
      true,"","string");
    cmd.add(rankingConfigurationFileInput);

    TCLAP::ValueArg<int> numberOfYearsToReportInput("n",
      "number_of_years_to_report_on", 
      "The path to the ranking configuration file.",
      false,2,"int");
    cmd.add(numberOfYearsToReportInput);    

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
      "Analyze quarterly data (TTM).", false);
    cmd.add(quarterlyAnalysisInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode          = exchangeCodeInput.getValue();    
    calculateDataFolder        = calculateDataFolderInput.getValue();
    fundamentalFolder     = fundamentalFolderInput.getValue();
    historicalFolder      = historicalFolderInput.getValue();    
    rankingFile           = rankingFileInput.getValue();
    rankingConfigurationFile 
                          = rankingConfigurationFileInput.getValue();
    reportFolder          = reportFolderOutput.getValue();
    analyzeQuarters       = quarterlyAnalysisInput.getValue();
    analyzeYears          = !analyzeQuarters;

    numberOfYearsToReport = numberOfYearsToReportInput.getValue();

    verbose             = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Calculate Data Input Folder" << std::endl;
      std::cout << "    " << calculateDataFolder << std::endl;

      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;

      std::cout << "  Ranking File Path" << std::endl;
      std::cout << "    " << rankingFile << std::endl;

      std::cout << "  Ranking Configuration File" << std::endl;
      std::cout << "    " << rankingConfigurationFile  << std::endl;

      std::cout << "  Report Folder" << std::endl;
      std::cout << "    " << reportFolder << std::endl;
      
      std::cout << "  Number of years to report" << std::endl;
      std::cout << "    " << numberOfYearsToReport << std::endl;

      std::cout << "  Analyze Quarters (untested)" << std::endl;
      std::cout << "    " << analyzeQuarters << std::endl;


    }

    int numberOfIntervalsToAnalyze = numberOfYearsToReport;
    if(analyzeQuarters){
      numberOfIntervalsToAnalyze *= 4;
    }

    //Load the metric table data set
    nlohmann::ordered_json metricTableSet;
    bool metricTableLoaded = 
      JsonFunctions::loadJsonFile(rankingFile, metricTableSet,verbose);
                                  
    //Load the list of ranking metrics
    std::vector < std::vector < std::string >> listOfRankingMetrics;
    bool listOfRankingMetricsLoaded = 
      UtilityFunctions::readListOfRankingMetrics(rankingConfigurationFile,
                                               listOfRankingMetrics, verbose);


    //File name
    std::filesystem::path rankingConfigFilePath = rankingConfigurationFile;
    std::string rankingConfigFileName = rankingConfigFilePath.filename().string();

    std::string fileNameWithoutExtension = rankingConfigFileName;
    fileNameWithoutExtension = 
      fileNameWithoutExtension.substr(0,fileNameWithoutExtension.length()-4);                                               
    fileNameWithoutExtension.append("_report");

    writeReportTableToFile(metricTableSet,listOfRankingMetrics,
      fundamentalFolder,historicalFolder,calculateDataFolder,reportFolder,
      fileNameWithoutExtension,numberOfIntervalsToAnalyze,verbose);

    //std::string reportFileName = rankingConfigFileName;
    //reportFileName = reportFileName.substr(0,reportFileName.length()-4);                                               
    //reportFileName.append("_sortedByTicker.txt");

    //This file is enormous. Skipping for now
    //writeMetricTableSortedByTickerToTextFile(metricTableSet,listOfRankingMetrics,
    //  reportFolder,reportFileName,numberOfIntervalsToAnalyze,verbose);  

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  return 0;
}
