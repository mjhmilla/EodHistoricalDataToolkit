
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



struct MetricSummaryDataSet{
  std::vector< std::string > ticker;
  std::vector< std::vector < date::sys_days > > date;  
  std::vector< std::vector< double > > metric;
  std::vector< std::vector< int > > metricRank;
  std::vector< double > metricRankSum;
  std::vector< size_t > rank;
  std::vector< size_t > sortedIndex;
  std::vector < std::vector < PlottingFunctions::SummaryStatistics > > summaryStatistics;
  std::vector < double > weight;
};

// Fun example to get the ranked index of an entry
// https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes

template <typename T>
std::vector<size_t> calcSortedVectorIndicies(const std::vector<T> &v, 
                                            bool sortAscending)
{
  std::vector<size_t> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);
  if (sortAscending)
  {
    std::stable_sort(idx.begin(), idx.end(),
                     [&v](size_t i1, size_t i2)
                     { return v[i1] < v[i2]; });
  }
  else
  {
    std::stable_sort(idx.begin(), idx.end(),
                     [&v](size_t i1, size_t i2)
                     { return v[i1] > v[i2]; });
  }
  return idx;
};

//==============================================================================
template <typename T>
std::vector< size_t > getRank(const std::vector<T> &indiciesOfSortedVector){

  std::vector<size_t> rank(indiciesOfSortedVector.size());
  for(size_t i = 0; i < indiciesOfSortedVector.size(); ++i){
    rank[indiciesOfSortedVector[i]] = i;
  }     
  return rank;                           

};

//==============================================================================

void rankMetricData(const nlohmann::ordered_json &marketReportConfig, 
                    MetricSummaryDataSet &metricDataSetUpd,             
                    bool verbose)
{

  if(verbose){
    std::cout << std::endl << "Ranking metric data ..." << std::endl;
  }

  //Evaluate the ranking of the individual metrics
  size_t indexMetric=0;

  metricDataSetUpd.rank.resize(metricDataSetUpd.metric.size());
  metricDataSetUpd.metricRankSum.resize(metricDataSetUpd.metric.size());
  metricDataSetUpd.sortedIndex.resize(metricDataSetUpd.metric.size());

  metricDataSetUpd.metricRank.resize(metricDataSetUpd.metric.size());
  for(size_t i=0; i<metricDataSetUpd.metric.size();++i){
    metricDataSetUpd.metricRank[i].resize(metricDataSetUpd.metric[i].size());
    metricDataSetUpd.rank[i]         =0;
    metricDataSetUpd.metricRankSum[i]=0;
    metricDataSetUpd.sortedIndex[i]  =0;
  }

  for( auto const &rankingItem : marketReportConfig["ranking"].items()){
    //Retrieve the metric data
    std::string metricName(rankingItem.key());

    double lowerBound = 
      JsonFunctions::getJsonFloat(rankingItem.value()["lowerBound"]);

    double upperBound = 
      JsonFunctions::getJsonFloat(rankingItem.value()["upperBound"]);

    std::string direction;
    JsonFunctions::getJsonString(rankingItem.value()["direction"],direction);

    bool sortAscending=false;
    if(direction.compare("smallestIsBest")==0){
      sortAscending=true;
    }else if(direction.compare("biggestIsBest")==0){
      sortAscending=false;
    }else{
      std::cout << "Error: ranking metric " 
                << metricName
                << " at index " << indexMetric
                << " has a sort direction of "
                << " but this field can only be smallestIsBest/biggestIsBest"
                << std::endl;
      std::abort();              
    }

    std::vector< double > column;
    for(size_t indexRow=0; 
      indexRow < metricDataSetUpd.metric.size(); ++indexRow ){
      
      double value = metricDataSetUpd.metric[indexRow][indexMetric];

      if(!std::isnan(lowerBound)){
        if(value < lowerBound){
          value = std::numeric_limits<double>::max();
        }
      }
      if(!std::isnan(upperBound)){
        if(value > upperBound){
          value = std::numeric_limits<double>::max();
        }
      }
      if(std::isnan(value)){
        if(sortAscending){
          if(!std::isnan(upperBound)){
            value = upperBound;
          }else{
            value = std::numeric_limits<double>::max();
          }
        }else{
          if(!std::isnan(lowerBound)){
            value = lowerBound;
          }else{
            value = -std::numeric_limits<double>::max();
          }
        }
      }
      column.push_back(value);
    }

    std::vector< size_t > indicesOfSortedVector = 
      calcSortedVectorIndicies(column, sortAscending);

    std::vector< size_t > columnRank = getRank(indicesOfSortedVector);

    double weight = 
      JsonFunctions::getJsonFloat(rankingItem.value()["weight"]);


    for(size_t indexRow=0; indexRow < column.size(); ++indexRow){
      metricDataSetUpd.metricRank[indexRow][indexMetric] = columnRank[indexRow];
      double value = 
        static_cast<double>(metricDataSetUpd.metricRank[indexRow][indexMetric]);
      value *= weight;
      metricDataSetUpd.metricRankSum[indexRow] += value;
    }
    ++indexMetric;

  }

  metricDataSetUpd.sortedIndex = 
    calcSortedVectorIndicies(metricDataSetUpd.metricRankSum, true);
  metricDataSetUpd.rank = getRank(metricDataSetUpd.sortedIndex);    


  bool here=true;

};

