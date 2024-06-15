
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
#include "UtilityFunctions.h"


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
bool readMetricData(std::string &analysisFolder, 
              std::vector< std::vector< std::string > > &listOfRankingMetrics,
              std::vector< FinancialAnalysisToolkit::TickerMetricData > &tickerMetricDataSet,
              date::year_month_day &youngestDate,
              date::year_month_day &oldestDate,
              bool ignoreNegativeValues,
              bool verbose){

  bool inputsAreValid=true;
  int validFileCount=0;
  int totalFileCount=0;      
  std::string analysisExt = ".json";

  youngestDate = date::year{1900}/1/1;
  auto today = date::floor<date::days>(std::chrono::system_clock::now());
  oldestDate = date::year_month_day(today);

  std::vector< int > monthOfRecord;
  monthOfRecord.resize(12);
  if(verbose){
    std::cout << std::endl;
    std::cout << "Reading in metric values." << std::endl;
    std::cout << '\t' << "# entries" << '\t' << "Valid Files" << std::endl;
    std::cout <<  '\t' << '\t' << '\t' << "Invalid Files" << std::endl;
  }      

  for (const auto & file 
        : std::filesystem::directory_iterator(analysisFolder)){  

    ++totalFileCount;
    bool validInput=true;

    //
    // Check the file name
    //    
    std::string fileName   = file.path().filename(); 
    std::size_t fileExtPos = fileName.find(analysisExt);
    std::string ticker;

    if( fileExtPos == std::string::npos ){
      validInput=false;
      if(verbose){
        std::cout <<"Error file name should end in .json but this does not:"
                  <<fileName << std::endl; 
      }
    }else{
      ticker = fileName.substr(0,fileExtPos);
    }

    //
    //Read in the file
    //
    nlohmann::ordered_json analysisData;
    bool loadedAnalysisData =JsonFunctions::loadJsonFile(fileName,
                                        analysisFolder, analysisData, verbose);     
    
    //
    //Populate the Metric table
    //        
    FinancialAnalysisToolkit::TickerMetricData tickerMetricDataEntry;
    tickerMetricDataEntry.ticker = ticker;

    std::vector< double > metricData;
    std::vector< std::string > dateString;
    metricData.resize(listOfRankingMetrics.size());

    //Load the date metric vector                 
    if(validInput){
      
      for(auto& el: analysisData.items()){

        date::year_month_day ymdEntry;
        std::istringstream dateStream(el.key());
        dateStream >> date::parse("%Y-%m-%d",ymdEntry);

        bool validData = true;
        for(int i=0; i < listOfRankingMetrics.size();++i){ 
          //std::cout << el.key() << " " << listOfRankingMetrics[i][0] << std::endl;             
          metricData[i]=JsonFunctions::getJsonFloat(
              analysisData[el.key()][listOfRankingMetrics[i][0]]);
          if( std::isnan(metricData[i])){
            validData = false;
          }else if(metricData[i] < 0 && ignoreNegativeValues){
            validData = false;
          }
        }
        if(validData){
          dateString.push_back(el.key());
          tickerMetricDataEntry.dates.push_back(date::sys_days(ymdEntry));
          tickerMetricDataEntry.metrics.push_back(metricData);

          date::year_month_day ymd(ymdEntry);
          unsigned int indexMonth=unsigned{ymd.month()};
          --indexMonth;
          ++monthOfRecord[indexMonth];

          if(ymdEntry < oldestDate){
            oldestDate=ymdEntry;
          }
          if(ymdEntry > youngestDate){
            youngestDate=ymdEntry;
          }
          
        }
      }

      if(tickerMetricDataEntry.metrics.size() > 0){
        tickerMetricDataSet.push_back(tickerMetricDataEntry);
        ++validFileCount;
        if(verbose){
          std::cout << totalFileCount 
                    << '\t' << tickerMetricDataEntry.metrics.size()
                    << '\t' << fileName << std::endl;
        }
      }else{
        if(verbose){
          std::cout << totalFileCount 
                    << '\t' << '\t' << '\t' << fileName << std::endl;
        }
      }
    }
  }


  if(verbose){
    std::cout << std::endl;
    std::cout << validFileCount << "/" << totalFileCount
              << " analysis files contain valid data"
              << std::endl;
    std::cout << std::endl;
    std::cout << "Month of reporting" << std::endl;
    for(unsigned int i=0; i<monthOfRecord.size(); ++i){
      std::cout << (i+1) << '\t' << monthOfRecord[i] << std::endl;
    }       

    std::cout << std::endl;
    std::cout << "Oldest Date" << std::endl;
    std::cout << oldestDate.year() << "-" << oldestDate.month() << "-" 
              << oldestDate.day() << std::endl;

    std::cout << "Youngest Date" << std::endl;
    std::cout << youngestDate.year() << "-" << youngestDate.month() << "-" 
              << youngestDate.day() << std::endl;
    std::cout << std::endl;

  }

  if(tickerMetricDataSet.size() == 0){
    inputsAreValid=false;
  }

  return inputsAreValid;

}

