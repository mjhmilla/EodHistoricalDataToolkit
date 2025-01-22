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
void createComparisonConfig(nlohmann::ordered_json &configTemplate,
                            nlohmann::ordered_json &configUpd)
{

  //Expand the template into a full set of screens
  std::vector < std::string > manditoryFields;
  manditoryFields.push_back("report");
  manditoryFields.push_back("screen_variations");
  manditoryFields.push_back("screen_template");

  for(size_t i=0; i<manditoryFields.size();++i){
    if(!configTemplate.contains(manditoryFields[i].c_str())){
      std::cerr << "Error: comparision template file does not contain a "
                << manditoryFields[i].c_str()
                << "field"
                << std::endl;
      std::abort();
    }
  }

  //Check that each series item has the same number of values
  int numberOfVariations=0;
  for(auto &seriesItem : configTemplate["screen_variations"].items()){
    if(numberOfVariations==0){
      numberOfVariations = seriesItem.value()["values"].size();
    }else{
      if(seriesItem.value()["values"].size() != numberOfVariations){
        std::cerr << "Error: each series item must have the same number of "
                  << "items in the values field"
                  << std::endl;
        std::abort();                  
      }
    }
  }

  //Create the set of screens from the template
  configUpd["report"] = configTemplate["report"];  
  nlohmann::ordered_json comparisonReportScreens;
  int numberOfDigits = 
    static_cast<int>(std::ceil(static_cast<double>(numberOfVariations)/10.0));

  for(size_t i=0; i < numberOfVariations; ++i){

    nlohmann::ordered_json screenEntry = nlohmann::ordered_json::object();

    //Deep copy of the template
    screenEntry["filter"].update(
        configTemplate["screen_template"]["filter"]);
    screenEntry["ranking"].update(
        configTemplate["screen_template"]["ranking"]);
    screenEntry["weighting"].update(
        configTemplate["screen_template"]["weighting"]);


    //Update the template    
    for(auto &updItem : configTemplate["screen_variations"].items()){

      //Get the address to update
      std::vector< std::string > field;
      for(auto &fieldItem : updItem.value()["field"].items()){
        field.push_back(fieldItem.value().get<std::string>());
      }

      //Get the type
      std::string dataType = updItem.value()["value_type"];
      bool isString=false;
      if(dataType.compare("string")==0){
        isString=true;
      }else if(dataType.compare("float")==0){
        isString=false;
      }else{
        std::cerr << "Error: screen_variation " << updItem.key() 
                  << " has a value_type of " << dataType 
                  << " which is not one of the two accepted values: "
                  << "string or float"
                  << std::endl;
      }

      std::string dataStruct = updItem.value()["value_structure"];
      bool isArray=false;
      if(dataStruct.compare("array")==0){
        isArray=true;
      }else if(dataStruct.compare("scalar")==0){
        isArray=false;
      }else{
        std::cerr << "Error: screen_variation " << updItem.key() 
                  << " has a value_structure of " << dataStruct 
                  << " which is not one of the two accepted values: "
                  << "array or scalar"
                  << std::endl;
      }


      //Get the new value
      auto &valueItem = updItem.value()["values"].at(i);
      bool updateSuccessful = true;
      std::string strValue;
      double floatValue;

      if(isString){
        strValue = valueItem.get<std::string>();

        switch( field.size() ){
          case 1:{
            if(isArray){
              screenEntry[field[0]]={(strValue)};
            }else{
              screenEntry[field[0]] = strValue;
            }
          };
          break;
          case 2:{
            if(isArray){
              screenEntry[field[0]][field[1]]={(strValue)};
            }else{
              screenEntry[field[0]][field[1]] = strValue;
            }
          };
          break;
          case 3:{
            if(isArray){
              screenEntry[field[0]][field[1]][field[2]]={(strValue)};
            }else{
              screenEntry[field[0]][field[1]][field[2]] = strValue;
            }
          };
          break;
          case 4:{
            if(isArray){
              screenEntry[field[0]][field[1]][field[2]][field[3]]
                ={(strValue)};
            }else{
              screenEntry[field[0]][field[1]][field[2]][field[3]] 
                = strValue;
            }
          };
          break;
          default: {
            updateSuccessful=false;
          }
        };

      }else{
        floatValue = valueItem.get<double>();

        switch( field.size() ){
          case 1:{
            if(isArray){
              screenEntry[field[0]]={(floatValue)};
            }else{
              screenEntry[field[0]] = floatValue;
            }
          };
          break;
          case 2:{
            if(isArray){
              screenEntry[field[0]][field[1]]={(floatValue)};
            }else{
              screenEntry[field[0]][field[1]] = floatValue;
            }
          };
          break;
          case 3:{
            if(isArray){
              screenEntry[field[0]][field[1]][field[2]]={(floatValue)};            
            }else{
              screenEntry[field[0]][field[1]][field[2]] = floatValue;
            }
          };
          break;
          case 4:{
            if(isArray){
              screenEntry[field[0]][field[1]][field[2]][field[3]]={( 
                floatValue)};
            }else{
              screenEntry[field[0]][field[1]][field[2]][field[3]] 
                = floatValue;
            }
          };
          break;
          default: {
            updateSuccessful=false;
          }
        };
      }

      if(!updateSuccessful){
        std::cerr << "Error: Unable to update: ";
        for(size_t j=0; j<field.size();++j){
          std::cerr << field[j] << " ";
        }
        if(isString){
          std::cerr << "to a new string value of " << strValue << std::endl;
        }else{
          std::cerr << "to a new float value of " << floatValue << std::endl;
        }
      }

      std::stringstream ss;
      ss << i;
      std::string stringNumber = ss.str();

      while(stringNumber.size()<numberOfDigits){
        stringNumber.push_back('0');
      }

      std::string screenName = "screen_";
      screenName.append(stringNumber);

      comparisonReportScreens[screenName]=screenEntry;
    }
  }

  configUpd["screens"] = comparisonReportScreens;




};