//==============================================================================

bool appendMetricData(const std::string &tickerFileName,  
                  const std::string &fundamentalFolder,                 
                  const std::string &historicalFolder,
                  const std::string &calculateDataFolder,
                  const nlohmann::ordered_json &marketReportConfig, 
                  const date::year_month_day &targetDate,
                  int maxTargetDateErrorInDays,
                  MetricSummaryDataSet &metricDataSetUpd,             
                  bool verbose){

  bool inputsAreValid=true;

  bool rankingFieldExists = marketReportConfig.contains("ranking");

  bool weightFieldExists = marketReportConfig.contains("market_weighting");
  

  if(rankingFieldExists){

    std::vector< double > metricVector;
    std::vector< date::sys_days > dateVector;
    std::vector< PlottingFunctions::SummaryStatistics> percentileVector;
    double weight=1.0;

    nlohmann::ordered_json targetJsonTable;
    bool loadedCalculateData = false;

    for( auto const &rankingItem : marketReportConfig["ranking"].items()){
      //Retrieve the metric data
      std::string metricName(rankingItem.key());

      std::string folder; 
      JsonFunctions::getJsonString(rankingItem.value()["folder"],folder);

      std::vector< std::string > fieldVector;
      std::string fieldName("");
      for( auto const &fieldEntry : rankingItem.value()["field"]){
        JsonFunctions::getJsonString(fieldEntry,fieldName);
        fieldVector.push_back(fieldName);
      }      

      // Fetch the target data file
      std::string fullPathFileName("");      
      if(folder == "calculateData"){
        fullPathFileName = calculateDataFolder;                      
      }else{
        std::cerr << "Error: in ranking " << metricName 
                  << " the folder field is " << folder 
                  << " but should calculateData" 
                  << std::endl;
        std::abort();                  
      }

      fullPathFileName.append(tickerFileName);      

      //Only load this file once.
      if(!loadedCalculateData){
        loadedCalculateData = 
          JsonFunctions::loadJsonFile(fullPathFileName,
                                      targetJsonTable,
                                      verbose); 
      }                                    

      // Read in all of the data for the metric.
      // Each of the different file types have different formats and so each 
      // needs its own function to load the data

      if(loadedCalculateData){


        int smallestDayError = std::numeric_limits< int >::max();
        date::sys_days targetDay = targetDate;
        date::sys_days closestDay = date::year{1900}/1/1;
        std::string closestDate;
        double targetMetricValue = 0.;

        std::vector< double > metricData;

        for(auto &metricItem : targetJsonTable.items()){
          std::string itemDate(metricItem.key());
          std::istringstream itemDateStream(itemDate);
          itemDateStream.exceptions(std::ios::failbit);
          date::sys_days itemDays;
          itemDateStream >> date::parse("%Y-%m-%d",itemDays);

          double metricValue = 
            JsonFunctions::getJsonFloat(targetJsonTable[itemDate],fieldVector);
          bool metricValueValid = JsonFunctions::isJsonFloatValid(metricValue);

          if(metricValueValid){
            metricData.push_back(metricValue);          
          }

          int dayError = (targetDay-itemDays).count();
          if(dayError < smallestDayError && dayError >= 0 && metricValueValid){
            smallestDayError  = dayError;
            closestDay        = itemDays;
            targetMetricValue = metricValue;
            closestDate       = itemDate;
          }
            
        }

        if(weightFieldExists){
          std::string folderWeighting; 
          JsonFunctions::getJsonString( 
            marketReportConfig["market_weighting"]["folder"],folderWeighting);

          if( (folderWeighting.compare("calculateData") != 0) ){
            std::cout << "Error: market_weighting:folder in the "
                      << "market configuration  file should be calculateData "
                      << " but is instead " << folderWeighting
                      << std::endl;
          }

          std::vector< std::string > fieldAddress;
          for(auto &el : marketReportConfig["market_weighting"]["field"]){
            fieldAddress.push_back(el);
          }

          weight = JsonFunctions::getJsonFloat(
                        targetJsonTable[closestDate],fieldAddress);

        }

        sciplot::Vec metricDataVec(metricData.size());
        for(size_t i=0; i< metricData.size(); ++i){
          metricDataVec[i] = metricData[i];
        }
                                
        PlottingFunctions::SummaryStatistics percentileSummary;
        PlottingFunctions::extractSummaryStatistics(metricDataVec,
                                                    percentileSummary); 
        percentileSummary.current = targetMetricValue;                                                    

        if(smallestDayError <= maxTargetDateErrorInDays){
          metricVector.push_back(targetMetricValue);
        }else{
          metricVector.push_back(
              std::numeric_limits< double >::signaling_NaN());
        }
        dateVector.push_back(closestDay);
        percentileVector.push_back(percentileSummary);
      }else{
        inputsAreValid=false;
      }
    }
    if(inputsAreValid){
      metricDataSetUpd.ticker.push_back(tickerFileName);
      metricDataSetUpd.date.push_back(dateVector);
      metricDataSetUpd.metric.push_back(metricVector);
      metricDataSetUpd.summaryStatistics.push_back(percentileVector);
      metricDataSetUpd.weight.push_back(weight);
    }
  }


  return inputsAreValid;
}

