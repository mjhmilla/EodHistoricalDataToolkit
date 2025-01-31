//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <limits>
#include <filesystem>

#include <cassert>

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


void appendComparisonData(
  const nlohmann::ordered_json &comparisonConfig, 
  const std::vector< ScreenerToolkit::MetricSummaryDataSet > &metricSummaryDataSet,
  ScreenerToolkit::MetricSummaryDataSet &metricComparisonDataSetUpd)
{
    

  size_t i=0;

  for(auto &screen: comparisonConfig.items()){
    metricComparisonDataSetUpd.ticker.push_back(screen.key());

    //
    // Evaluate the weighted average of each metric value and summary statistics
    // Ignore the date field
    //
    double weightTotal = 0.;
    std::vector< double > metricAvg;
    std::vector< PlottingFunctions::SummaryStatistics > summaryStatsAvg;

    for(size_t k=0; k<metricSummaryDataSet[i].metricRank[0].size(); ++k){
      metricAvg.push_back(0.);
      PlottingFunctions::SummaryStatistics summaryStats;
      summaryStats.min = 0;
      summaryStats.max = 0;
      summaryStats.current = 0;
      for(size_t k=0; k<PlottingFunctions::NUM_PERCENTILES;++k){
        summaryStats.percentiles[k]=0.;
      }
      summaryStatsAvg.push_back(summaryStats);
    }

    for(size_t j=0; j< metricSummaryDataSet[i].ticker.size(); ++j){
      double weight = metricSummaryDataSet[i].weight[j];
      weightTotal += weight;
      
      for(size_t k=0; k<metricSummaryDataSet[i].metricRank[j].size(); ++k){
        metricAvg[k] += (metricSummaryDataSet[i].metricRank[j][k])*weight;

        summaryStatsAvg[k].min += 
          metricSummaryDataSet[i].summaryStatistics[j][k].min * weight;
        summaryStatsAvg[k].max += 
          metricSummaryDataSet[i].summaryStatistics[j][k].max * weight;
        summaryStatsAvg[k].current += 
          metricSummaryDataSet[i].summaryStatistics[j][k].current * weight;
        for(size_t x=0; x<PlottingFunctions::NUM_PERCENTILES;++x){
          summaryStatsAvg[k].percentiles[x] += 
            metricSummaryDataSet[i].summaryStatistics[j][k].percentiles[x];
        }


      }
    }

    for(size_t k=0; k<metricSummaryDataSet[i].metricRank[0].size(); ++k){
      metricAvg[k] = metricAvg[k] / weightTotal;
      summaryStatsAvg[k].min = summaryStatsAvg[k].min / weightTotal; 
      summaryStatsAvg[k].max = summaryStatsAvg[k].max / weightTotal;
      summaryStatsAvg[k].current = summaryStatsAvg[k].current / weightTotal; 

      for(size_t x=0; x<PlottingFunctions::NUM_PERCENTILES;++x){
        summaryStatsAvg[k].percentiles[x] = 
          summaryStatsAvg[k].percentiles[x] / weightTotal;
      }

    }

    double weightAvg = weightTotal 
      / static_cast<double>(metricSummaryDataSet[i].ticker.size());

    metricComparisonDataSetUpd.metric.push_back(metricAvg);
    metricComparisonDataSetUpd.weight.push_back(weightAvg);
    metricComparisonDataSetUpd.summaryStatistics.push_back(summaryStatsAvg);


    ++i;
  }  

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

  for(size_t itemVar=0; itemVar < numberOfVariations; ++itemVar){

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

      //Get the new value
      auto &valueItem = updItem.value()["values"].at(itemVar);

      //Flatten the structure so we that we can update it independent of its 
      //structure
      std::string keyFlattened;
      for(size_t i=0; i<field.size();++i){
        keyFlattened.append("/");
        keyFlattened.append(field[i]);
      }      
      auto screenEntryFlat = screenEntry.flatten();

      //Retrieve the element to update from the template to determine whether
      //its an array or not.
      nlohmann::ordered_json eleTemplate;
      JsonFunctions::getJsonElement(
          configTemplate["screen_template"],field,eleTemplate);

      if(eleTemplate.is_array()){
        keyFlattened.append("/0");
        screenEntryFlat[keyFlattened] = valueItem;
      }else{
        screenEntryFlat[keyFlattened] = valueItem;
      }

      screenEntry=screenEntryFlat.unflatten();

      std::string screenName = configTemplate["screen_names"].at(itemVar);

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
  bool plottingSizeConsistent=true;
  int numberOfScreens = 0;  
  int numberOfRankingItems=0;
  int numberOfPlottingItems=0;
  

  if(screenFieldExists){
    numberOfScreens = comparisonConfig["screens"].size();

    for(auto &screenItem : comparisonConfig["screens"].items()){
      if(!screenItem.value().contains("ranking")){
        rankingFieldExists=false;
      }
      if(numberOfRankingItems==0){
        numberOfRankingItems = screenItem.value()["ranking"].size();
        for(auto &rankingItem : screenItem.value()["ranking"].items()){
          bool addPlot = 
            JsonFunctions::getJsonBool(
              rankingItem.value()["plotSettings"]["addPlot"]);   
          if(addPlot){
            ++numberOfPlottingItems;
          }          
        }
      }else{
        if(numberOfRankingItems != screenItem.value()["ranking"].size()){
          rankingSizeConsistent=false;
        }
        int tempNumberOfPlottingItems=0;
        for(auto &rankingItem : screenItem.value()["ranking"].items()){
          bool addPlot = 
            JsonFunctions::getJsonBool(
              rankingItem.value()["plotSettings"]["addPlot"]);   
          if(addPlot){
            ++tempNumberOfPlottingItems;
          }          
        }
        if(numberOfPlottingItems != tempNumberOfPlottingItems){
          plottingSizeConsistent=false;
        }        
      }      
    } 
  }

  if(screenFieldExists && rankingFieldExists 
    && rankingSizeConsistent && plottingSizeConsistent){

    
    std::vector< std::vector< sciplot::PlotVariant > > arrayOfPlotVariant;

    std::vector< std::vector< sciplot::Plot2D >> arrayOfPlot2D;
    arrayOfPlot2D.resize(numberOfPlottingItems);
    for(size_t i=0; i<numberOfPlottingItems; ++i){
      arrayOfPlot2D[i].resize(1);
    }

;  
    std::vector< bool > axisLabelsAdded;
    axisLabelsAdded.resize(numberOfPlottingItems);
    for(size_t i=0;i<numberOfPlottingItems;++i){
      axisLabelsAdded[i]=false;
    }

    size_t indexScreen=0;

    for(auto const &screenItem : comparisonConfig["screens"].items()){

      bool isValid = true;
      if(metricSummaryDataSet[indexScreen].ticker.size() == 0){
        isValid = false;
      }

      if(isValid && indexScreen >= indexStart && indexScreen < indexEnd){


        int numberOfTickersPerScreen = 
          metricSummaryDataSet[indexScreen].ticker.size();

        double tmp = 
          JsonFunctions::getJsonFloat(
            comparisonConfig["report"]["number_of_tickers_per_screen"],false);

        if(JsonFunctions::isJsonFloatValid(tmp) 
          && static_cast<int>(tmp) < numberOfTickersPerScreen
          && static_cast<int>(tmp) > 0){
          numberOfTickersPerScreen = static_cast<int>(tmp);
        }


        int indexRanking=0;
        int indexPlotting=0;
        for(auto const &rankingItem : screenItem.value()["ranking"].items())
        {

          bool addPlot = 
            JsonFunctions::getJsonBool(
              rankingItem.value()["plotSettings"]["addPlot"]);     

          if(addPlot){
          
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

              arrayOfPlot2D[indexPlotting][0].drawCurve(xThreshold,yThreshold)
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
              arrayOfPlot2D[indexPlotting][0],
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
              arrayOfPlot2D[indexPlotting][0],
              xAxisLabel,
              yAxisLabel,
              subplotSettings); 
            axisLabelsAdded[indexRanking]=true;

            //}

            if(!std::isnan(lowerBoundPlot) && !std::isnan(upperBoundPlot) ){
              arrayOfPlot2D[indexPlotting][0].yrange(lowerBoundPlot,upperBoundPlot);
            }

            arrayOfPlot2D[indexPlotting][0].xrange(
                static_cast<double>(indexStart)-xWidth+1.0,
                static_cast<double>(indexEnd)+xWidth+1.0);

            arrayOfPlot2D[indexPlotting][0].legend().hide();

            ++indexPlotting;          
          }

          ++indexRanking;

        }
      }
      ++indexScreen;
    }

    double canvasWidth  = 0.;
    double canvasHeight = 0.;
    bool isCanvasSizeSet=false;

    for(const auto &screenItem : comparisonConfig["screens"].items()){
      if(!isCanvasSizeSet){
        for(const auto &rankingItem : screenItem.value()["ranking"].items()){
          bool addPlot = 
            JsonFunctions::getJsonBool(
              rankingItem.value()["plotSettings"]["addPlot"]); 

          if(!isCanvasSizeSet && addPlot){
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
    }

    for(size_t indexPlotting=0; 
        indexPlotting < numberOfPlottingItems; ++indexPlotting){

      std::vector< sciplot::PlotVariant > rowOfPlotVariant;      
      rowOfPlotVariant.push_back(arrayOfPlot2D[indexPlotting][0]);
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

void generateComparisonLaTeXReport(
      size_t indexStart,
      size_t indexEnd,
      const nlohmann::ordered_json &comparisonConfig, 
      const std::vector< ScreenerToolkit::MetricSummaryDataSet> &metricDataSet,
      const std::vector< std::string > &comparisonSummaryPlots,
      const std::string &tickerReportFolder,
      const std::string &comparisonReportFolder,
      const std::string &comparisonReportFileName,
      bool verbose){

  if(verbose){
    std::cout << "----------------------------------------" << std::endl;    
    std::cout << "Generating LaTeX report: " << comparisonReportFileName
              << std::endl;
    std::cout << "----------------------------------------" << std::endl;              
  }


  std::string outputReportPath(comparisonReportFolder);
  outputReportPath.append(comparisonReportFileName);

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
    assert(false);  
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
  JsonFunctions::getJsonString(comparisonConfig["report"]["title"],reportTitle);
  latexReport << "\\title{" << reportTitle <<"}" << std::endl;

  latexReport << std::endl << std::endl;
  latexReport << "\\begin{document}" << std::endl << std::endl;
  latexReport << "\\maketitle" << std::endl;
  latexReport << "\\today" << std::endl;

  for(auto const &figName : comparisonSummaryPlots){
    latexReport << "\\begin{figure}[h]" << std::endl;
    latexReport << "  \\begin{center}" << std::endl;
    latexReport << "    \\includegraphics{"
                << comparisonReportFolder << figName
                << "}" << std::endl;
    latexReport << " \\end{center}" << std::endl;
    latexReport << "\\end{figure}"<< std::endl;
    latexReport << std::endl;    
  }

  // Add the list of screens
  latexReport << "\\begin{multicols}{2}" << std::endl;
  latexReport << "\\begin{enumerate}" << std::endl;
  latexReport << "\\setcounter{enumi}{" << indexStart <<"}" << std::endl;  
  latexReport << "\\itemsep0pt" << std::endl;

  size_t i=0;


  for(auto &itemScreen: comparisonConfig["screens"].items()){

    //Since it's not possible directly access an element and then
    //get its name, here we iterate until we get to the right spot
    //
    //https://github.com/nlohmann/json/issues/1936

    if(i >= indexStart && i < indexEnd){

      std::string screenName=itemScreen.key();
      std::string screenLabel(screenName);

      ReportingFunctions::sanitizeStringForLaTeX(screenName,true);
      ReportingFunctions::sanitizeLabelForLaTeX(screenLabel,true);

      int numberOfTickers = metricDataSet[i].ticker.size();
      double tmp = JsonFunctions::getJsonFloat(
          comparisonConfig["report"]["number_of_tickers_per_screen"]);    
      
      if(JsonFunctions::isJsonFloatValid(tmp) 
          && static_cast<int>(tmp) > 0
          && static_cast<int>(tmp) < numberOfTickers){
        numberOfTickers = static_cast<int>(tmp);
      }


      latexReport << "\\item " << "\\ref{" << screenLabel << "} "
                  <<  screenName 
                  << "---" << numberOfTickers  
                  << std::endl;
    }
    ++i;                
  }
  latexReport << "\\end{enumerate}" << std::endl;
  latexReport << "\\end{multicols}" << std::endl;
  latexReport << std::endl;
  
  // Append the lists of tickers in each screen
  i=0;
  

  latexReport << "\\setcounter{section}{" << indexStart << "}" << std::endl;
  for(auto &itemScreen : comparisonConfig["screens"].items()){

    if(i >= indexStart && i < indexEnd){

      int numberOfTickers = metricDataSet[i].ticker.size();
      double tmp = JsonFunctions::getJsonFloat(
          comparisonConfig["report"]["number_of_tickers_per_screen"]);    
      
      if(JsonFunctions::isJsonFloatValid(tmp) 
        && static_cast<int>(tmp) > 0
        && static_cast<int>(tmp) < numberOfTickers){
        numberOfTickers = static_cast<int>(tmp);
      }
  
      std::string screenName=itemScreen.key();
      std::string screenLabel(screenName);

      ReportingFunctions::sanitizeStringForLaTeX(screenName,true);
      ReportingFunctions::sanitizeLabelForLaTeX(screenLabel,true);

      latexReport << "\\section{" << screenName << "}" << std::endl;
      latexReport << "\\label{" << screenLabel << "}" << std::endl;

      if(numberOfTickers > 0){
        latexReport << "\\begin{multicols}{3}" << std::endl;
        latexReport << "\\begin{enumerate}" << std::endl;
        latexReport << "\\setcounter{enumi}{" << 0 <<"}" << std::endl;  
        latexReport << "\\itemsep0pt" << std::endl;

        for(size_t j=0; j < numberOfTickers; ++j){
          size_t k = metricDataSet[i].sortedIndex[j];

          std::string tickerString(metricDataSet[i].ticker[k]);
          std::string tickerLabel(metricDataSet[i].ticker[k]);
          std::string tickerFile(metricDataSet[i].ticker[k]);

          ReportingFunctions::sanitizeStringForLaTeX(tickerString,true);
          ReportingFunctions::sanitizeLabelForLaTeX(tickerLabel,true);
          ReportingFunctions::sanitizeFolderName(tickerFile,true);

          latexReport << "\\item " 
                      <<  tickerString
                      << "--- " << metricDataSet[i].metricRankSum[k] 
                      << std::endl;
        }


        latexReport << "\\end{enumerate}" << std::endl;
        latexReport << "\\end{multicols}" << std::endl;
        latexReport << std::endl;
      }
    }
    ++i;
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

  //Scan to see whether the filter opens fundamental data, historical data
  //and/or calculate data.

  bool useFundamentalData=false;
  bool useHistoricalData=false;
  bool useCalculateData=false;

  for(auto &screenItem : comparisonConfig["screens"].items()){ 
    for(auto &filterItem : screenItem.value()["filter"].items()){
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
  }

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
      int screenCount = 0;
      for(auto &screenItem : comparisonConfig["screens"].items()){  

        tickerPassesFilter = 
          ScreenerToolkit::applyFilter(
            fileName,
            fundamentalData,
            historicalData,
            calculateData,
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

  //Scan to see whether the ranking opens fundamental data, historical data
  //and/or calculate data.

  useFundamentalData=false;
  useHistoricalData=false;
  useCalculateData=false;

  for(auto &screenItem : comparisonConfig["screens"].items()){ 
    for(auto &filterItem : screenItem.value()["ranking"].items()){
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
  }



  std::vector< ScreenerToolkit::MetricSummaryDataSet > metricSummaryDataSet;
  metricSummaryDataSet.resize(tickerSet.size());

  int screenCount = 0;
  for(auto &screenItem : comparisonConfig["screens"].items()){

    if(tickerSet[screenCount].filtered.size()>0){
      for(size_t i=0; i< tickerSet[screenCount].filtered.size();++i){
        //
        //Load the fundamental, historical, and calculate data
        //once so that the necessary ranking information can be retreived
        //without having to load the files again.
        //
        std::string fundamentalDataPath = fundamentalFolder;
        fundamentalDataPath.append(tickerSet[screenCount].filtered[i]);
        nlohmann::ordered_json fundamentalData;
        bool loadedFundamentalData=false;
        if(useFundamentalData){
          loadedFundamentalData = 
            JsonFunctions::loadJsonFile(fundamentalDataPath,
                                        fundamentalData,
                                        verbose);     
        }

        std::string historicalDataPath = historicalFolder;
        historicalDataPath.append(tickerSet[screenCount].filtered[i]);
        nlohmann::ordered_json historicalData;
        bool loadedHistoricalData=false;
        if(useHistoricalData){
          loadedHistoricalData = 
            JsonFunctions::loadJsonFile(historicalDataPath,
                                        historicalData,
                                        verbose);     
        }

        std::string calculateDataPath = calculateDataFolder;
        calculateDataPath.append(tickerSet[screenCount].filtered[i]);
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


        if(validInput){
          bool appendedMetricData = 
            ScreenerToolkit::appendMetricData(
                            tickerSet[screenCount].filtered[i],  
                            fundamentalData,                 
                            historicalData,
                            calculateData,
                            screenItem.value(),
                            targetDate,
                            maxTargetDateErrorInDays,
                            metricSummaryDataSet[screenCount],                        
                            verbose);
        }
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
  int maximumNumberOfReports   = -1;

  if(comparisonConfig.contains("report")){
    if(comparisonConfig["report"].contains("number_of_screens_per_report")){
      double tmp = JsonFunctions::getJsonFloat(
        comparisonConfig["report"]["number_of_screens_per_report"],false);
      if(JsonFunctions::isJsonFloatValid(tmp) 
        && static_cast<int>(tmp) > 0){
        numberOfScreensPerReport = static_cast<int>(tmp);
      }
    }
    if(comparisonConfig["report"].contains("number_of_reports")){
      double tmp = JsonFunctions::getJsonFloat(
        comparisonConfig["report"]["number_of_reports"],false);
      if(JsonFunctions::isJsonFloatValid(tmp)){
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

  if(maximumNumberOfReports < 0){
    maximumNumberOfReports = maximumNumberOfReportsDefault;
  }

  maximumNumberOfReports = std::min(maximumNumberOfReports,
                                    maximumNumberOfReportsDefault);

  int numberOfReportDigits =   
    static_cast<int>(
      std::ceil(
        static_cast<double>(maximumNumberOfReports)/10.0
      )
    );                                  

  for(size_t indexReport=0; indexReport <maximumNumberOfReports; ++indexReport){

    size_t indexStart = (numberOfScreensPerReport)*indexReport;
    size_t indexEnd   = std::min( indexStart+numberOfScreensPerReport,
                                  metricSummaryDataSet.size()       );  

    //Create the plot name
    std::stringstream reportNumber;
    reportNumber << indexReport;
    std::string reportNumberStr(reportNumber.str());

    while(reportNumberStr.length()<numberOfReportDigits){
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


    std::string comparisonReportFileName("report_");
    comparisonReportFileName.append(comparisonReportConfigurationFileName);
    comparisonReportFileName.append("_");
    comparisonReportFileName.append(reportNumberStr);    
    comparisonReportFileName.append(".tex");

    generateComparisonLaTeXReport(
      indexStart,
      indexEnd,
      comparisonConfig, 
      metricSummaryDataSet,
      screenSummaryPlots,
      tickerReportFolder,
      comparisonReportFolder,
      comparisonReportFileName,
      verbose);

  }

  return 0;
}