//==============================================================================
void createMetricTable( 
      std::vector< FinancialAnalysisToolkit::TickerMetricData > &tickerMetricDataSet,
      std::vector< FinancialAnalysisToolkit::MetricTable > &metricTableSet,
      date::year_month_day &youngestDate,
      date::year_month_day &oldestDate,
      unsigned int referenceMonth,
      bool analyzeQuarters, 
      bool verbose){

  FinancialAnalysisToolkit::MetricTable metricTableEntry;      

  unsigned int durationMonths = 12;
  if(analyzeQuarters){
    durationMonths = 3; 
  }

  date::year_month_day periodStart = 
    youngestDate.year()/date::month(referenceMonth)/1;
  date::year_month_day periodEnd = periodStart;
  periodEnd = periodEnd + date::months(durationMonths);

  //make sure that the first period contains the youngest date
  while(periodEnd < youngestDate){
    periodStart += date::months(durationMonths);
    periodEnd = periodStart;
    periodEnd += date::months(durationMonths);
  }

  date::sys_days dayStart(periodStart);
  date::sys_days dayEnd(periodEnd);
  date::sys_days dayOldest(oldestDate);

  //Build the set of analysis periods;
  while( dayStart > dayOldest ){
    
    metricTableEntry.dateStart=dayStart;

    //Subtract off 1 day from the end so that the periods are non-overlapping
    metricTableEntry.dateEnd  = dayEnd;        
    metricTableSet.push_back(metricTableEntry);

    dayEnd     = dayStart-date::days(1);
    periodStart= dayStart;
    periodStart= periodStart - date::months(durationMonths);
    dayStart   = periodStart; 
  }
  
  //Scan through the TickerMetricDataSet and put each entry into
  //the correct analysis period
  for(auto &tickerEntry : tickerMetricDataSet){

    for(size_t indexTicker =0; 
                indexTicker< tickerEntry.dates.size(); 
                ++indexTicker){

      bool found=false;
      size_t indexTable=0;         
      while( !found && indexTable < metricTableSet.size()){
        if(  tickerEntry.dates[indexTicker] >= metricTableSet[indexTable].dateStart 
          && tickerEntry.dates[indexTicker] <= metricTableSet[indexTable].dateEnd){

          metricTableSet[indexTable].tickers.push_back(tickerEntry.ticker);
          std::vector< double > metric;
          for(size_t indexMetric=0; 
            indexMetric < tickerEntry.metrics[indexTicker].size(); 
            ++indexMetric){
            metric.push_back(tickerEntry.metrics[indexTicker][indexMetric]);
          }
          metricTableSet[indexTable].metrics.push_back(metric);
          found=true;
        }          
        ++indexTable;
      }
    }
  }  

}

//==============================================================================
void sortMetricTable(std::vector< FinancialAnalysisToolkit::MetricTable > &metricTableSet,
     std::vector< std::vector < std::string > > &listOfRankingMetrics){

  //Evaluate each metric ranking
  for(auto &tableEntry : metricTableSet){
    
    tableEntry.metricRank.resize(tableEntry.metrics.size());
    tableEntry.metricRankSum.resize(tableEntry.metrics.size());

    for(size_t i =0; i<tableEntry.metricRank.size();++i){
      tableEntry.metricRank[i].resize(tableEntry.metrics[i].size());
    }

    //Extract the jth column for sorting. There is not an elegant
    //way to do this, so I'm just copying it over.
    std::vector< double > column;
    column.resize(tableEntry.metrics.size());
    for(size_t j=0; j < tableEntry.metrics[0].size(); ++j){
      for(size_t i =0; i < tableEntry.metrics.size(); ++i){
        column[i] = tableEntry.metrics[i][j];
      }
      //Get the sorted indices 
      bool sortAscending=true;
      if(listOfRankingMetrics[j][1].compare("biggestIsBest")==0){
        sortAscending=false;
      }
      std::vector< size_t > columnRank = rank(column,sortAscending);

      for(size_t i =0; i < tableEntry.metrics.size(); ++i){
        tableEntry.metricRank[i][j] = columnRank[i];
      }
    }
    //Evaluate the combined ranking
    tableEntry.metricRankSum.resize(tableEntry.metrics.size());
    std::fill(tableEntry.metricRankSum.begin(),
              tableEntry.metricRankSum.end(),0);

    for(size_t i=0; i<tableEntry.metricRank.size();++i){
      for(size_t j=0; j<tableEntry.metricRank[i].size();++j){
        size_t k = tableEntry.metricRank[i][j];
        tableEntry.metricRankSum[k]+=i;
      }
    }

    //Swap the: now the index holds the rank.
    std::vector< std::vector< size_t > > metricRankUnsorted;
    metricRankUnsorted = tableEntry.metricRank;

    for(size_t i=0; i<tableEntry.metricRank.size();++i){
      for(size_t j=0; j<tableEntry.metricRank[i].size();++j){
        size_t k = metricRankUnsorted[i][j];
        tableEntry.metricRank[k][j]=i;
      }
    }

    tableEntry.rank = rank(tableEntry.metricRankSum,true);
    
    //Reorder all fields according to the ranking
    FinancialAnalysisToolkit::MetricTable tableEntryUnsorted = tableEntry;
    for(size_t i=0; i<tableEntry.rank.size();++i){

      size_t k = tableEntryUnsorted.rank[i];
      tableEntry.tickers[i]       = tableEntryUnsorted.tickers[k];
      tableEntry.rank[i]          = i;
      tableEntry.metricRankSum[i] = tableEntryUnsorted.metricRankSum[k];

      for(size_t j=0;j<tableEntryUnsorted.metrics[0].size();++j){
        tableEntry.metrics[i][j]    = tableEntryUnsorted.metrics[k][j];
        tableEntry.metricRank[i][j] = tableEntryUnsorted.metricRank[k][j];
      }
    }

  }

};

