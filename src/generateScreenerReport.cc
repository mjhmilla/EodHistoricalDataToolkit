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

#include "FinancialAnalysisFunctions.h"
#include "JsonFunctions.h"
#include <sciplot/sciplot.hpp>
#include "PlottingFunctions.h"
#include "ReportingFunctions.h"
#include "ScreenerFunctions.h"

//==============================================================================
void plotScreenerReportData(
                    size_t indexTickerStart,
                    size_t indexTickerEnd,
                    const nlohmann::ordered_json &screenReportConfig, 
                    const ScreenerFunctions::MetricSummaryDataSet &metricDataSet,
                    const PlottingFunctions::PlotSettings &settings,
                    const std::string &screenerReportFolder,
                    const std::string &summaryPlotFileName,
                    bool verbose)
{

  if(verbose){

    std::cout << "----------------------------------------" << std::endl;    
    std::cout << "Generating summary plot: " << summaryPlotFileName<< std::endl;
    std::cout << "----------------------------------------" << std::endl;              
  }

  bool rankingFieldExists = screenReportConfig.contains("ranking");

  double canvasWidth=0.;
  double canvasHeight=0.;


  if(rankingFieldExists){

    int numberOfRankingItems = screenReportConfig["ranking"].size();
    int numberOfPlottingItems=0;

    for(auto &rankingItem : screenReportConfig["ranking"].items()){
      bool addPlot = 
        JsonFunctions::getJsonBool(
          rankingItem.value()["plotSettings"]["addPlot"]);   
      if(addPlot){
        ++numberOfPlottingItems;
      }          
    }

    std::vector< std::vector< sciplot::PlotVariant > > arrayOfPlotVariant;
    std::vector< std::vector< sciplot::Plot2D >> arrayOfPlot2D;
    arrayOfPlot2D.resize(numberOfPlottingItems);
    for(size_t i=0; i<numberOfPlottingItems; ++i){
      arrayOfPlot2D[i].resize(1);
    }

    int indexRanking=0;
    int indexPlotting=0;

    for( auto const &rankingItem : screenReportConfig["ranking"].items()){
      bool addPlot = 
        JsonFunctions::getJsonBool(
          rankingItem.value()["plotSettings"]["addPlot"]);

      if(addPlot){
        PlottingFunctions::PlotSettings subplotSettings = settings;
        subplotSettings.lineWidth = 0.5;

        NumericalFunctions::SummaryStatistics groupSummary;   
        groupSummary.percentiles.resize(
            NumericalFunctions::PercentileIndices::NUM_PERCENTILES,0.);

        double groupWeight = 0; //Tickers between indexTickerStart-indexTickerEnd

        NumericalFunctions::SummaryStatistics screenSummary;   
        screenSummary.percentiles.resize(
            NumericalFunctions::PercentileIndices::NUM_PERCENTILES,0.);

        double screenWeight= 0; //All tickers in screenDataSet

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

        canvasWidth=subplotSettings.plotWidthInPoints;
        canvasHeight+=subplotSettings.plotHeightInPoints;

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
        
        sciplot::Vec xThreshold(2);
        sciplot::Vec yThreshold(2);
        xThreshold[0] = static_cast<double>(indexTickerStart)+1.0;
        xThreshold[1] = static_cast<double>(indexTickerEnd)+1.0;
        yThreshold[0] = threshold;
        yThreshold[1] = threshold;

        arrayOfPlot2D[indexPlotting][0].drawCurve(xThreshold,yThreshold)
          .lineColor("gray")
          .lineWidth(settings.lineWidth*0.5)
          .labelNone();

        //
        //Evaluate the box-and whisker plots for the entire set of data
        //
        for(size_t i=0; i<metricDataSet.ticker.size();++i){

          if(metricDataSet.summaryStatistics[i][indexRanking].percentiles.size()>0){

            for(size_t j=0; j < screenSummary.percentiles.size();++j){
              screenSummary.percentiles[j] += 
                metricDataSet.summaryStatistics[i][indexRanking].percentiles[j]
                *metricDataSet.weight[i];
            }

            screenSummary.current +=
              metricDataSet.summaryStatistics[i][indexRanking].current
              *metricDataSet.weight[i];

            screenSummary.min +=  
              metricDataSet.summaryStatistics[i][indexRanking].min
              *metricDataSet.weight[i];

            screenSummary.max +=  
              metricDataSet.summaryStatistics[i][indexRanking].max
              *metricDataSet.weight[i];

            screenWeight += metricDataSet.weight[i];
          }

        }

        //Plot the screen summary
        for(size_t j=0; j < screenSummary.percentiles.size();++j){
          screenSummary.percentiles[j] = 
            screenSummary.percentiles[j] / screenWeight;
        }
        screenSummary.min     = screenSummary.min / screenWeight; 
        screenSummary.max     = screenSummary.max / screenWeight; 
        screenSummary.current = screenSummary.current / screenWeight; 


        std::string currentColor("black");
        std::string boxColor("skyblue");
        int currentLineType = 0;

        if(smallestIsBest && screenSummary.current <= threshold){
            currentLineType=1;
        }
        if(biggestIsBest && screenSummary.current  >= threshold){
            currentLineType=1;
        }

        double xMid = static_cast<double>(indexTickerEnd)+2.0;
        double xWidth = 0.4;
        PlottingFunctions::drawBoxAndWhisker(
          arrayOfPlot2D[indexPlotting][0],
          xMid,
          xWidth,
          screenSummary,
          boxColor.c_str(),
          currentColor.c_str(),
          currentLineType,
          subplotSettings,
          verbose);      


        //
        //Evaluate the box-and-whisker plots for this group of tickers
        //
        size_t tickerCount=0;
        for( size_t i=indexTickerStart; i< indexTickerEnd; ++i){

          size_t indexSorted = metricDataSet.sortedIndex[i];

          double xMid = static_cast<double>(i)+1.0;
          double xWidth = 0.4;

          std::string currentColor("black");
          std::string boxColor("light-gray");
          int currentLineType = 0;

          if(metricDataSet.summaryStatistics[indexSorted][indexRanking].percentiles.size()>0){
            if(smallestIsBest && 
              metricDataSet.summaryStatistics[indexSorted][indexRanking].current 
              <= threshold){
              currentLineType = 1;
            }
            if(biggestIsBest && 
              metricDataSet.summaryStatistics[indexSorted][indexRanking].current 
                >= threshold){    
              currentLineType = 1;      
            }

            PlottingFunctions::drawBoxAndWhisker(
              arrayOfPlot2D[indexPlotting][0],
              xMid,
              xWidth,
              metricDataSet.summaryStatistics[indexSorted][indexRanking],
              boxColor.c_str(),
              currentColor.c_str(),
              currentLineType,
              subplotSettings,
              verbose);

            for(size_t j=0; j < groupSummary.percentiles.size();++j){
              groupSummary.percentiles[j] += 
                metricDataSet.summaryStatistics[indexSorted][indexRanking].percentiles[j]
                *metricDataSet.weight[indexSorted];
            }

            groupSummary.current +=
              metricDataSet.summaryStatistics[indexSorted][indexRanking].current
              *metricDataSet.weight[indexSorted];

            groupSummary.min +=  
              metricDataSet.summaryStatistics[indexSorted][indexRanking].min
              *metricDataSet.weight[indexSorted];

            groupSummary.max +=  
              metricDataSet.summaryStatistics[indexSorted][indexRanking].max
              *metricDataSet.weight[indexSorted];

            groupWeight += metricDataSet.weight[indexSorted];

            ++tickerCount;
          }
        }

        //
        //Plot the group summary
        //
        for(size_t j=0; j < groupSummary.percentiles.size();++j){
          groupSummary.percentiles[j] = 
            groupSummary.percentiles[j] / groupWeight;
        }
        groupSummary.min     = groupSummary.min / groupWeight; 
        groupSummary.max     = groupSummary.max / groupWeight; 
        groupSummary.current = groupSummary.current / groupWeight; 


        currentColor="black";
        boxColor="chartreuse";
        currentLineType = 0;

        if(smallestIsBest && groupSummary.current <= threshold){
            currentLineType=1;
        }
        if(biggestIsBest && groupSummary.current  >= threshold){
            currentLineType=1;
        }

        xMid = static_cast<double>(indexTickerEnd)+1.0;
        PlottingFunctions::drawBoxAndWhisker(
          arrayOfPlot2D[indexPlotting][0],
          xMid,
          xWidth,
          groupSummary,
          boxColor.c_str(),
          currentColor.c_str(),
          currentLineType,
          subplotSettings,
          verbose);
        
        //Add the labels
        std::string xAxisLabel("Ranking");

        std::string yAxisLabel(rankingItem.key());
        yAxisLabel.append(" (");
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << weight;
        yAxisLabel.append( stream.str() );
        yAxisLabel.append(")");


        PlottingFunctions::configurePlot(
          arrayOfPlot2D[indexPlotting][0],
          xAxisLabel,
          yAxisLabel,
          subplotSettings);

        if(!std::isnan(lowerBoundPlot) && !std::isnan(upperBoundPlot) ){
          arrayOfPlot2D[indexPlotting][0].yrange(lowerBoundPlot,upperBoundPlot);
        }

        arrayOfPlot2D[indexPlotting][0].xrange(
            static_cast<double>(indexTickerStart)-xWidth+1.0,
            static_cast<double>(indexTickerEnd)+xWidth+2.0);

        arrayOfPlot2D[indexPlotting][0].legend().hide();
        
        std::vector< sciplot::PlotVariant > rowOfPlotVariant;      
        rowOfPlotVariant.push_back(arrayOfPlot2D[indexPlotting][0]);
        arrayOfPlotVariant.push_back(rowOfPlotVariant);
        ++indexPlotting;
      }

      ++indexRanking;
    } 

    sciplot::Figure figSummary(arrayOfPlotVariant);
    figSummary.title("Summary");
    sciplot::Canvas canvas = {{figSummary}};

    canvas.size(canvasWidth, canvasHeight) ;    

    // Save the figure to a PDF file
    std::string outputFileName = screenerReportFolder;
    outputFileName.append(summaryPlotFileName);
    canvas.save(outputFileName);     
  }

};

