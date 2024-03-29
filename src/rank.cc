
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
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
  std::vector< std::vector< int > > metricRank;
  std::vector< int > rank;
};

bool loadRankingMetricTable(
          std::string& rankingFilePath,
          std::vector< std::vector< std::string > > &listOfRankingMetrics,
          bool verbose){

  bool rankingFileIsValid=true;

    std::ifstream file(rankingFilePath);

    if(file.is_open()){      
      std::string entryA,entryB;
      do{        
        entryA.clear();
        std::getline(file,entryA,',');
        if(entryA.length() > 0){
          std::getline(file,entryB,'\n');
          std::vector< std::string> rankingMetric;          
          rankingMetric.push_back(entryA);
          rankingMetric.push_back(entryB);

          if(   entryB.compare("smallestIsBest") == 0
             || entryB.compare("biggestIsBest" ) == 0){ 
            listOfRankingMetrics.push_back(rankingMetric);        
          }else{
            if(verbose){
              std::cout << "Error: the second entry of each line in " 
                        << rankingFilePath 
                        << " must be either smallestIsBest or biggestIsBest. " 
                        << "This entry is not acceptable: " 
                        << entryB << std::endl; 
            }
          }

          if(rankingMetric.size() != 2){
            rankingFileIsValid=false;
            if(verbose){
              std::cout << "Error: each line in " << rankingFilePath 
                        << " must have exactly 2 entries each separated "
                        << "by a comma." << std::endl; 
            }
          }
        }
      }while(entryA.length() > 0 && rankingFileIsValid);
      
      if(listOfRankingMetrics.size()==0){
        rankingFileIsValid=false;
        if(verbose){
          std::cout << "Error: " << rankingFilePath 
                    << " is empty. " << std::endl;
        }
      }

    }else{
      rankingFileIsValid = false;
      if(verbose){
        std::cout << "Error: could not open " << rankingFilePath << std::endl;
      }
    }

  return rankingFileIsValid;

};


int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string analysisFolder;
  std::string rankFolder;
  std::string rankingFilePath;
  bool analyzeYears=true;
  bool analyzeQuarters=false;
  unsigned int referenceMonth = 12; 

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

    TCLAP::SwitchArg quarterlyAnalysisInput("q","quarterly",
      "Analyze quarterly data. Caution: this is not yet been tested.", false);
    cmd.add(quarterlyAnalysisInput);    

    TCLAP::ValueArg< unsigned int > referenceMonthInput("m","reference_month", 
      "The month that is the expected month of submission with January as 1 "
      " and December as 12.",
      false,12,"unsigned int");
    cmd.add(referenceMonthInput);


    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode          = exchangeCodeInput.getValue();    
    analysisFolder        = analysisFolderInput.getValue();
    rankFolder            = rankFolderOutput.getValue();
    rankingFilePath       = rankingFilePathInput.getValue();
    analyzeQuarters       = quarterlyAnalysisInput.getValue();
    analyzeYears          = !analyzeQuarters;
    referenceMonth        = referenceMonthInput.getValue();

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

    //==========================================================================
    //
    //Read the list of metrics into a 2D array of strings
    //
    //==========================================================================    
    
    std::vector< std::vector< std::string > > listOfRankingMetrics;    
    bool inputsAreValid = loadRankingMetricTable(rankingFilePath,
                                                 listOfRankingMetrics,
                                                 verbose);

    std::vector< TickerMetricData > tickerMetricDataSet;
    date::year_month_day youngestDate = date::year{1900}/1/1;
    auto today = date::floor<date::days>(std::chrono::system_clock::now());
    date::year_month_day oldestDate(today);

    std::vector< int > monthOfRecord;
    monthOfRecord.resize(12);
    if(verbose){
      std::cout << std::endl;
      std::cout << "Reading in metric values." << std::endl;
      std::cout << '\t' << "# entries" << '\t' << "Valid Files" << std::endl;
      std::cout <<  '\t' << '\t' << '\t' << "Invalid Files" << std::endl;
 
    }
    //==========================================================================
    //
    // Read in the data from each file
    //
    //==========================================================================
    if(inputsAreValid){      
      int validFileCount=0;
      int totalFileCount=0;      
      std::string analysisExt = ".analysis.json";

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
        if(validInput){
          try{
            //Load the json file
            std::stringstream ss;
            ss << analysisFolder << fileName;
            std::string filePathName = ss.str();
            std::ifstream inputJsonFileStream(filePathName.c_str());
            analysisData = nlohmann::ordered_json::parse(inputJsonFileStream);

          }catch(const nlohmann::json::parse_error& e){
            std::cout << e.what() << std::endl;
            validInput=false;
            if(verbose){
              std::cout << "  Skipping: failed while reading json file " 
                        << fileName << std::endl; 
            }
          }
        }

        //
        //Populate the Metric table
        //        
        TickerMetricData tickerMetricDataEntry;
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
    }

    //==========================================================================
    //
    //Create the individual ranking tables 
    //
    //==========================================================================
    if(tickerMetricDataSet.size()>0){
      std::vector< MetricTable > metricTableSet;
      MetricTable metricTableEntry;      


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
      
      bool here=true;
      //Scan through the TickerMetricDataSet and put each entry into
      //the correct analysis period

    }


  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  return 0;
}