//==============================================================================

bool applyFilter(const std::string &tickerFileName,  
                 const std::string &fundamentalFolder,
                 const std::string &historicalFolder,
                 const std::string &calculateDataFolder,
                 const nlohmann::ordered_json &marketReportConfig,
                 bool replaceNansWithMissingData,
                 bool verbose){

  bool valueFilter = true;

  bool filterFieldExists = marketReportConfig.contains("filter");

  if(filterFieldExists){
    for( auto const &el : marketReportConfig["filter"].items() ){
      //
      // Retreive the filter data
      //
      std::string filterName(el.key());

      std::string folder; 
      JsonFunctions::getJsonString(el.value()["folder"],folder);

      std::vector< std::string > fieldVector;
      std::string fieldName("");
      for( auto const &fieldEntry : el.value()["field"]){
        JsonFunctions::getJsonString(fieldEntry,fieldName);
        fieldVector.push_back(fieldName);
      }

      std::string conditionName("");
      JsonFunctions::getJsonString(el.value()["condition"],conditionName);

      std::string operatorName("");
      JsonFunctions::getJsonString(el.value()["operator"],operatorName);

      std::string valueType("");
      JsonFunctions::getJsonString(el.value()["valueType"],valueType);


      std::vector< std::string > valueStringVector;
      std::string valueStringName;
      if(valueType == "string"){
        for( auto const &fieldEntry : el.value()["values"]){
          JsonFunctions::getJsonString(fieldEntry,valueStringName);
          valueStringVector.push_back(valueStringName);
        }  
      }

      std::vector< double > valueFloatVector;
      double valueFloatName;
      if(valueType == "float"){
        for( auto const &fieldEntry : el.value()["values"]){
          valueFloatName = JsonFunctions::getJsonFloat(fieldEntry);
          valueFloatVector.push_back(valueFloatName);
        }  
      }

      //
      // Fetch the target data and apply the filter
      //
      std::string fullPathFileName("");      
      if(folder == "fundamentalData"){
        fullPathFileName = fundamentalFolder;                
      }else if(folder == "historicalData"){
        fullPathFileName = historicalFolder;                        
      }else if(folder == "calculateData"){
        fullPathFileName = calculateDataFolder;                      
      }else{
        std::cerr << "Error: in filter " << filterName 
                  << " the folder field is " << folder 
                  << " but should be fundamentalData, historicalData,"
                  << " or calculateData" 
                  << std::endl;
        std::abort();                  
      }

      fullPathFileName.append(tickerFileName);

      nlohmann::ordered_json targetJsonTable;
      bool loadedConfiguration = 
        JsonFunctions::loadJsonFile(fullPathFileName,
                                    targetJsonTable,
                                    verbose); 

      std::string valueString;
      double valueFloat;
      std::vector < bool > valueBoolVector;

      if(valueType == "string"){
        JsonFunctions::getJsonString(targetJsonTable, fieldVector, valueString);

        //Evaluate all of the individual values against the target
        for(size_t i = 0; i <  valueStringVector.size(); ++i){
          if(conditionName == "=="){
            bool equalityTest = (valueString == valueStringVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "!="){
            bool equalityTest = (valueString != valueStringVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else{
            std::cerr << "Error: in filter " << filterName 
                  << " the condition field should be " 
                  << " == or != for a string"
                  << " but is instead " << conditionName  
                  << std::endl;
            std::abort();   
          }        
        }

      }else if(valueType == "float"){
        valueFloat = JsonFunctions::getJsonFloat(targetJsonTable, fieldVector,
                                                 replaceNansWithMissingData);

        //Evaluate all of the individual values against the target
        for(size_t i = 0; i <  valueFloatVector.size(); ++i){
          if(conditionName == "=="){
            bool equalityTest = (valueFloat == valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "!="){
            bool equalityTest = (valueFloat != valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == ">"){
            bool equalityTest = (valueFloat > valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "<"){
            bool equalityTest = (valueFloat < valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == ">="){
            bool equalityTest = (valueFloat >= valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "<="){
            bool equalityTest = (valueFloat <= valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else{
            std::cerr << "Error: in filter " << filterName 
                  << " the condition field should be one of" 
                  << " (==, !=, >, <, >=, <=) for a float"
                  << " but is instead " << conditionName  
                  << std::endl;
            std::abort();   
          }        
        }


      }else{
        std::cerr << "Error: in filter " << filterName 
                  << " valueType should be string or float but is instead" 
                  << valueType 
                  << std::endl;        
      }


      //Apply the operator between all of the values to yield the final 
      //filter value
      valueFilter = valueBoolVector[0];
      for(size_t i = 1; i < valueBoolVector.size(); ++i){
        if(operatorName == "||"){
          valueFilter = (valueFilter || valueBoolVector[i]);
        }else if( operatorName == "&&"){
          valueFilter = (valueFilter && valueBoolVector[i]);
        }else{
          std::cerr << "Error: in filter " << filterName 
                << " the operator field should be " 
                << " || or && for a string"
                << " but is instead " << operatorName  
                << std::endl;
          std::abort();             
        }
      }



    }


  }

  return valueFilter;
};

//==============================================================================
void plotReportData(const nlohmann::ordered_json &marketReportConfig, 
                    const MetricSummaryDataSet &metricDataSet,
                    const PlottingFunctions::PlotSettings &settings,
                    const std::string &marketReportFolder,
                    const std::string &summaryPlotFileName,
                    bool verbose)
{

  bool rankingFieldExists = marketReportConfig.contains("ranking");

  double canvasWidth=0.;
  double canvasHeight=0.;


  if(rankingFieldExists){

    size_t numberOfRankingItems = marketReportConfig["ranking"].size();
    std::vector< std::vector< sciplot::PlotVariant > > arrayOfPlotVariant;


    std::vector< std::vector< sciplot::Plot2D >> arrayOfPlot2D;
    arrayOfPlot2D.resize(numberOfRankingItems);
    for(size_t i=0; i<numberOfRankingItems; ++i){
      arrayOfPlot2D[i].resize(1);
    }

    size_t indexMetric=0;

    for( auto const &rankingItem : marketReportConfig["ranking"].items()){
      
      PlottingFunctions::PlotSettings subplotSettings = settings;

      PlottingFunctions::SummaryStatistics marketSummary;   
      marketSummary.percentiles.resize(
          PlottingFunctions::PercentileIndices::NUM_PERCENTILES,0.);

      double marketWeight = 0;

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
      xThreshold[0] = 0.0;
      xThreshold[1] = static_cast<double>(metricDataSet.ticker.size())+2.0;
      yThreshold[0] = threshold;
      yThreshold[1] = threshold;

      arrayOfPlot2D[indexMetric][0].drawCurve(xThreshold,yThreshold)
        .lineColor("gray")
        .lineWidth(settings.lineWidth*0.5)
        .labelNone();

      for( size_t i=0; i<metricDataSet.ticker.size(); ++i){

        size_t indexSorted = metricDataSet.sortedIndex[i];

        double xMid = static_cast<double>(i)+1.0;
        double xWidth = 0.4;

        std::string currentColor("black");
        std::string boxColor("light-gray");
        int currentLineType = 0;

        if(smallestIsBest && 
           metricDataSet.summaryStatistics[indexSorted][indexMetric].current 
           <= threshold){
          currentLineType = 1;
        }
        if(biggestIsBest && 
           metricDataSet.summaryStatistics[indexSorted][indexMetric].current 
            >= threshold){    
          currentLineType = 1;      
        }

        PlottingFunctions::drawBoxAndWhisker(
          arrayOfPlot2D[indexMetric][0],
          xMid,
          xWidth,
          metricDataSet.summaryStatistics[indexSorted][indexMetric],
          boxColor.c_str(),
          currentColor.c_str(),
          currentLineType,
          subplotSettings,
          verbose);

        for(size_t j=0; j < marketSummary.percentiles.size();++j){
          marketSummary.percentiles[j] += 
            metricDataSet.summaryStatistics[indexSorted][indexMetric].percentiles[j]
            *metricDataSet.weight[indexSorted];
        }

        marketSummary.current +=
          metricDataSet.summaryStatistics[indexSorted][indexMetric].current
          *metricDataSet.weight[indexSorted];

        marketSummary.min +=  
          metricDataSet.summaryStatistics[indexSorted][indexMetric].min
          *metricDataSet.weight[indexSorted];

        marketSummary.max +=  
          metricDataSet.summaryStatistics[indexSorted][indexMetric].max
          *metricDataSet.weight[indexSorted];

        marketWeight += metricDataSet.weight[indexSorted];
      }

      //Plot the market summary
      for(size_t j=0; j < marketSummary.percentiles.size();++j){
        marketSummary.percentiles[j] = marketSummary.percentiles[j] / marketWeight;
      }
      marketSummary.min     = marketSummary.min / marketWeight; 
      marketSummary.max     = marketSummary.max / marketWeight; 
      marketSummary.current = marketSummary.current / marketWeight; 


      std::string currentColor("black");
      std::string boxColor("chartreuse");
      int currentLineType = 0;

      if(smallestIsBest && marketSummary.current <= threshold){
          currentLineType=1;
      }
      if(biggestIsBest && marketSummary.current  >= threshold){
          currentLineType=1;
      }

      double xMid = metricDataSet.ticker.size()+1.0;
      double xWidth = 0.4;
      PlottingFunctions::drawBoxAndWhisker(
        arrayOfPlot2D[indexMetric][0],
        xMid,
        xWidth,
        marketSummary,
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
        arrayOfPlot2D[indexMetric][0],
        xAxisLabel,
        yAxisLabel,
        subplotSettings);

      if(!std::isnan(lowerBoundPlot) && !std::isnan(upperBoundPlot) ){
        arrayOfPlot2D[indexMetric][0].yrange(lowerBoundPlot,upperBoundPlot);
      }

      arrayOfPlot2D[indexMetric][0].legend().hide();
      
      std::vector< sciplot::PlotVariant > rowOfPlotVariant;      
      rowOfPlotVariant.push_back(arrayOfPlot2D[indexMetric][0]);
      arrayOfPlotVariant.push_back(rowOfPlotVariant);

      ++indexMetric;
    } 

    sciplot::Figure figSummary(arrayOfPlotVariant);
    figSummary.title("Summary");
    sciplot::Canvas canvas = {{figSummary}};

    canvas.size(canvasWidth, canvasHeight) ;    

    // Save the figure to a PDF file
    std::string outputFileName = marketReportFolder;
    outputFileName.append(summaryPlotFileName);
    canvas.save(outputFileName);     
  }

};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string calculateDataFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string marketReportConfigurationFilePath;  
  std::string tickerReportFolder;
  std::string marketReportFolder;  
  std::string dateOfTable;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will produce a market report in the form of "
    "text and tables about the companies that are best ranked."
    ,' ', "0.0");

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");
    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> tickerReportFolderOutput("t","ticker_report_folder_path", 
      "The path to the folder will contains the ticker reports.",
      true,"","string");
    cmd.add(tickerReportFolderOutput);

    TCLAP::ValueArg<std::string> marketReportFolderOutput("o","market_report_folder_path", 
      "The path to the folder will contains the market report.",
      true,"","string");
    cmd.add(marketReportFolderOutput);    

    TCLAP::ValueArg<std::string> calculateDataFolderInput("a","calculate_folder_path", 
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

    TCLAP::ValueArg<std::string> marketReportConfigurationFilePathInput("c",
      "market_report_configuration_file", 
      "The path to the json file that contains the names of the metrics "
      " to plot.",
      true,"","string");

    cmd.add(marketReportConfigurationFilePathInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode              = exchangeCodeInput.getValue();  
    marketReportConfigurationFilePath 
                              = marketReportConfigurationFilePathInput.getValue();      
    calculateDataFolder       = calculateDataFolderInput.getValue();
    fundamentalFolder         = fundamentalFolderInput.getValue();
    historicalFolder          = historicalFolderInput.getValue();    
    tickerReportFolder        = tickerReportFolderOutput.getValue();
    marketReportFolder        = marketReportFolderOutput.getValue();    
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

      std::cout << "  Market Report Configuration File" << std::endl;
      std::cout << "    " << marketReportConfigurationFilePath << std::endl;          

      std::cout << "  Market Report Folder" << std::endl;
      std::cout << "    " << marketReportFolder << std::endl;  

    }

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  bool replaceNansWithMissingData = true;

  //Load the report configuration file
  nlohmann::ordered_json marketReportConfig;
  bool loadedConfiguration = 
    JsonFunctions::loadJsonFile(marketReportConfigurationFilePath,
                                marketReportConfig,
                                verbose);
                                  

  if(!loadedConfiguration){
    std::cerr << "Error: cannot open " << marketReportConfigurationFilePath 
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

      //Check to see if this ticker passes the filter
      bool tickerPassesFilter=false;
      if(validInput){
        tickerPassesFilter = applyFilter( fileName,  
                                  fundamentalFolder,
                                  historicalFolder,
                                  calculateDataFolder,
                                  marketReportConfig,
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
    auto today = date::floor<date::days>(std::chrono::system_clock::now());
    date::year_month_day targetDate(today);
    int maxTargetDateErrorInDays = 365;

    MetricSummaryDataSet metricDataSet;    
    if(filteredTickers.size() > 0){
      for (size_t i=0; i<filteredTickers.size(); ++i){
        

        bool appendedMetricData = 
            appendMetricData(filteredTickers[i],  
                        fundamentalFolder,                 
                        historicalFolder,
                        calculateDataFolder,
                        marketReportConfig,
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

      rankMetricData(marketReportConfig,metricDataSet,verbose);

    }

    std::string marketReportConfigurationFileName =
      std::filesystem::path(marketReportConfigurationFilePath).filename();
    size_t idx = marketReportConfigurationFileName.find(".json");
    marketReportConfigurationFileName 
      = marketReportConfigurationFileName.substr(0,idx);

    std::string summaryPlotFileName("summary_");
    summaryPlotFileName.append(marketReportConfigurationFileName);
    summaryPlotFileName.append(".pdf");
    
    PlottingFunctions::PlotSettings settings;

    plotReportData(marketReportConfig, 
                   metricDataSet,
                   settings,
                   marketReportFolder,
                   summaryPlotFileName,
                   verbose);

    bool here=true;                   

  }

  return 0;
}
