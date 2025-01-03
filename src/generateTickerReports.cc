
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

#include <sciplot/sciplot.hpp>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"
#include "UtilityFunctions.h"
#include "ReportingFunctions.h"
#include "PlottingFunctions.h"


//const char* BoxAndWhiskerColorA="web-blue";
//const char* BoxAndWhiskerColorB="gray0";

//==============================================================================
struct LineSettings{
  std::string colour;
  std::string name;
  double lineWidth;
};

struct BoxAndWhiskerSettings{
  double xOffsetFromStart;
  double xOffsetFromEnd;
  std::string boxWhiskerColour;
  std::string currentValueColour;
  BoxAndWhiskerSettings():
    xOffsetFromStart(std::nan("1")),
    xOffsetFromEnd(std::nan("1")),    
    boxWhiskerColour("blue"),
    currentValueColour("black"){};
};


struct AxisSettings{
  std::string xAxisName;
  std::string yAxisName;
  double xMin;
  double xMax;  
  double yMin;
  double yMax;
  AxisSettings():
    xAxisName(""),
    yAxisName(""),
    xMin(std::nan("1")),
    xMax(std::nan("1")),
    yMin(std::nan("1")),
    yMax(std::nan("1")){};

};

struct SubplotSettings{
  size_t indexRow;
  size_t indexColumn;
};


//==============================================================================
struct JsonMetaData{
  //std::string ticker;
  const nlohmann::ordered_json& jsonData;
  std::vector<std::string> address;
  std::string fieldName;
  std::string dateName;
  bool isArray;
  JsonMetaData(const nlohmann::ordered_json &jsonTable)
    :jsonData(jsonTable){};

};

//==============================================================================
struct TickerMetaData{
  std::string primaryTicker;
  std::string companyName;
  std::string currencyCode;
  std::string country;
  std::string isin;
  std::string url;
  TickerMetaData():
    primaryTicker(""),
    companyName(""),
    currencyCode(""),
    country(""),
    isin(""),
    url(""){}
};
//==============================================================================
enum JSON_FIELD_TYPE{
  FLOAT=0,
  STRING,
  BOOL,
  JSON_FIELD_TYPE_SIZE
};

struct JsonFieldAddress{
  std::vector< std::string > fieldNames;
  JSON_FIELD_TYPE type;
  JsonFieldAddress():fieldNames(),type(FLOAT){};
};

//==============================================================================
bool isNameInList(std::string &name, std::vector< std::string > &listOfNames){
  bool nameIsInList = false;
  size_t i = 0;
  while(nameIsInList == false && i < listOfNames.size()){
    if(name.compare(listOfNames[i])==0){
      nameIsInList=true;
    }
    ++i;
  }
  return nameIsInList;
};

//==============================================================================
void getTickerMetaData(    
    const nlohmann::ordered_json &fundamentalData,
    TickerMetaData &tickerMetaDataUpd){

  JsonFunctions::getJsonString( fundamentalData[GEN]["PrimaryTicker"],
                                tickerMetaDataUpd.primaryTicker);

  JsonFunctions::getJsonString( fundamentalData[GEN]["Name"],
                                tickerMetaDataUpd.companyName); 

  //ReportingFunctions::sanitizeStringForLaTeX(tickerMetaDataUpd.companyName);                                

  JsonFunctions::getJsonString( fundamentalData[GEN]["ISIN"],
                                tickerMetaDataUpd.isin);                                

  JsonFunctions::getJsonString( fundamentalData[GEN]["CurrencyCode"],
                                tickerMetaDataUpd.currencyCode);

  //ReportingFunctions::sanitizeStringForLaTeX(tickerMetaDataUpd.currencyCode);

  JsonFunctions::getJsonString(fundamentalData[GEN]["CountryName"],
                               tickerMetaDataUpd.country);  

  //ReportingFunctions::sanitizeStringForLaTeX(tickerMetaDataUpd.country);   

  JsonFunctions::getJsonString(fundamentalData[GEN]["WebURL"],
                               tickerMetaDataUpd.url);  

};
//==============================================================================
//From:
//https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes
template <typename T>
std::vector< size_t > sort_indices(std::vector<T> &v){
  std::vector< size_t > idx(v.size());
  std::iota(idx.begin(),idx.end(),0);

  std::stable_sort(idx.begin(),idx.end(),
                    [&v](size_t i1, size_t i2){return v[i1]<v[i2];});
  std::stable_sort(v.begin(),v.end());

  return idx;
}