//==============================================================================
void writeMetricTableToJsonFile(
      std::vector< FinancialAnalysisToolkit::MetricTable > &metricTableSet,
      std::vector< std::vector< std::string> > &listOfRankingMetrics,
      std::string &fundamentalFolder,
      std::string &historicalFolder,
      std::string &rankFolderOutput,
      std::string &outputFileName,
      bool verbose){

  if(verbose){
    std::cout<<"Writing Metric Tables to json" << std::endl;
  }

  nlohmann::ordered_json jsonMetricTableSet;

  for(auto const &tableEntry: metricTableSet){
    date::year_month_day ymdStart = tableEntry.dateStart;
    date::year_month_day ymdEnd   = tableEntry.dateEnd;

    nlohmann::ordered_json jsonTableEntry;

    std::stringstream dateStart, dateEnd;
    dateStart << ymdStart.year() <<"-"<< unsigned{ymdStart.month()} <<"-"<<ymdStart.day();
    dateEnd   << ymdEnd.year()   <<"-"<< unsigned{ymdEnd.month()}   <<"-"<<ymdEnd.day();

    nlohmann::ordered_json jsonDateEntry=nlohmann::ordered_json::object();

    jsonDateEntry.push_back({"dateStart",dateStart.str()});
    jsonDateEntry.push_back({"dateEnd",dateEnd.str()});

    jsonTableEntry.push_back(jsonDateEntry);

    if(verbose){
      std::cout << dateStart.str() << " to " << dateEnd.str() << std::endl;
    }

    for(size_t i=0; i<tableEntry.tickers.size();++i){
      nlohmann::ordered_json jsonTickerEntry=nlohmann::ordered_json::object();
      if(verbose){
        std::cout << '\t' << i << '\t' << tableEntry.tickers[i] << std::endl;
      }
      jsonTickerEntry.push_back({"PrimaryTicker", tableEntry.tickers[i]});
      jsonTickerEntry.push_back({"Ranking",       tableEntry.rank[i]});      
      jsonTickerEntry.push_back({"MetricRankSum", tableEntry.metricRankSum[i]});
      for(size_t j=0; j<tableEntry.metrics[i].size();++j){
        std::string metricValueName = listOfRankingMetrics[j][0];
        std::string metricRankName  = listOfRankingMetrics[j][0]; 

        metricValueName.append("_value");
        metricRankName.append("_rank");
        jsonTickerEntry.push_back({metricRankName,tableEntry.metricRank[i][j]});
        jsonTickerEntry.push_back({metricValueName,tableEntry.metrics[i][j]});
      } 
      jsonTableEntry.push_back(jsonTickerEntry);     
    }
    jsonMetricTableSet.push_back(jsonTableEntry);
  }
  std::string rankingResultFilePath = rankFolderOutput;
  rankingResultFilePath.append(outputFileName);
  
  std::ofstream outputFileStream(rankingResultFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << jsonMetricTableSet;
  outputFileStream.close();  
}



//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string analysisFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string rankFolder;
  std::string rankingFilePath;
  bool analyzeYears=true;
  bool analyzeQuarters=false;
  unsigned int referenceMonth = 12;
  bool ignoreNegativeValues=true; 

  int maxDifferenceInDays = 15; //a half month at most.

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will produce tables that rank companies "
    "according to the value of individual and groups of performace metrics."
    ,' ', "0.0");

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");
    cmd.add(exchangeCodeInput);  


    TCLAP::ValueArg<std::string> analysisFolderInput("i","input_folder_path", 
      "The path to the folder contains the analysis json files "
      "produced by the calculate method",
      true,"","string");
    cmd.add(analysisFolderInput);

    TCLAP::ValueArg<std::string> rankFolderOutput("o","output_folder_path", 
      "The path to the folder will contain the ranking tables produced by "
      " this method.",
      true,"","string");
    cmd.add(rankFolderOutput);

    TCLAP::ValueArg<std::string> rankingFilePathInput("r",
      "ranking_configuration_file", 
      "The path to the file that contains the list of metrics to use when "
      "ranking businesses. Each row must contain two entries: the first is the"
      "name of a metric that exists in the analysis data (e.g. priceToValue), "
      "and the second entry is either smallestIsBest or biggestIsBest that is "
      "used to ensure that a rank of 1 is always best. If multiple metrics are "
      "given, the final ranking is formed using the sum of each metrics"
      " ranking.",
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

    TCLAP::ValueArg< unsigned int > referenceMonthInput("m","reference_month", 
      "The month that is the expected month of submission with January as 1 "
      " and December as 12.",
      false,1,"unsigned int");
    cmd.add(referenceMonthInput);

    TCLAP::SwitchArg ignoreNegativeValuesInput("n","ignore_negative_values",
      "Ignore negative values", false);
    cmd.add(ignoreNegativeValuesInput);    


    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode          = exchangeCodeInput.getValue();    
    analysisFolder        = analysisFolderInput.getValue();
    rankFolder            = rankFolderOutput.getValue();
    fundamentalFolder     = fundamentalFolderInput.getValue();
    historicalFolder      = historicalFolderInput.getValue();
    rankingFilePath       = rankingFilePathInput.getValue();
    analyzeQuarters       = quarterlyAnalysisInput.getValue();
    analyzeYears          = !analyzeQuarters;
    referenceMonth        = referenceMonthInput.getValue();
    ignoreNegativeValues  = ignoreNegativeValuesInput.getValue();

    if(referenceMonth < 1 || referenceMonth > 12){
      std::cout << "Exiting: referenceMonth must be between 1-12" << std::endl;
      std::abort();
    }

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

      std::cout << "  Ranking Output Folder" << std::endl;
      std::cout << "    " << rankFolder << std::endl;

      std::cout << "  Ranking File Path" << std::endl;
      std::cout << "    " << rankingFilePath << std::endl;

      std::cout << "  Analyze Quarters (untested)" << std::endl;
      std::cout << "    " << analyzeYears << std::endl;

      if(analyzeQuarters){
        std::cout << "Exiting: analyze quarters has not been tested "
                  << std::endl;
        std::abort();                  
      }
    }

    //Read the list of metrics into a 2D array of strings    
    std::vector< std::vector< std::string > > listOfRankingMetrics;    
    bool inputsAreValid = UtilityFunctions::readListOfRankingMetrics(
                                                 rankingFilePath,
                                                 listOfRankingMetrics,
                                                 verbose);

    // Read in the data from each file
    std::vector< FinancialAnalysisToolkit::TickerMetricData > tickerMetricDataSet;
    date::year_month_day youngestDate = date::year{1900}/1/1;
    auto today = date::floor<date::days>(std::chrono::system_clock::now());
    date::year_month_day oldestDate(today);

    if(inputsAreValid){  
      inputsAreValid = readMetricData(analysisFolder,
                                      listOfRankingMetrics,
                                      tickerMetricDataSet,
                                      youngestDate,
                                      oldestDate,
                                      ignoreNegativeValues,
                                      verbose);
    }

    //==========================================================================
    //Create the ranking table and write to file 
    //==========================================================================
    if(inputsAreValid){
      std::vector< FinancialAnalysisToolkit::MetricTable > metricTableSet;

      createMetricTable( 
            tickerMetricDataSet,
            metricTableSet,
            youngestDate,
            oldestDate,
            referenceMonth,
            analyzeQuarters, 
            verbose);
      
      sortMetricTable(metricTableSet, listOfRankingMetrics);

      std::string jsonFileName = std::filesystem::path(rankingFilePath).stem();
      jsonFileName.append("_ranking.json");

      writeMetricTableToJsonFile(metricTableSet,
                            listOfRankingMetrics,
                            fundamentalFolder,
                            historicalFolder,
                            rankFolder,
                            jsonFileName,
                            verbose);    

      std::string csvFileName = std::filesystem::path(rankingFilePath).stem();
      csvFileName.append("_ranking.csv");
                        
    }


  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  return 0;
}
