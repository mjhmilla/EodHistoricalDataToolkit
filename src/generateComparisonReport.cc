//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <limits>
#include <filesystem>

#include <chrono>
#include "date.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"
#include <sciplot/sciplot.hpp>
#include "PlottingFunctions.h"
#include "ReportingFunctions.h"
#include "ScreenerToolkit.h"

struct TickerSet{
  std::vector< std::string > filtered;
  std::vector< std::string > processed;
};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string calculateDataFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string comparisonReportConfigurationFilePath;  
  std::string tickerReportFolder;
  std::string comparisonReportFolder;  
  std::string dateOfTable;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will compare the results of multiple "
    "screens side-by-side."
    ,' ', "0.0");

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");
    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> tickerReportFolderOutput("t",
      "ticker_report_folder_path", 
      "The path to the folder will contains the ticker reports.",
      true,"","string");
    cmd.add(tickerReportFolderOutput);

    TCLAP::ValueArg<std::string> comparisonReportFolderOutput("o",
      "comparison_report_folder_path", 
      "The path to the folder will contains the comparison report.",
      true,"","string");
    cmd.add(comparisonReportFolderOutput);    

    TCLAP::ValueArg<std::string> calculateDataFolderInput("a",
      "calculate_folder_path", 
      "The path to the folder contains the output "
      "produced by the calculate method",
      true,"","string");
    cmd.add(calculateDataFolderInput);

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

    TCLAP::ValueArg<std::string> dateOfTableInput("d","date", 
      "Date used to produce the dicounted cash flow model detailed output.",
      false,"","string");
    cmd.add(dateOfTableInput);

    TCLAP::ValueArg<std::string> comparisonReportConfigurationFilePathInput("c",
      "screen_report_configuration_file", 
      "The path to the json file that contains the names of the metrics "
      " to plot.",
      true,"","string");

    cmd.add(comparisonReportConfigurationFilePathInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode              = exchangeCodeInput.getValue();  
    comparisonReportConfigurationFilePath 
                        = comparisonReportConfigurationFilePathInput.getValue();      
    calculateDataFolder       = calculateDataFolderInput.getValue();
    fundamentalFolder         = fundamentalFolderInput.getValue();
    historicalFolder          = historicalFolderInput.getValue();    
    tickerReportFolder        = tickerReportFolderOutput.getValue();
    comparisonReportFolder    = comparisonReportFolderOutput.getValue();    
    dateOfTable               = dateOfTableInput.getValue();
    verbose                   = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;


      if(dateOfTable.length()>0){
        std::cout << "  Date used to calculate tabular output" << std::endl;
        std::cout << "    " << dateOfTable << std::endl;
      }

      std::cout << "  Calculate Data Input Folder" << std::endl;
      std::cout << "    " << calculateDataFolder << std::endl;

      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;

      std::cout << "  Ticker Report Folder" << std::endl;
      std::cout << "    " << tickerReportFolder << std::endl;  

      std::cout << "  Comparison Report Configuration File" << std::endl;
      std::cout << "    " <<comparisonReportConfigurationFilePath << std::endl;          

      std::cout << "  Comparison Report Folder" << std::endl;
      std::cout << "    " << comparisonReportFolder << std::endl;  

    }

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  bool replaceNansWithMissingData = true;

  auto today = date::floor<date::days>(std::chrono::system_clock::now());
  date::year_month_day targetDate(today);
  int maxTargetDateErrorInDays = 365*2;

  //Load the report configuration file
  nlohmann::ordered_json comparisonReportConfig;
  bool loadedConfiguration = 
    JsonFunctions::loadJsonFile(comparisonReportConfigurationFilePath,
                                comparisonReportConfig,
                                verbose);
                                  

  if(!loadedConfiguration){
    std::cerr << "Error: cannot open " << comparisonReportConfigurationFilePath 
              << std::endl;
    std::abort();    
  }

  //
  // Filter loop
  //    - Loop over all tickers
  //    - For each ticker, loop over all filters. 
  //    - Store the tickers that pass each specific filter
  //

  if(verbose){
    std::cout << std::endl << "Filtering" << std::endl << std::endl;
  }

  std::vector< TickerSet > tickerSet;
  tickerSet.resize(comparisonReportConfig["screens"].size());
  std::string analysisExt = ".json";  

  for (const auto & file 
        : std::filesystem::directory_iterator(calculateDataFolder)){    

    bool validInput = true;
    std::string fileName   = file.path().filename();
    std::size_t fileExtPos = fileName.find(analysisExt);
    //if(verbose){
    //  std::cout << fileName << std::endl;
    //}
    if( fileExtPos == std::string::npos ){
      validInput=false;
      if(verbose){
        std::cout << std::endl;
        std::cout << fileName << std::endl;
        std::cout << "Skipping " << fileName 
                  << " should end in .json but this does not:" 
                  << std::endl
                  << std::endl;
      }
    }     

    //Check to see if this ticker passes the filter
    bool tickerPassesFilter=false;

    if(validInput){   
      int screenCount = 0;
      for(auto &screenItem : comparisonReportConfig["screens"].items()){  

        tickerPassesFilter = 
          ScreenerToolkit::applyFilter(
            fileName,  
            fundamentalFolder,
            historicalFolder,
            calculateDataFolder,
            tickerReportFolder,
            screenItem.value(),
            targetDate,
            maxTargetDateErrorInDays,                                  
            replaceNansWithMissingData,
            verbose);

        if(tickerPassesFilter){
          tickerSet[screenCount].filtered.push_back(fileName);

          if(verbose){
            std::cout << fileName << '\t' 
                      << tickerSet[screenCount].filtered.size() << '\t'
                      << " in screen "
                      << (1+screenCount) << "/" 
                      << comparisonReportConfig["screens"].size()
                      << std::endl; 
          }
          break;
        }
        ++screenCount;
      }
    }
  }

  //
  // Ranking loop
  //  -For each screen, rank the tickers that pass the filter
  //
  if(verbose){
    std::cout << std::endl << "Ranking" << std::endl << std::endl;
  }


  std::vector< ScreenerToolkit::MetricSummaryDataSet > metricSummaryDataSet;
  metricSummaryDataSet.resize(tickerSet.size());

  int screenCount = 0;
  for(auto &screenItem : comparisonReportConfig["screens"].items()){

    if(tickerSet[screenCount].filtered.size()>0){
      for(size_t i=0; i< tickerSet[screenCount].filtered.size();++i){
        bool appendedMetricData = 
          ScreenerToolkit::appendMetricData(
                          tickerSet[screenCount].filtered[i],  
                          fundamentalFolder,                 
                          historicalFolder,
                          calculateDataFolder,
                          screenItem.value(),
                          targetDate,
                          maxTargetDateErrorInDays,
                          metricSummaryDataSet[screenCount],                        
                          verbose);
      }

      ScreenerToolkit::rankMetricData(screenItem.value(),
                                      metricSummaryDataSet[screenCount],
                                      verbose);

    }
    ++screenCount;
  }


  //
  // Reporting loop
  //  

  if(verbose){
    std::cout << std::endl << "Generating reports"  << std::endl << std::endl;
  }

  std::string comparisonReportConfigurationFileName =
    std::filesystem::path(comparisonReportConfigurationFilePath).filename();

  ReportingFunctions::sanitizeFolderName(comparisonReportConfigurationFileName);  

  int numberOfScreensPerReport = 50;
  int maximumNumberOfReports=1;

  if(comparisonReportConfig.contains("report")){
    if(comparisonReportConfig["report"].contains("number_of_screens_per_report")){
      double tmp = JsonFunctions::getJsonFloat(
        comparisonReportConfig["report"]["number_of_screens_per_report"],false);
      if(!std::isnan(tmp)){
        numberOfScreensPerReport = static_cast<int>(tmp);
      }
    }
    if(comparisonReportConfig["report"].contains("number_of_reports")){
      double tmp = JsonFunctions::getJsonFloat(
        comparisonReportConfig["report"]["number_of_reports"],false);
      if(!std::isnan(tmp)){
        maximumNumberOfReports = static_cast<int>(tmp);
      }        
    }      
  }

  int maximumNumberOfReportsDefault = 
      static_cast<int>(
          std::ceil(
            static_cast<double>(metricSummaryDataSet.size())
          / static_cast<double>(numberOfScreensPerReport  ))
        );


  maximumNumberOfReports = std::min(maximumNumberOfReports,
                                    maximumNumberOfReportsDefault);

  for(size_t indexReport=0; indexReport < maximumNumberOfReports;++indexReport){



    size_t indexStart = (numberOfScreensPerReport)*indexReport;
    size_t indexEnd   = std::min( indexStart+numberOfScreensPerReport,
                                  metricSummaryDataSet.size()       );  

    std::stringstream reportNumber;
    reportNumber << indexReport;
    std::string reportNumberStr(reportNumber.str());

    while(reportNumberStr.length()<3){
      std::string tmp("0");
      reportNumberStr = tmp.append(reportNumberStr);
    } 

    std::string summaryPlotFileName("summary_");
    summaryPlotFileName.append(comparisonReportConfigurationFileName);
    summaryPlotFileName.append("_");
    summaryPlotFileName.append(reportNumberStr);
    summaryPlotFileName.append(".pdf");
    
    PlottingFunctions::PlotSettings settings;

    std::vector< std::string > screenSummaryPlots;
    screenSummaryPlots.push_back(summaryPlotFileName);


  }

  return 0;
}