//==============================================================================
void findReplaceKeywords(std::string &stringToUpd,
                         const std::vector< std::string > keywords,
                         const std::vector< std::string > replacements)
{
  if(keywords.size() != replacements.size()){
    std::cout << "Error: the keywords and replacement vectors must be the same"
              << " size"
              << std::endl;
    std::abort();              
  }

  for(size_t i=0; i<keywords.size(); ++i){
    size_t idx0 = stringToUpd.find(keywords[i]);
    if(idx0 != std::string::npos){
      stringToUpd.replace(idx0,keywords[i].length(),replacements[i]);
    }
  }

};
//==============================================================================
void getFindAndReplacementVectors(const TickerMetaData &tickerMetaData,
                                  std::vector< std::string > &keywordsUpd,
                                  std::vector< std::string > &replacementUpd)
{

  keywordsUpd.clear();
  replacementUpd.clear();

  keywordsUpd.push_back("@URL");
  keywordsUpd.push_back("@CompanyNameLaTeX");
  keywordsUpd.push_back("@PrimaryTickerLaTeX");
  keywordsUpd.push_back("@CurrencyCodeLaTeX");
  keywordsUpd.push_back("@CountryNameLaTeX");
  keywordsUpd.push_back("@CompanyName");
  keywordsUpd.push_back("@PrimaryTicker");
  keywordsUpd.push_back("@CurrencyCode");
  keywordsUpd.push_back("@CountryName");

  std::string companyNameLatex(tickerMetaData.companyName);
  std::string primaryTickerLatex(tickerMetaData.primaryTicker);
  std::string currencyCodeLatex(tickerMetaData.currencyCode);
  std::string countryNameLatex(tickerMetaData.country);
  std::string companyName(tickerMetaData.companyName);
  std::string primaryTicker(tickerMetaData.primaryTicker);
  std::string currencyCode(tickerMetaData.currencyCode);
  std::string countryName(tickerMetaData.country);

  ReportingFunctions::sanitizeStringForLaTeX(companyNameLatex);
  ReportingFunctions::sanitizeStringForLaTeX(primaryTickerLatex);
  ReportingFunctions::sanitizeStringForLaTeX(currencyCodeLatex);
  ReportingFunctions::sanitizeStringForLaTeX(countryNameLatex);

  ReportingFunctions::sanitizeFolderName(companyName);
  ReportingFunctions::sanitizeFolderName(primaryTicker);
  ReportingFunctions::sanitizeFolderName(currencyCode);
  ReportingFunctions::sanitizeFolderName(countryName);

  replacementUpd.push_back(tickerMetaData.url);      

  replacementUpd.push_back(companyNameLatex);
  replacementUpd.push_back(primaryTickerLatex);
  replacementUpd.push_back(currencyCodeLatex);
  replacementUpd.push_back(countryNameLatex);

  replacementUpd.push_back(companyName);
  replacementUpd.push_back(primaryTicker);
  replacementUpd.push_back(currencyCode);
  replacementUpd.push_back(countryName);


};

//==============================================================================
void extractDataSeries(
      const nlohmann::ordered_json &jsonData,
      const std::vector<std::string> &addressToTimeSeries,
      const char* dateFieldName,
      const char* floatFieldName,
      std::vector<double> &dateSeries,
      std::vector<double> &floatSeries){

  dateSeries.clear();
  floatSeries.clear();
  std::vector< double > tmpFloatSeries;

  nlohmann::ordered_json jsonElement;

  bool isElementValid = false;

  if(addressToTimeSeries.size()>0){
    isElementValid = JsonFunctions::getJsonElement(
                        jsonData, addressToTimeSeries,jsonElement);
  }else{
    jsonElement = jsonData;
    if(jsonElement.size()>0){
      isElementValid=true;
    }
  }

  if(isElementValid){
    for(auto &el: jsonElement.items()){

      double floatData = 
        JsonFunctions::getJsonFloat(el.value()[floatFieldName]);

      if(JsonFunctions::isJsonFloatValid(floatData)){
    
        tmpFloatSeries.push_back(floatData);

        std::string dateEntryStr;
        JsonFunctions::getJsonString(el.value()[dateFieldName],
                        dateEntryStr); 

        double timeData = UtilityFunctions::convertToFractionalYear(dateEntryStr);
        dateSeries.push_back(timeData);
      }
    
    }

    std::vector< size_t > indicesSorted = sort_indices(dateSeries);
    for(size_t i=0; i<indicesSorted.size();++i){
      floatSeries.push_back( tmpFloatSeries[ indicesSorted[i] ] );
    }
  }

}

//==============================================================================
void formatJsonFieldAsLabel(std::string &nameUpd){

  size_t pos = nameUpd.find("_value",0);
  nameUpd = nameUpd.substr(0,pos);

  pos = nameUpd.find_last_of("_");
  if(pos != std::string::npos){
    nameUpd = nameUpd.substr(pos+1,nameUpd.size()-1);
  }
  ReportingFunctions::convertCamelCaseToSpacedText(nameUpd);
}