//==============================================================================
void generateScreenerLaTeXReport(
                    size_t indexTickerStart,
                    size_t indexTickerEnd,
                    const nlohmann::ordered_json &screenReportConfig, 
                    const ScreenerFunctions::MetricSummaryDataSet &metricDataSet,
                    const std::vector< std::string > &screenSummaryPlots,
                    const std::string &tickerReportFolder,
                    const std::string &screenerReportFolder,
                    const std::string &screenReportFileName,
                    bool verbose){

  if(verbose){
    std::cout << "----------------------------------------" << std::endl;    
    std::cout << "Generating LaTeX report: " << screenReportFileName
              << std::endl;
    std::cout << "----------------------------------------" << std::endl;              
  }


  std::string outputReportPath(screenerReportFolder);
  outputReportPath.append(screenReportFileName);

  std::string latexReportPath(outputReportPath);
  std::ofstream latexReport;

  try{
    latexReport.open(latexReportPath);
  }catch(std::ofstream::failure &ofstreamErr){
    std::cerr << std::endl << std::endl 
              << "Error: an exception was thrown trying to open " 
              << latexReportPath              
              << " :"
              << latexReportPath << std::endl << std::endl
              << ofstreamErr.what()
              << std::endl;    
    std::abort();
  }  

  // Write the opening to the latex file
  latexReport << "\\documentclass[11pt,onecolumn,a4paper]{article}" 
              << std::endl;
  latexReport << "\\usepackage[hmargin={1.35cm,1.35cm},vmargin={2.0cm,3.0cm},"
                    "footskip=0.75cm,headsep=0.25cm]{geometry}"<<std::endl;
  latexReport << "\\usepackage{graphicx,caption}"<<std::endl;
  latexReport << "\\usepackage{times}"<<std::endl;
  latexReport << "\\usepackage{graphicx}"<<std::endl;
  latexReport << "\\usepackage{hyperref}"<<std::endl;
  latexReport << "\\usepackage{multicol}"<<std::endl;
  latexReport << "\\usepackage[usenames,dvipsnames,table]{xcolor}"<<std::endl;

  // Add the opening figure
  std::string reportTitle;
  JsonFunctions::getJsonString(screenReportConfig["report"]["title"],reportTitle);
  latexReport << "\\title{" << reportTitle <<"}" << std::endl;

  latexReport << std::endl << std::endl;
  latexReport << "\\begin{document}" << std::endl << std::endl;
  latexReport << "\\maketitle" << std::endl;
  latexReport << "\\today" << std::endl << std::endl;

  for(auto const &figName : screenSummaryPlots){
    //latexReport << "\\begin{figure}[h]" << std::endl;
    //latexReport << "  \\begin{center}" << std::endl;
    latexReport << "\\includegraphics{"
                << screenerReportFolder << figName
                << "}" << std::endl;
    //latexReport << " \\end{center}" << std::endl;
    //latexReport << "\\end{figure}"<< std::endl;
    latexReport << std::endl;    
  }

  // Add the list of companies
  latexReport << "\\begin{multicols}{3}" << std::endl;
  latexReport << "\\raggedcolumns" << std::endl;
  latexReport << "\\begin{enumerate}" << std::endl;
  latexReport << "\\setcounter{enumi}{" << indexTickerStart <<"}" << std::endl;  
  latexReport << "\\itemsep0pt" << std::endl;

  double totalWeight=0.;
  for(size_t i = 0; i < metricDataSet.ticker.size(); ++i){
    totalWeight += metricDataSet.weight[i];
  }  

  for(size_t i = indexTickerStart; i < indexTickerEnd; ++i){
    size_t j = metricDataSet.sortedIndex[i];

    double weight  = metricDataSet.weight[j];
    std::stringstream stream;
    stream << std::fixed << std::setprecision(3) 
           << (weight/totalWeight)*100.0
           << "\\%";

    std::string tickerString(metricDataSet.ticker[j]);
    ReportingFunctions::sanitizeStringForLaTeX(tickerString,true);

    std::string tickerLabel(metricDataSet.ticker[j]);
    ReportingFunctions::sanitizeLabelForLaTeX(tickerLabel,true);

    //          << "---" << stream.str() 

    latexReport << "\\item " << "\\ref{" << tickerLabel << "} "
                <<  tickerString 
                << "---" << metricDataSet.metricRankSum[j] 
                << std::endl;
  }
  latexReport << "\\end{enumerate}" << std::endl;
  latexReport << "\\end{multicols}" << std::endl;
  latexReport << std::endl;
  
  // Append the ticker reports in oder
  for(size_t i = indexTickerStart; i < indexTickerEnd; ++i){
    size_t j = metricDataSet.sortedIndex[i];

    std::string tickerString(metricDataSet.ticker[j]);
    std::string tickerLabel(metricDataSet.ticker[j]);
    std::string tickerFile(metricDataSet.ticker[j]);

    ReportingFunctions::sanitizeStringForLaTeX(tickerString,true);
    ReportingFunctions::sanitizeLabelForLaTeX(tickerLabel,true);
    ReportingFunctions::sanitizeFolderName(tickerFile,true);

    std::filesystem::path tickerReportPath=tickerReportFolder;
    tickerReportPath /= tickerFile;
    tickerReportPath /= tickerFile;
    tickerReportPath += ".tex";

    std::filesystem::path graphicsPath=tickerReportFolder;
    graphicsPath /= tickerFile;



    latexReport << "\\break" << std::endl;
    latexReport << "\\newpage" << std::endl << std::endl;

    latexReport << "\\section{ " << tickerString << " }" <<std::endl; 
    latexReport << "\\label{" << tickerLabel << "}" << std::endl;
    latexReport << "\\graphicspath{{" << graphicsPath.string() << "}}" 
                << std::endl;
    latexReport << "\\input{"
                << tickerReportPath.string()
                << "}"
                << std::endl
                << std::endl;
  }
  //close the file

  latexReport << "\\end{document}" << std::endl;
  latexReport.close();

};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string calculateDataFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string screenReportConfigurationFilePath;  
  std::string tickerReportFolder;
  std::string screenerReportFolder;  
  std::string dateOfTable;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will produce a screen report in the form of"
    " text and tables about the companies that are best ranked."
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

    TCLAP::ValueArg<std::string> screenerReportFolderOutput("o",
      "screen_report_folder_path", 
      "The path to the folder will contains the screener report.",
      true,"","string");
    cmd.add(screenerReportFolderOutput);    

    TCLAP::ValueArg<std::string> calculateDataFolderInput("a",
      "calculate_folder_path", 
      "The path to the folder contains the output "
      "produced by the calculate method",
      true,"","string");
    cmd.add(calculateDataFolderInput);

    TCLAP::ValueArg<std::string> fundamentalFolderInput("f",
      "fundamental_data_folder_path", 
      "The path to the folder that contains the fundamental data json files "
      "from https://eodhistoricaldata.com/ to analyze",
      true,"","string");
    cmd.add(fundamentalFolderInput);

    TCLAP::ValueArg<std::string> historicalFolderInput("p",
      "historical_data_folder_path", 
      "The path to the folder that contains the historical (price)"
      " data json files from https://eodhistoricaldata.com/ to analyze",
      true,"","string");
    cmd.add(historicalFolderInput);

    TCLAP::ValueArg<std::string> dateOfTableInput("d","date", 
      "Date used to produce the discounted cash flow model detailed output.",
      false,"","string");
    cmd.add(dateOfTableInput);

    TCLAP::ValueArg<std::string> screenerReportConfigurationFilePathInput("c",
      "screen_report_configuration_file", 
      "The path to the json file that contains the names of the metrics "
      " to plot.",
      true,"","string");

    cmd.add(screenerReportConfigurationFilePathInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode              = exchangeCodeInput.getValue();  
    screenReportConfigurationFilePath 
                        = screenerReportConfigurationFilePathInput.getValue();      
    calculateDataFolder       = calculateDataFolderInput.getValue();
    fundamentalFolder         = fundamentalFolderInput.getValue();
    historicalFolder          = historicalFolderInput.getValue();    
    tickerReportFolder        = tickerReportFolderOutput.getValue();
    screenerReportFolder      = screenerReportFolderOutput.getValue();    
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

      std::cout << "  Screener Report Configuration File" << std::endl;
      std::cout << "    " << screenReportConfigurationFilePath << std::endl;          

      std::cout << "  Screener Report Folder" << std::endl;
      std::cout << "    " << screenerReportFolder << std::endl;  

    }

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  bool replaceNansWithMissingData = true;

  auto today = date::floor<date::days>(std::chrono::system_clock::now());
  date::year_month_day targetDate(today);
  int maxTargetDateErrorInDays = 365;

  //Get the name of the configuration 
  std::string screenName = 
    std::filesystem::path(screenReportConfigurationFilePath).filename();
  size_t i = screenName.find_last_of('.');
  screenName = screenName.substr(0,i);

  std::filesystem::path screenReportPath(screenerReportFolder);
  screenReportPath.append(screenName);

  if(!std::filesystem::create_directory(screenReportPath)){
    for (auto& path: std::filesystem::directory_iterator(screenReportPath)) {
        std::filesystem::remove_all(path);
    }
  }

  screenerReportFolder = screenReportPath.string();
  screenerReportFolder = screenerReportFolder 
        + std::filesystem::path::preferred_separator;


  //Load the report configuration file
  nlohmann::ordered_json screenReportConfig;
  bool loadedConfiguration = 
    JsonFunctions::loadJsonFile(screenReportConfigurationFilePath,
                                screenReportConfig,
                                verbose);
                                  

  if(!loadedConfiguration){
    std::cerr << "Error: cannot open " << screenReportConfigurationFilePath 
              << std::endl;
    
  }



  if(loadedConfiguration){
    std::vector< std::string > processedTickers;

    int totalFileCount = 0;
    int validFileCount = 0;
    std::string analysisExt = ".json";  



    //Go through every ticker file in the calculate folder:
    //1. Create a ticker folder in the reporting folder
    //2. Save to this ticker folder:
    //  a. The pdf of the ticker's plots
    //  b. The LaTeX file for the report (as an input)
    //  c. A wrapper LaTeX file to generate an individual report

    std::vector< std::string > filteredTickers;

    bool useFundamentalData=false;
    bool useHistoricalData=false;
    bool useCalculateData=false;

    for(auto &filterItem : screenReportConfig["filter"].items()){
      std::string folder; 
      JsonFunctions::getJsonString(filterItem.value()["folder"],folder);  
      if(folder =="fundamentalData"){
        useFundamentalData=true;
      }else if(folder == "historicalData"){
        useHistoricalData=true;
      }else if(folder == "calculateData"){
        useCalculateData=true;
      }
    }

    for (const auto & file 
          : std::filesystem::directory_iterator(calculateDataFolder)){  

      ++totalFileCount;
      bool validInput = true;

      std::string fileName   = file.path().filename();

      std::size_t fileExtPos = fileName.find(analysisExt);

      if(verbose){
        std::cout << totalFileCount << '\t' << fileName << std::endl;
      }
      
      if( fileExtPos == std::string::npos ){
        validInput=false;
        if(verbose){
          std::cout << '\t' 
                    << "Skipping " << fileName 
                    << " should end in .json but this does not:" << std::endl; 
        }
      }


      //Load the fundamental, historical, and calculate data
      std::string fundamentalDataPath = fundamentalFolder;
      fundamentalDataPath.append(fileName);
      nlohmann::ordered_json fundamentalData;
      bool loadedFundamentalData=false;

      if(useFundamentalData){
        loadedFundamentalData = 
          JsonFunctions::loadJsonFile(fundamentalDataPath,
                                      fundamentalData,
                                      verbose);     
      }

      std::string historicalDataPath = historicalFolder;
      historicalDataPath.append(fileName);
      nlohmann::ordered_json historicalData;
      bool loadedHistoricalData=false;

      if(useHistoricalData){
        loadedHistoricalData = 
          JsonFunctions::loadJsonFile(historicalDataPath,
                                      historicalData,
                                      verbose);     
      }

      std::string calculateDataPath = calculateDataFolder;
      calculateDataPath.append(fileName);
      nlohmann::ordered_json calculateData;
      bool loadedCalculateData=false;

      if(useCalculateData){
        loadedCalculateData = 
          JsonFunctions::loadJsonFile(calculateDataPath,
                                      calculateData,
                                      verbose); 
      }

      validInput  = validInput && (
              (useFundamentalData && loadedFundamentalData) 
          || (useCalculateData   && loadedCalculateData) 
          || (useHistoricalData  && loadedHistoricalData));

      //Check to see if this ticker passes the filter
      bool tickerPassesFilter=false;



      if(validInput){        
        tickerPassesFilter = 
          ScreenerFunctions::applyFilter(
            fileName,  
            fundamentalData,
            historicalData,
            calculateData,
            tickerReportFolder,
            screenReportConfig,
            targetDate,
            maxTargetDateErrorInDays,                                  
            replaceNansWithMissingData,
            verbose);
      }    

      if(tickerPassesFilter){
        filteredTickers.push_back(fileName);
      }
      if(verbose){
        if(tickerPassesFilter){
          std::cout << '\t'
                    << "passed"
                    << std::endl;
        }else{
          std::cout << '\t'
                    << "filtered out"
                    << std::endl;
        }
      }

    }

    if(verbose){
      std::cout << '\n' << '\n'
                << filteredTickers.size() << '\t' 
                << "files passed the filter"
                << '\n' 
                << totalFileCount - filteredTickers.size() << '\t' 
                << "files removed"
                << std::endl;

      std::cout << '\n' << '\n'
                << "Tickers that passed the filter: " << std::endl;
      for(size_t i=0; i< filteredTickers.size(); ++i){
        std::cout << i << '\t' << filteredTickers[i] << std::endl;
      }                

    }

    //
    // Collect all of the metrics used for ranking
    //

    ScreenerFunctions::MetricSummaryDataSet metricDataSet;    
    if(filteredTickers.size() > 0){
      if(verbose){
        std::cout << "Appending metric data ..."  << std::endl << std::endl;
      }

      //Scan to see whether the ranking opens fundamental data, historical data
      //and/or calculate data.

      bool useFundamentalData=false;
      bool useHistoricalData=false;
      bool useCalculateData=false;

      for(auto &filterItem : screenReportConfig["ranking"].items()){
        std::string folder; 
        JsonFunctions::getJsonString(filterItem.value()["folder"],folder);  
        if(folder =="fundamentalData"){
          useFundamentalData=true;
        }else if(folder == "historicalData"){
          useHistoricalData=true;
        }else if(folder == "calculateData"){
          useCalculateData=true;
        }
      }

      for (size_t i=0; i<filteredTickers.size(); ++i){

        if(verbose){
          std::cout << i << "." << '\t' << filteredTickers[i] << std::endl;
        }        

        //
        //Load the fundamental, historical, and calculate data
        //once so that the necessary ranking information can be retreived
        //without having to load the files again.
        //
        std::string fundamentalDataPath = fundamentalFolder;
        fundamentalDataPath.append(filteredTickers[i]);
        nlohmann::ordered_json fundamentalData;
        bool loadedFundamentalData=false;
        if(useFundamentalData){
          loadedFundamentalData = 
            JsonFunctions::loadJsonFile(fundamentalDataPath,
                                        fundamentalData,
                                        verbose);     
        }

        std::string historicalDataPath = historicalFolder;
        historicalDataPath.append(filteredTickers[i]);
        nlohmann::ordered_json historicalData;
        bool loadedHistoricalData=false;
        if(useHistoricalData){
          loadedHistoricalData = 
            JsonFunctions::loadJsonFile(historicalDataPath,
                                        historicalData,
                                        verbose);     
        }

        std::string calculateDataPath = calculateDataFolder;
        calculateDataPath.append(filteredTickers[i]);
        nlohmann::ordered_json calculateData;
        bool loadedCalculateData = false;
        if(useCalculateData){
          loadedCalculateData = 
            JsonFunctions::loadJsonFile(calculateDataPath,
                                        calculateData,
                                        verbose); 
        }

        bool validInput  = (
            (useFundamentalData && loadedFundamentalData) 
         || (useCalculateData   && loadedCalculateData) 
         || (useHistoricalData  && loadedHistoricalData));

        bool appendedMetricData = 
            ScreenerFunctions::appendMetricData(
                        filteredTickers[i],  
                        fundamentalData,                 
                        historicalData,
                        calculateData,
                        screenReportConfig,
                        targetDate,
                        maxTargetDateErrorInDays,
                        metricDataSet,                        
                        verbose);
        
        if(verbose && !appendedMetricData){
          std::cout << "Skipping: " 
                    << filteredTickers[i] 
                    << ": missing valid metric data."
                    << std::endl;
        }                        
      }

      ScreenerFunctions::rankMetricData(screenReportConfig,metricDataSet,verbose);

    }

    std::string screenReportConfigurationFileName =
      std::filesystem::path(screenReportConfigurationFilePath).filename();

    ReportingFunctions::sanitizeFolderName(screenReportConfigurationFileName);  

    int numberOfTickersPerReport = 50;
    int maximumNumberOfReports=1;
    int maximumNumberOfReportsDefault = 
      static_cast< int >(
        std::ceil(static_cast<double>(metricDataSet.ticker.size())
                /static_cast<double>(numberOfTickersPerReport))
      );

    if(screenReportConfig.contains("report")){
      if(screenReportConfig["report"].contains("number_of_tickers_per_report")){
        double tmp = JsonFunctions::getJsonFloat(
          screenReportConfig["report"]["number_of_tickers_per_report"],false);
        if(!std::isnan(tmp)){
          numberOfTickersPerReport = static_cast<int>(tmp);
        }
      }
      if(screenReportConfig["report"].contains("number_of_reports")){
        double tmp = JsonFunctions::getJsonFloat(
          screenReportConfig["report"]["number_of_reports"],false);
        if(!std::isnan(tmp)){
          maximumNumberOfReports = static_cast<int>(tmp);
        }        
      }      
    }

    maximumNumberOfReports = std::min(maximumNumberOfReports,
                                      maximumNumberOfReportsDefault);

    for(size_t indexReport=0; indexReport<maximumNumberOfReports;++indexReport){

      size_t indexStart = (numberOfTickersPerReport)*indexReport;
      size_t indexEnd   = std::min( indexStart+numberOfTickersPerReport,
                                    metricDataSet.ticker.size()       );  

      std::stringstream reportNumber;
      reportNumber << indexReport;
      std::string reportNumberStr(reportNumber.str());

      while(reportNumberStr.length()<3){
        std::string tmp("0");
        reportNumberStr = tmp.append(reportNumberStr);
      }

      std::string summaryPlotFileName("summary_");
      summaryPlotFileName.append(screenReportConfigurationFileName);
      summaryPlotFileName.append("_");
      summaryPlotFileName.append(reportNumberStr);
      summaryPlotFileName.append(".pdf");
      
      PlottingFunctions::PlotSettings settings;

      std::vector< std::string > screenSummaryPlots;
      screenSummaryPlots.push_back(summaryPlotFileName);

      plotScreenerReportData(
        indexStart,
        indexEnd,
        screenReportConfig, 
        metricDataSet,
        settings,
        screenerReportFolder,
        summaryPlotFileName,
        verbose);      

      std::string screenReportFileName("report_");
      screenReportFileName.append(screenReportConfigurationFileName);
      screenReportFileName.append("_");
      screenReportFileName.append(reportNumberStr);    
      screenReportFileName.append(".tex");

      generateScreenerLaTeXReport(
        indexStart,
        indexEnd,
        screenReportConfig, 
        metricDataSet,
        screenSummaryPlots,
        tickerReportFolder,
        screenerReportFolder,
        screenReportFileName,
        verbose);

    }






  }

  return 0;
}