//==============================================================================
void plotComparisonReportData(
    size_t indexStart,
    size_t indexEnd,
    const nlohmann::ordered_json &comparisonConfig, 
    const std::vector< ScreenerToolkit::MetricSummaryDataSet > &metricSummaryDataSet,
    const PlottingFunctions::PlotSettings &settings,
    const std::string &comparisonReportFolder,
    const std::string &comparisonPlotFileName,
    bool verbose)
{

  bool screenFieldExists  = comparisonConfig.contains("screens");
  bool rankingFieldExists = true;
  bool rankingSizeConsistent=true;
  int numberOfScreens = 0;  
  int numberOfRankingItems=0;
  

  if(screenFieldExists){
    numberOfScreens = comparisonConfig["screens"].size();

    for(auto &screenItem : comparisonConfig["screens"].items()){
      if(!screenItem.value().contains("ranking")){
        rankingFieldExists=false;
      }
      if(numberOfRankingItems==0){
        numberOfRankingItems = screenItem.value()["ranking"].size();
      }else{
        if(numberOfRankingItems != screenItem.value()["ranking"].size()){
          rankingSizeConsistent=false;
        }
      }
    } 
  }

  if(screenFieldExists && rankingFieldExists && rankingSizeConsistent){

    
    std::vector< std::vector< sciplot::PlotVariant > > arrayOfPlotVariant;

    std::vector< std::vector< sciplot::Plot2D >> arrayOfPlot2D;
    arrayOfPlot2D.resize(numberOfRankingItems);
    for(size_t i=0; i<numberOfRankingItems; ++i){
      arrayOfPlot2D[i].resize(1);
    }

;  
    std::vector< bool > axisLabelsAdded;
    axisLabelsAdded.resize(numberOfRankingItems);
    for(size_t i=0;i<numberOfRankingItems;++i){
      axisLabelsAdded[i]=false;
    }

    size_t indexScreen=0;

    for(auto const &screenItem : comparisonConfig["screens"].items()){

      bool isValid = true;
      if(metricSummaryDataSet[indexScreen].ticker.size() == 0){
        isValid = false;
      }

      if(isValid && indexScreen >= indexStart && indexScreen < indexEnd){


        int numberOfTickersPerScreen = -1;

        double tmp = 
          JsonFunctions::getJsonFloat(
            comparisonConfig["report"]["number_of_tickers_per_screen"],false);

        if(JsonFunctions::isJsonFloatValid(tmp)){
          numberOfTickersPerScreen = static_cast<int>(tmp);
        }

        if(numberOfTickersPerScreen==-1 
            || numberOfTickersPerScreen > metricSummaryDataSet[indexScreen].ticker.size()){
          numberOfTickersPerScreen = 
            metricSummaryDataSet[indexScreen].ticker.size();
        }

        int indexRanking=0;
        for(auto const &rankingItem : screenItem.value()["ranking"].items())
        {
          PlottingFunctions::PlotSettings subplotSettings = settings;
          subplotSettings.lineWidth = 0.5;

          double weight = 
            JsonFunctions::getJsonFloat(rankingItem.value()["weight"]);

          double lowerBoundPlot = 
            JsonFunctions::getJsonFloat(
              rankingItem.value()["plotSettings"]["lowerBound"]);

          double upperBoundPlot = 
            JsonFunctions::getJsonFloat(
              rankingItem.value()["plotSettings"]["upperBound"]);

          bool useLogarithmicScaling=
            JsonFunctions::getJsonBool(
              rankingItem.value()["plotSettings"]["logarithmic"]);
          subplotSettings.logScale=useLogarithmicScaling;

          double width = 
            JsonFunctions::getJsonFloat(
              rankingItem.value()["plotSettings"]["width"]);

          double height = 
            JsonFunctions::getJsonFloat(
              rankingItem.value()["plotSettings"]["height"]);

          subplotSettings.plotWidthInPoints=
            PlottingFunctions::convertCentimetersToPoints(width);
          subplotSettings.plotHeightInPoints=
            PlottingFunctions::convertCentimetersToPoints(height);

          std::string direction;
          JsonFunctions::getJsonString(rankingItem.value()["direction"],direction);
          bool smallestIsBest=false;
          bool biggestIsBest=false;
          if(direction.compare("smallestIsBest")==0){
            smallestIsBest=true;
          }
          if(direction.compare("biggestIsBest")==0){
            biggestIsBest=true;
          }

          double threshold = 
            JsonFunctions::getJsonFloat(
              rankingItem.value()["plotSettings"]["threshold"]);



          if(indexScreen == indexStart){

            sciplot::Vec xThreshold(2);
            sciplot::Vec yThreshold(2);
            xThreshold[0] = static_cast<double>(indexStart)+1.0;
            xThreshold[1] = static_cast<double>(indexEnd)+1.0;
            yThreshold[0] = threshold;
            yThreshold[1] = threshold;

            arrayOfPlot2D[indexRanking][0].drawCurve(xThreshold,yThreshold)
              .lineColor("gray")
              .lineWidth(settings.lineWidth*0.5)
              .labelNone();
          }


          //Evaluate the weighted average across all tickers in
          PlottingFunctions::SummaryStatistics boxWhisker;
          boxWhisker.min=0.;
          boxWhisker.max=0.;
          boxWhisker.current=0.;
          boxWhisker.name = "";
          boxWhisker.percentiles.resize(PlottingFunctions::NUM_PERCENTILES);
          for(size_t i=0; i<PlottingFunctions::NUM_PERCENTILES;++i){
            boxWhisker.percentiles[i]=0.;
          }

          double totalWeight=0;
          for(size_t indexTicker=0; 
              indexTicker < numberOfTickersPerScreen;
              ++indexTicker)
          {
            int indexSorted = 
              metricSummaryDataSet[indexScreen].sortedIndex[indexTicker]; 

            if(metricSummaryDataSet[indexScreen]
                .summaryStatistics[indexSorted][indexRanking].percentiles.size()>0){

              double weight = metricSummaryDataSet[indexScreen].weight[indexSorted]; 
              boxWhisker.min += weight 
                              * metricSummaryDataSet[indexScreen]
                                .summaryStatistics[indexSorted][indexRanking].min;
              boxWhisker.max += weight 
                              * metricSummaryDataSet[indexScreen]
                                .summaryStatistics[indexSorted][indexRanking].max;
              boxWhisker.current += weight 
                              * metricSummaryDataSet[indexScreen]
                                .summaryStatistics[indexSorted][indexRanking].current;

              for(size_t i= 0; i < PlottingFunctions::NUM_PERCENTILES;++i){
                boxWhisker.percentiles[i] += weight 
                  * metricSummaryDataSet[indexScreen]
                    .summaryStatistics[indexSorted][indexRanking].percentiles[i];
              }
              totalWeight += weight;                                                
            }
          }
          
          boxWhisker.min     = boxWhisker.min     / totalWeight;
          boxWhisker.max     = boxWhisker.max     / totalWeight;
          boxWhisker.current = boxWhisker.current / totalWeight;

          for(size_t i=0; i<PlottingFunctions::NUM_PERCENTILES;++i){
            boxWhisker.percentiles[i] = boxWhisker.percentiles[i] / totalWeight;
          }

      

          //Plot the box and whisker
          std::string currentColor("black");
          std::string boxColor("light-gray");
          int currentLineType = 0;

          if(smallestIsBest && boxWhisker.current <= threshold){
              currentLineType=1;
          }
          if(biggestIsBest && boxWhisker.current  >= threshold){
              currentLineType=1;
          }          

          double xMid = static_cast<double>(indexScreen)+1.0;
          double xWidth=0.4;
          PlottingFunctions::drawBoxAndWhisker(
            arrayOfPlot2D[indexRanking][0],
            xMid,
            xWidth,
            boxWhisker,
            boxColor.c_str(),
            currentColor.c_str(),
            currentLineType,
            subplotSettings,
            verbose);          

          //if(!axisLabelsAdded[indexRanking]){

          std::string xAxisLabel("Screen");
          std::string yAxisLabel(rankingItem.key());
          yAxisLabel.append(" (");
          std::stringstream stream;
          stream << std::fixed << std::setprecision(2) << weight;
          yAxisLabel.append( stream.str());
          yAxisLabel.append(")");

          PlottingFunctions::configurePlot(
            arrayOfPlot2D[indexRanking][0],
            xAxisLabel,
            yAxisLabel,
            subplotSettings); 
          axisLabelsAdded[indexRanking]=true;

          //}

          if(!std::isnan(lowerBoundPlot) && !std::isnan(upperBoundPlot) ){
            arrayOfPlot2D[indexRanking][0].yrange(lowerBoundPlot,upperBoundPlot);
          }

          arrayOfPlot2D[indexRanking][0].xrange(
              static_cast<double>(indexStart)-xWidth+1.0,
              static_cast<double>(indexEnd)+xWidth+1.0);

          arrayOfPlot2D[indexRanking][0].legend().hide();

          ++indexRanking;
        }
      }
      ++indexScreen;
    }

    double canvasWidth  = 0.;
    double canvasHeight = 0.;
    bool isCanvasSizeSet=false;

    for(const auto &screenItem : comparisonConfig["screens"].items()){
      for(const auto &rankingItem : screenItem.value()["ranking"].items()){
        if(!isCanvasSizeSet){
          double width = 
            JsonFunctions::getJsonFloat(
              rankingItem.value()["plotSettings"]["width"]);

          double height = 
            JsonFunctions::getJsonFloat(
              rankingItem.value()["plotSettings"]["height"]);

          canvasWidth  =PlottingFunctions::convertCentimetersToPoints(width);
          canvasHeight+=PlottingFunctions::convertCentimetersToPoints(height);      
        }
      }
      if(canvasWidth > 0){
        isCanvasSizeSet=true;
      }
    }

    for(size_t indexRanking=0; indexRanking < numberOfRankingItems; ++indexRanking){
      std::vector< sciplot::PlotVariant > rowOfPlotVariant;      
      rowOfPlotVariant.push_back(arrayOfPlot2D[indexRanking][0]);
      arrayOfPlotVariant.push_back(rowOfPlotVariant);  
    }

    sciplot::Figure figComparison(arrayOfPlotVariant);
    figComparison.title("Comparison");
    sciplot::Canvas canvas = {{figComparison}};

    canvas.size(canvasWidth, canvasHeight) ;    

    // Save the figure to a PDF file
    std::string outputFileName = comparisonReportFolder;
    outputFileName.append(comparisonPlotFileName);
    canvas.save(outputFileName);  

  }



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
  nlohmann::ordered_json comparisonInput;
  bool loadedConfiguration = 
    JsonFunctions::loadJsonFile(comparisonReportConfigurationFilePath,
                                comparisonInput,
                                verbose);
                                  

  if(!loadedConfiguration){
    std::cerr << "Error: cannot open " << comparisonReportConfigurationFilePath 
              << std::endl;
    std::abort();    
  }

  nlohmann::ordered_json comparisonConfig;
  if(comparisonInput.contains("screen_variations")){
    createComparisonConfig(comparisonInput, comparisonConfig);

    std::string outputFilePath(comparisonReportFolder);
    std::string outputFileName("comparison_config_from_template.json");    
    outputFilePath.append(outputFileName);

    std::ofstream outputFileStream(outputFilePath,
        std::ios_base::trunc | std::ios_base::out);
    outputFileStream << comparisonConfig;
    outputFileStream.close();

  }else{
    comparisonConfig.update(comparisonInput);
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
  tickerSet.resize(comparisonConfig["screens"].size());
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
      for(auto &screenItem : comparisonConfig["screens"].items()){  

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
                      << comparisonConfig["screens"].size()
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
  for(auto &screenItem : comparisonConfig["screens"].items()){

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

  if(comparisonConfig.contains("report")){
    if(comparisonConfig["report"].contains("number_of_screens_per_report")){
      double tmp = JsonFunctions::getJsonFloat(
        comparisonConfig["report"]["number_of_screens_per_report"],false);
      if(!std::isnan(tmp)){
        numberOfScreensPerReport = static_cast<int>(tmp);
      }
    }
    if(comparisonConfig["report"].contains("number_of_reports")){
      double tmp = JsonFunctions::getJsonFloat(
        comparisonConfig["report"]["number_of_reports"],false);
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

    //Create the plot name
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

    plotComparisonReportData(
      indexStart,
      indexEnd,
      comparisonConfig, 
      metricSummaryDataSet,
      settings,
      comparisonReportFolder,
      summaryPlotFileName,
      verbose);  
  }

  return 0;
}