//==============================================================================
void plotMetric(
        const JsonMetaData &jsonData,
        const PlottingFunctions::PlotSettings &plotSettings,        
        const LineSettings &lineSettings,
        const AxisSettings &axisSettings,
        const BoxAndWhiskerSettings &boxAndWhiskerSettings,
        sciplot::Plot2D &plotMetricUpd,
        bool removeInvalidData,
        bool verbose)
{

    std::vector<double> xTmp;
    std::vector<double> yTmp;

    extractDataSeries(
        jsonData.jsonData,
        jsonData.address,
        jsonData.dateName.c_str(),
        jsonData.fieldName.c_str(),
        xTmp,
        yTmp);     
    
    bool timeSeriesValid = (xTmp.size()>0 && yTmp.size() > 0);

    if(timeSeriesValid || !removeInvalidData){
      
      if(!timeSeriesValid){
        xTmp.push_back(JsonFunctions::MISSING_VALUE);
        yTmp.push_back(JsonFunctions::MISSING_VALUE);
      }

      sciplot::Vec x(xTmp.size());
      sciplot::Vec y(yTmp.size());

      for(size_t i=0; i<xTmp.size();++i){
        x[i]= xTmp[i];
        y[i]=yTmp[i];
      }
      PlottingFunctions::SummaryStatistics metricSummaryStatistics;
      metricSummaryStatistics.name=jsonData.fieldName;
      bool validSummaryStats = 
        PlottingFunctions::extractSummaryStatistics(y,
                            metricSummaryStatistics);
      metricSummaryStatistics.current = yTmp[yTmp.size()-1];

      plotMetricUpd.drawCurve(x,y)
        .label(lineSettings.name)
        .lineColor(lineSettings.colour)
        .lineWidth(lineSettings.lineWidth);

      std::vector< double > xRange,yRange;
      PlottingFunctions::getDataRange(xTmp,xRange,1.0);
      PlottingFunctions::getDataRange(yTmp,yRange,
                          std::numeric_limits< double >::lowest());
      if(yRange[0] > 0){
        yRange[0] = 0;
      }
      if(yRange[1]-yRange[0] <= 0){
        yRange[1] = 1.0;
      }
      //Add some blank space to the top of the plot
      yRange[1] = yRange[1] + 0.2*(yRange[1]-yRange[0]);
      int currentLineType=1;

      if(!std::isnan(axisSettings.xMin)){
        xRange[0]=axisSettings.xMin;
      }

      if(!std::isnan(axisSettings.xMax)){
        xRange[1]=axisSettings.xMax;
      }

      if(!std::isnan(axisSettings.yMin)){
        yRange[0]=axisSettings.yMin;
      }

      if(!std::isnan(axisSettings.yMax)){
        yRange[1]=axisSettings.yMax;
      }

      plotMetricUpd.legend().atTopLeft();   

      if(validSummaryStats){
        double xPos = xRange[1]+1;
        if(!std::isnan(boxAndWhiskerSettings.xOffsetFromStart)){
          xPos = xRange[0]+boxAndWhiskerSettings.xOffsetFromStart;
        }
        if(!std::isnan(boxAndWhiskerSettings.xOffsetFromEnd)){
          xPos = xRange[1]+boxAndWhiskerSettings.xOffsetFromEnd;          
        }

        PlottingFunctions::drawBoxAndWhisker(
            plotMetricUpd,
            xPos,
            0.5,
            metricSummaryStatistics,
            boxAndWhiskerSettings.boxWhiskerColour.c_str(),
            boxAndWhiskerSettings.currentValueColour.c_str(),
            currentLineType,
            plotSettings,
            verbose);
      }

      if(!std::isnan(boxAndWhiskerSettings.xOffsetFromStart)){
        xRange[0] += (boxAndWhiskerSettings.xOffsetFromStart-1);
      }
      if(!std::isnan(boxAndWhiskerSettings.xOffsetFromEnd)){
        xRange[1] += (boxAndWhiskerSettings.xOffsetFromEnd+1);          
      }

      plotMetricUpd.xrange(
          static_cast<sciplot::StringOrDouble>(xRange[0]),
          static_cast<sciplot::StringOrDouble>(xRange[1]));              

      plotMetricUpd.yrange(
          static_cast<sciplot::StringOrDouble>(yRange[0]),
          static_cast<sciplot::StringOrDouble>(yRange[1]));

      if((xRange[1]-xRange[0])<5.0){
        plotMetricUpd.xtics().increment(plotSettings.xticMinimumIncrement);
      }

      if((xRange[1]-xRange[0]) > 10){
        double xSpan = (xRange[1]-xRange[0]);
        double increment = 1;

        while( xSpan/increment > 10 ){
          if(std::abs(increment-1) < std::numeric_limits<double>::epsilon()){
            increment = 5.0;
          }else{
            increment += 5.0;
          }
        }

        plotMetricUpd.xtics().increment(increment);

      }


      PlottingFunctions::configurePlot( plotMetricUpd,
                                        axisSettings.xAxisName,
                                        axisSettings.yAxisName,
                                        plotSettings);

    }


};

//==============================================================================
void writePlot(
      const std::vector< std::vector < sciplot::Plot2D >> &matrixOfPlots,
      const PlottingFunctions::PlotSettings &plotSettings,  
      const std::string &titleStr,    
      const std::string &outputPlotPath)
{

  size_t nrows = matrixOfPlots.size();
  size_t ncols = matrixOfPlots[0].size();

  std::vector< std::vector < sciplot::PlotVariant > > arrayOfPlotVariants;    

  for(size_t indexRow=0; indexRow < nrows; ++indexRow){
    std::vector< sciplot::PlotVariant > rowOfPlotVariants;    
    for(size_t indexCol=0; indexCol < ncols; ++indexCol){
      rowOfPlotVariants.push_back(matrixOfPlots[indexRow][indexCol]);
    }
    arrayOfPlotVariants.push_back(rowOfPlotVariants);
  }
  
  sciplot::Figure figTicker(arrayOfPlotVariants);

  figTicker.title(titleStr);

  sciplot::Canvas canvas = {{figTicker}};

  size_t canvasWidth  = 
    static_cast<size_t>(
      plotSettings.plotWidthInPoints*static_cast<double>(ncols));
  size_t canvasHeight = 
    static_cast<size_t>(
      plotSettings.plotHeightInPoints*static_cast<double>(nrows));

  canvas.size(canvasWidth, canvasHeight) ;

  // Save the figure to a PDF file
  canvas.save(outputPlotPath);

}

//==============================================================================
void updatePlotArray(
        std::vector< std::vector < sciplot::Plot2D >> &matrixOfPlots,
        PlottingFunctions::PlotSettings &plotSettingsUpd,        
        const TickerMetaData &tickerMetaData,
        const nlohmann::ordered_json &fundamentalData,
        const nlohmann::ordered_json &historicalData,
        const nlohmann::ordered_json &calculateData,
        const nlohmann::ordered_json &plotConfiguration,     
        const std::vector< std::string > &keywords,
        const std::vector< std::string > &replacements,          
        bool verbose)
{
  double tmp = 
    JsonFunctions::getJsonFloat(plotConfiguration["settings"]["rows"],false);
  size_t nrows = static_cast<size_t>(tmp);

  tmp = 
    JsonFunctions::getJsonFloat(plotConfiguration["settings"]["columns"],false);
  size_t ncols = static_cast<size_t>(tmp);

  if(matrixOfPlots.size() != nrows){
    matrixOfPlots.resize(nrows);
  }
  for(size_t i=0; i < nrows; ++i){
    if(matrixOfPlots[i].size() != ncols){
      matrixOfPlots[i].resize(ncols);
    }
  }

  double plotWidthCm = 
    JsonFunctions::getJsonFloat(
      plotConfiguration["settings"]["plotWidthCm"],false);

  double plotHeightCm = 
    JsonFunctions::getJsonFloat(
      plotConfiguration["settings"]["plotHeightCm"],false);

  plotSettingsUpd.plotWidthInPoints = 
    PlottingFunctions::convertCentimetersToPoints(plotWidthCm);     

  plotSettingsUpd.plotHeightInPoints = 
    PlottingFunctions::convertCentimetersToPoints(plotHeightCm);     

  


  for(auto &plotConfig : plotConfiguration["plots"].items()){

    //Get the JsonMetaData
    std::string jsonData;
    std::vector< JsonMetaData > jsonMetaDataVector;
    JsonFunctions::getJsonString(plotConfig.value()["jsonData"], jsonData);  


    if(jsonData.compare("fundamentalData")==0){
      JsonMetaData jsonMetaData(fundamentalData);
      jsonMetaDataVector.push_back(jsonMetaData);


    }else if(jsonData.compare("historicalData")==0){
      JsonMetaData jsonMetaData(historicalData);
      jsonMetaDataVector.push_back(jsonMetaData);


    }else if(jsonData.compare("calculateData")==0){
      JsonMetaData jsonMetaData(calculateData);
      jsonMetaDataVector.push_back(jsonMetaData);

    }else{
      std::cout << "Error: jsonData type not recognized. The field jsonData "
                << "needs to be in the set [fundamentalData, historicalData, "
                << "calculateData] but instead this was passed in: "
                << jsonData
                << std::endl;
      std::abort();                
    }

    size_t indexEnd = jsonMetaDataVector.size()-1;
    jsonMetaDataVector[indexEnd].address.clear();
    for(auto &addressItem : plotConfig.value()["address"].items()){
      std::string fieldName;
      JsonFunctions::getJsonString(addressItem.value(),fieldName);
      jsonMetaDataVector[indexEnd].address.push_back(fieldName);
    }
    std::string fieldName;
    JsonFunctions::getJsonString(plotConfig.value()["fieldName"],fieldName);
    jsonMetaDataVector[indexEnd].fieldName = fieldName;

    std::string dateName;
    JsonFunctions::getJsonString(plotConfig.value()["dateName"],dateName);
    jsonMetaDataVector[indexEnd].dateName = dateName;
    
    jsonMetaDataVector[indexEnd].isArray = 
      JsonFunctions::getJsonBool(plotConfig.value()["isArray"],false);


    //Get the LineSettings
    LineSettings lineSettings;
    JsonFunctions::getJsonString(plotConfig.value()["lineColor"],
                                    lineSettings.colour);

    JsonFunctions::getJsonString(plotConfig.value()["legendEntry"],
                                    lineSettings.name); 
    findReplaceKeywords(lineSettings.name,keywords,replacements);

    lineSettings.lineWidth = 
      JsonFunctions::getJsonFloat(plotConfig.value()["lineWidth"],false);



    //Get the AxixSettings
    AxisSettings axisSettings;
    std::string xAxisLabel, yAxisLabel;
    JsonFunctions::getJsonString(plotConfig.value()["xAxisLabel"],
                                 axisSettings.xAxisName);        
    findReplaceKeywords(axisSettings.xAxisName,keywords,replacements);
                                 
    JsonFunctions::getJsonString(plotConfig.value()["yAxisLabel"],
                                 axisSettings.yAxisName);        
    findReplaceKeywords(axisSettings.yAxisName,keywords,replacements);

    axisSettings.xMin = JsonFunctions::getJsonFloat(plotConfig.value()["xMin"],
                        false);
    axisSettings.xMax = JsonFunctions::getJsonFloat(plotConfig.value()["xMax"],
                        false);
    axisSettings.yMin = JsonFunctions::getJsonFloat(plotConfig.value()["yMin"],
                        false);
    axisSettings.yMax = JsonFunctions::getJsonFloat(plotConfig.value()["yMax"],
                        false);

    //Get SubplotSettings
    SubplotSettings subplotSettings;

    tmp = JsonFunctions::getJsonFloat(plotConfig.value()["row"],false);
    subplotSettings.indexRow = static_cast<size_t>(tmp);

    tmp = JsonFunctions::getJsonFloat(plotConfig.value()["column"],false);
    subplotSettings.indexColumn = static_cast<size_t>(tmp);

    //Get Box and Whisker Settings
    BoxAndWhiskerSettings boxWhiskerSettings;
    tmp = JsonFunctions::getJsonFloat(plotConfig.value()["boxWhiskerPosition"],false);
    if(tmp >= 0){
      boxWhiskerSettings.xOffsetFromStart=tmp;
    }else{
      boxWhiskerSettings.xOffsetFromEnd = std::abs(tmp);
    }

    JsonFunctions::getJsonString(plotConfig.value()["boxWhiskerColor"],
                                 boxWhiskerSettings.boxWhiskerColour);

    boxWhiskerSettings.currentValueColour = lineSettings.colour;

    plotMetric(
        jsonMetaDataVector[indexEnd],
        plotSettingsUpd,
        lineSettings,
        axisSettings,
        boxWhiskerSettings,
        matrixOfPlots[subplotSettings.indexRow][subplotSettings.indexColumn],
        true,
        verbose);
  
  }

};

//==============================================================================
void generateLaTeXReportWrapper(
    const char* wrapperFilePath,
    const std::string &reportFileName,
    const TickerMetaData &tickerMetaData,
    bool verbose)
{
  if(verbose){
    std::cout << "Gerating LaTeX report wrapper file for " 
              << tickerMetaData.primaryTicker  << std::endl << std::endl; 
  }


  std::ofstream latexReport;
  try{
    latexReport.open(wrapperFilePath);
  }catch(std::ofstream::failure &ofstreamErr){
    std::cerr << std::endl << std::endl
              << "Skipping writing " << tickerMetaData.primaryTicker              
              << " because an exception was thrown while trying to open ofstream:"
              << wrapperFilePath << std::endl << std::endl
              << ofstreamErr.what()
              << std::endl;    
  }

  //Append the opening latex commands
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
  latexReport << "\\begin{document}"<<std::endl;
  latexReport << "\\input{" << reportFileName << "}" << std::endl;
  latexReport << "\\end{document}" << std::endl;
  latexReport.close();

};

//==============================================================================
bool generateLaTeXReport(
    const TickerMetaData &tickerMetaData,
    const std::string &date,
    const nlohmann::ordered_json &fundamentalData,
    const nlohmann::ordered_json &historicalData,
    const nlohmann::ordered_json &calculateData,
    const char* plotFileName,
    const char* plotDebuggingFileName,
    const char* outputReportPath,
    bool replaceNansWithMissingData,
    bool verbose)
{

  bool skip = false;

  if(verbose){
    std::cout << "Gerating LaTeX report for " 
              << tickerMetaData.primaryTicker  << std::endl; 
  }

  std::vector< JsonFieldAddress > tabularMetrics;
  JsonFieldAddress metric;

  metric.fieldNames.push_back("General");
  metric.fieldNames.push_back("ISIN");
  metric.type = JSON_FIELD_TYPE::STRING;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("General");
  metric.fieldNames.push_back("CurrencyCode");
  metric.type = JSON_FIELD_TYPE::STRING;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("General");
  metric.fieldNames.push_back("IPODate");
  metric.type = JSON_FIELD_TYPE::STRING;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("Highlights");
  metric.fieldNames.push_back("MarketCapitalizationMln");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);
  
  metric.fieldNames.clear();
  metric.fieldNames.push_back("Technicals");
  metric.fieldNames.push_back("Beta");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("Valuation");
  metric.fieldNames.push_back("EnterpriseValueEbitda");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("SharesStats");
  metric.fieldNames.push_back("PercentInsiders");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("SharesStats");
  metric.fieldNames.push_back("PercentInstitutions");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("TargetPrice");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("StrongBuy");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("Buy");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("Hold");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("Sell");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("StrongSell");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  std::string latexReportPath(outputReportPath);
  std::ofstream latexReport;

  try{
    latexReport.open(latexReportPath);
  }catch(std::ofstream::failure &ofstreamErr){
    std::cerr << std::endl << std::endl 
              << "Skipping writing " << latexReportPath              
              << " because an exception was thrown while trying to open ofstream:"
              << latexReportPath << std::endl << std::endl
              << ofstreamErr.what()
              << std::endl;    
    skip=true;
  }


  if(!skip){

    std::string webURL;
    JsonFunctions::getJsonString(fundamentalData[GEN]["WebURL"],webURL);

    std::string description;
    JsonFunctions::getJsonString(
        fundamentalData[GEN]["Description"],description);
    ReportingFunctions::sanitizeStringForLaTeX(description);

    std::string companyNameString(tickerMetaData.companyName);
    std::string primaryTickerString(tickerMetaData.primaryTicker);

    ReportingFunctions::sanitizeStringForLaTeX(companyNameString);
    ReportingFunctions::sanitizeStringForLaTeX(primaryTickerString);



    latexReport << std::endl;

    latexReport << "\\begin{figure}[h]" << std::endl;
    latexReport << "  \\begin{center}" << std::endl;
    latexReport << "    \\includegraphics{" 
                <<      plotFileName << "}" << std::endl;
    latexReport << "    \\caption{"
                << companyNameString
                << " (" << primaryTickerString <<") "
                << tickerMetaData.country << " ( \\url{" << webURL << "} )"
                << "}" << std::endl;
    latexReport << " \\end{center}" << std::endl;
    latexReport << "\\end{figure}"<< std::endl;
    latexReport << std::endl;

    latexReport << "\\begin{multicols}{2}" << std::endl;

    latexReport << description << std::endl;
    latexReport << std::endl;

    //latexReport << "\\begin{center}" << std::endl;


    latexReport << "\\bigskip" << std::endl;
    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\multicolumn{2}{c}{\\textbf{Contextual Data}} \\\\" << std::endl;

    for(size_t  indexTabularMetrics=0; 
                indexTabularMetrics < tabularMetrics.size();
              ++indexTabularMetrics)
    {
      JsonFieldAddress entryMetric = tabularMetrics[indexTabularMetrics];
      std::string labelMetric = entryMetric.fieldNames.back();
      ReportingFunctions::convertCamelCaseToSpacedText(labelMetric);

      bool fieldExists = JsonFunctions::doesFieldExist(fundamentalData, 
                                                entryMetric.fieldNames);

      if(fieldExists){
        switch(entryMetric.type){
          case JSON_FIELD_TYPE::BOOL : {
            bool value = JsonFunctions::getJsonBool(fundamentalData,
                              entryMetric.fieldNames, 
                              replaceNansWithMissingData);

            latexReport << labelMetric << " & " << value  << "\\\\" << std::endl;                          
          }
          break;
          case JSON_FIELD_TYPE::FLOAT : {
            double value = JsonFunctions::getJsonFloat(fundamentalData,
                            entryMetric.fieldNames, 
                            replaceNansWithMissingData);

            latexReport << labelMetric << " & " << value << "\\\\" << std::endl;                          
          }
          break;
          case JSON_FIELD_TYPE::STRING : {
            std::string value;
            JsonFunctions::getJsonString(fundamentalData,
                              entryMetric.fieldNames, 
                              value);

            latexReport << labelMetric << " & " << value << "\\\\" << std::endl;                          
          }
          break;
        };
      }else{
        latexReport << labelMetric << " & " << " - "  << "\\\\" << std::endl;
      }
    }
    latexReport << "\\end{tabular}" << std::endl << std::endl;

    //std::string date = calculateData.begin().key();

    ReportingFunctions::appendValuationTable(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData,
      date,
      verbose);
   

    latexReport << "\\end{multicols}" << std::endl;

    latexReport << "\\begin{figure}[h]" << std::endl;
    latexReport << "  \\begin{center}" << std::endl;
    latexReport << "    \\includegraphics{" 
                <<      plotDebuggingFileName << "}" << std::endl;
    latexReport << "    \\caption{"
                << companyNameString
                << " (" << primaryTickerString <<") "
                << tickerMetaData.country << " ( \\url{" << webURL << "} )"
                << " debugging data }" << std::endl;
    latexReport << " \\end{center}" << std::endl;
    latexReport << "\\end{figure}"<< std::endl;
    latexReport << std::endl;

    latexReport << "\\break"          << std::endl;
    latexReport << "\\newpage"        << std::endl;
  }

  latexReport.close();

  if(verbose){
    if(!skip){
      std::cout  << tickerMetaData.primaryTicker << std::endl;
    }else{
      std::cout << " Skipping " << tickerMetaData.primaryTicker << std::endl;
    }       
  }

  return !skip;

};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string calculateDataFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string plotSummaryConfigurationFilePath;  
  std::string plotOverviewConfigurationFilePath;  
  std::string reportFolder;
  std::string dateOfTable;

  std::string singleFileToEvaluate;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will produce reports in the form of text "
    "and tables about the companies in the chosen exchange with valid data."
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

    TCLAP::ValueArg<std::string> singleFileToEvaluateInput("i",
      "single_ticker_name", 
      "To evaluate a single ticker only, set the ticker name here.",
      false,"","string");
    cmd.add(singleFileToEvaluateInput);    

    TCLAP::ValueArg<std::string> dateOfTableInput("d","date", 
      "Date used to produce the dicounted cash flow model detailed output.",
      false,"","string");
    cmd.add(dateOfTableInput);

    TCLAP::ValueArg<std::string> plotSummaryConfigurationFilePathInput("s",
      "summary_plot_configuration", 
      "The path to the json file that defines the summary plot.",
      true,"","string");
    cmd.add(plotSummaryConfigurationFilePathInput);    

    TCLAP::ValueArg<std::string> plotOverviewConfigurationFilePathInput("r",
      "overview_plot_configuration", 
      "The path to the json file that defines the overview plot.",
      true,"","string");
    cmd.add(plotOverviewConfigurationFilePathInput);        

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    singleFileToEvaluate     = singleFileToEvaluateInput.getValue();
    exchangeCode             = exchangeCodeInput.getValue();  
    plotSummaryConfigurationFilePath 
                            = plotSummaryConfigurationFilePathInput.getValue();      
    plotOverviewConfigurationFilePath 
                            = plotOverviewConfigurationFilePathInput.getValue();      
    calculateDataFolder      = calculateDataFolderInput.getValue();
    fundamentalFolder        = fundamentalFolderInput.getValue();
    historicalFolder         = historicalFolderInput.getValue();    
    reportFolder             = reportFolderOutput.getValue();
    dateOfTable              = dateOfTableInput.getValue();
    verbose                  = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Single file name to evaluate" << std::endl;
      std::cout << "    " << singleFileToEvaluate << std::endl;

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

      std::cout << "  Report Folder" << std::endl;
      std::cout << "    " << reportFolder << std::endl;  

      std::cout << "  Summary Plot Configuration File" << std::endl;
      std::cout << "    " << plotSummaryConfigurationFilePath << std::endl;   

      std::cout << "  Overview Plot Configuration File" << std::endl;
      std::cout << "    " << plotOverviewConfigurationFilePath << std::endl;                
    }

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  bool replaceNansWithMissingData = true;

  //PlottingFunctions::PlotSettings plotSettings;
  //std::vector< std::vector < std::string > > subplotMetricNames;
  //readConfigurationFile(plotSummaryConfigurationFilePath,subplotMetricNames);

  //Load the summary plot configuration
  nlohmann::ordered_json summaryPlotConfig;
  bool loadedSummaryPlotData = 
    JsonFunctions::loadJsonFile(plotSummaryConfigurationFilePath,
                                summaryPlotConfig, 
                                verbose);

  if(!loadedSummaryPlotData){
    std::cout << "Error: could not load "  << std::endl; 
    std::cout << plotSummaryConfigurationFilePath << std::endl;
    std::abort();
  }
  
  //Load the overview plot configuration
  nlohmann::ordered_json overviewPlotConfig;
  bool loadedOverviewPlotData = 
    JsonFunctions::loadJsonFile(plotOverviewConfigurationFilePath,
                                overviewPlotConfig, 
                                verbose);

  if(!loadedOverviewPlotData){
    std::cout << "Error: could not load "  << std::endl; 
    std::cout << plotOverviewConfigurationFilePath << std::endl;
    std::abort();
  }



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



  for (const auto & file 
        : std::filesystem::directory_iterator(calculateDataFolder)){  

    ++totalFileCount;
    bool validInput = true;

    //
    // Check the file name
    //    
    std::string fileName   = file.path().filename();

    if(singleFileToEvaluate.length() > 0){
      fileName = singleFileToEvaluate;
    }    

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





    if(validInput){


      //
      // Create the output folder
      //      

      std::string ticker = fileName.substr(0,fileExtPos);
      std::string tickerFolderName = ticker;
      std::replace(tickerFolderName.begin(),tickerFolderName.end(),'.','_');

      std::string outputFolderName = reportFolder;
      outputFolderName.append(tickerFolderName);

      std::filesystem::path outputFolderPath = outputFolderName;
      if( !std::filesystem::is_directory(outputFolderPath) ){
        std::filesystem::create_directory(outputFolderPath);
      }


      //
      //Read in the inputs
      //
      nlohmann::ordered_json fundamentalData;  
      bool loadedFundData =JsonFunctions::loadJsonFile(ticker, 
                                fundamentalFolder, fundamentalData, verbose);
      if(loadedFundData){
        loadedFundData = !fundamentalData.empty();
      }
    
      nlohmann::ordered_json historicalData;
      bool loadedHistData =JsonFunctions::loadJsonFile(ticker, 
                                historicalFolder, historicalData, verbose); 
      if(loadedHistData){
        loadedHistData = !historicalData.empty();
      }
                                
      nlohmann::ordered_json calculateData;
      bool loadedCalculateData =JsonFunctions::loadJsonFile(ticker, 
                                calculateDataFolder, calculateData, verbose);
      if(loadedCalculateData){
        loadedCalculateData = !calculateData.empty();
      }


      if(loadedFundData && loadedHistData && loadedCalculateData){


        TickerMetaData tickerMetaData;
        getTickerMetaData(fundamentalData, tickerMetaData);
        bool isTickerProcessed = isNameInList(tickerMetaData.primaryTicker, 
                                            processedTickers); 

        std::vector< std::string > keywords;
        std::vector< std::string > replacements;
        getFindAndReplacementVectors( tickerMetaData,
                                      keywords,
                                      replacements);

        //
        // Generate and save the pdf
        //
        if(!isTickerProcessed && tickerMetaData.companyName.length() > 0){

          std::filesystem::path outputPlotFilePath = outputFolderPath;                  
          PlottingFunctions::PlotSettings plotSettingsSummary;
          std::vector< std::vector< sciplot::Plot2D >> summaryPlots;

          updatePlotArray(
            summaryPlots,
            plotSettingsSummary,
            tickerMetaData,
            fundamentalData,
            historicalData,
            calculateData,
            summaryPlotConfig,
            keywords,
            replacements,
            verbose);

          std::string titleSummary;
          JsonFunctions::getJsonString( summaryPlotConfig["settings"]["title"],
                                        titleSummary);
          findReplaceKeywords(titleSummary, keywords,replacements);                                        

          std::string plotSummaryFileName;
          JsonFunctions::getJsonString( summaryPlotConfig["settings"]["fileName"],
                                        plotSummaryFileName);                                        
          findReplaceKeywords(plotSummaryFileName, keywords,replacements); 

          std::filesystem::path outputSummaryPlotFilePath = outputFolderPath;          
          outputSummaryPlotFilePath.append(plotSummaryFileName);

          writePlot(summaryPlots,
                    plotSettingsSummary,
                    titleSummary,
                    outputSummaryPlotFilePath.c_str());


          PlottingFunctions::PlotSettings plotSettingsOverview;
          std::vector< std::vector< sciplot::Plot2D >> overviewPlots;

          updatePlotArray(
            overviewPlots,
            plotSettingsOverview,
            tickerMetaData,
            fundamentalData,
            historicalData,
            calculateData,
            overviewPlotConfig,
            keywords,
            replacements,
            verbose);

          std::string titleOverview;
          JsonFunctions::getJsonString( overviewPlotConfig["settings"]["title"],
                                        titleOverview);
          findReplaceKeywords(titleOverview, keywords, replacements); 

          std::string plotOverviewFileName;
          JsonFunctions::getJsonString( overviewPlotConfig["settings"]["fileName"],
                                        plotOverviewFileName);                                        
          findReplaceKeywords(plotOverviewFileName, keywords, replacements); 

          std::filesystem::path outputOverviewPlotFilePath = outputFolderPath;          
          outputOverviewPlotFilePath.append(plotOverviewFileName);

          writePlot(overviewPlots,
                    plotSettingsOverview,
                    titleOverview,
                    outputOverviewPlotFilePath.c_str());


          std::filesystem::path outputReportFilePath = outputFolderPath;
          std::string reportFileName(tickerFolderName);
          reportFileName.append(".tex");
          outputReportFilePath.append(reportFileName);

          std::string date = calculateData.begin().key();
          if(dateOfTable.length()>0){
            date=dateOfTable;
          }

          bool successGenerateLaTeXReport = 
            generateLaTeXReport(
              tickerMetaData,
              date,
              fundamentalData,
              historicalData,
              calculateData,
              plotSummaryFileName.c_str(),
              plotOverviewFileName.c_str(),
              outputReportFilePath.c_str(),    
              replaceNansWithMissingData,
              false); 

          std::string wrapperFileName("report_");
          wrapperFileName.append(reportFileName);

          outputReportFilePath = outputFolderPath;
          outputReportFilePath.append(wrapperFileName);


          generateLaTeXReportWrapper(outputReportFilePath.c_str(), 
                                      reportFileName, 
                                      tickerMetaData,
                                      false);

          processedTickers.push_back(tickerMetaData.primaryTicker);

          if(verbose){
            if(  !successGenerateLaTeXReport){
              std::cout << '\t' << "skipping" << std::endl;
              //std::cout << '\t' << successPlotTickerData 
              //          << '\t' << "plotting" << std::endl;
              std::cout << '\t' << successGenerateLaTeXReport 
                        << '\t' << "report generation" << std::endl;
              //std::cout << '\t' << successGenerateLatexReportWrapper 
              //          << '\t' << "report wrapper generation" << std::endl;

            }
          }

          ++validFileCount;     
        }
      }


    }

    if(singleFileToEvaluate.length() > 0){
      break;
    }
  }
 
  if(verbose){
    std::cout << std::endl;
    std::cout << "Summary" << std::endl;
    std::cout << '\t' << totalFileCount 
              << '\t' << "number of tickers processed" << std::endl;
    std::cout << '\t' << validFileCount
              << '\t' << "number of tickers with valid data" << std::endl;
  } 





  return 0;
}
