
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


const char* BoxAndWhiskerColorA="web-blue";
const char* BoxAndWhiskerColorB="gray0";

//==============================================================================
struct LineSettings{
  std::string colour;
  std::string name;
  double lineWidth;
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
  std::string ticker;
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
  TickerMetaData():
    primaryTicker(""),
    companyName(""),
    currencyCode(""),
    country(""),
    isin(""){}
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
  ReportingFunctions::sanitizeStringForLaTeX(tickerMetaDataUpd.companyName);                                

  JsonFunctions::getJsonString( fundamentalData[GEN]["ISIN"],
                                tickerMetaDataUpd.isin);                                

  JsonFunctions::getJsonString( fundamentalData[GEN]["CurrencyCode"],
                                tickerMetaDataUpd.currencyCode);
  ReportingFunctions::sanitizeStringForLaTeX(tickerMetaDataUpd.currencyCode);

  JsonFunctions::getJsonString(fundamentalData[GEN]["CountryName"],
                               tickerMetaDataUpd.country);  
  ReportingFunctions::sanitizeStringForLaTeX(tickerMetaDataUpd.country);      

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
void readConfigurationFile(std::string &plotConfigurationFilePath,
                std::vector< std::vector < std::string > > &subplotMetricNames)
{

  std::ifstream configFile(plotConfigurationFilePath);
  if(configFile.is_open()){
    bool endOfFile=false;

      std::vector< std::string > metricRowVector; 
      std::string line;
      do{
        line.clear();
        metricRowVector.clear();

        std::getline(configFile,line);
        
        if(line.length() > 0){
          //Extract the individual fields
          size_t i0=0;
          size_t i1=line.find_first_of(',',i0);
          while(i1 > i0){
            std::string metricName = line.substr(i0,i1-i0);
            if(metricName.length()>0){
              metricRowVector.push_back(metricName);
              metricName.clear();
            }
            i0=i1+1;
            i1 = line.find_first_of(',',i0);
            if(i1 == std::string::npos){
              i1 = line.length();
            }            
          }
          if(metricRowVector.size() > 0){
            subplotMetricNames.push_back(metricRowVector);
          }
        }else{
          endOfFile=true;
        }
      }while(!endOfFile);
  }
};
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
bool plotMetric(
        const JsonMetaData &jsonData,
        const PlottingFunctions::PlotSettings &plotSettings,        
        const LineSettings &lineSettings,
        const AxisSettings &axisSettings,
        sciplot::Plot2D &plotMetricUpd,
        bool removeInvalidData,
        bool verbose)
{

    bool success=false;

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
        PlottingFunctions::drawBoxAndWhisker(
            plotMetricUpd,
            xRange[1]+1,
            0.5,
            metricSummaryStatistics,
            BoxAndWhiskerColorA,
            BoxAndWhiskerColorB,
            currentLineType,
            plotSettings,
            verbose);
      }

      xRange[1] += 2.0;

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
      success=true;  
    }

  return success;

};

//==============================================================================
bool plotDebuggingData(
    const TickerMetaData &tickerMetaData,
    const nlohmann::ordered_json &fundamentalData,
    const nlohmann::ordered_json &historicalData,
    const nlohmann::ordered_json &calculateData,
    const char* outputPlotPath,
    const PlottingFunctions::PlotSettings &plotSettings,
    bool verbose)
{

  bool success = true;               

  size_t nrows = 2;
  size_t ncols = 2;

  PlottingFunctions::PlotSettings plotSettingsUpd(plotSettings);
  plotSettingsUpd.plotHeightInPoints= 8.0*InchesPerCentimeter*PointsPerInch;
  plotSettingsUpd.plotWidthInPoints= 8.0*InchesPerCentimeter*PointsPerInch;

  //std::vector<std::string> metrics;
  //metrics.push_back(std::string("presentValueDCF_afterTaxOperatingIncome"));
  //metrics.push_back(std::string("presentValueDCF_reinvestmentRate"));
  //metrics.push_back(std::string("presentValueDCF_netIncomeGrowth"));
  //metrics.push_back(std::string("presentValueDCF_costOfCapital"));
  //metrics.push_back(std::string("priceToValue_marketCapitalization"));
  //metrics.push_back(std::string("returnOnInvestedCapital_totalStockholderEquity"));
  //metrics.push_back(std::string("presentValueDCF_returnOnInvestedCapital_longTermDebt"));
  //metrics.push_back(std::string("returnOnInvestedCapital_netIncome"));

  std::vector< JsonMetaData > jsonMetricData;

  std::vector< SubplotSettings > subplotSettings;
  subplotSettings.resize(4);

  
  size_t i=0;

  subplotSettings[0].indexRow     = 0;
  subplotSettings[0].indexColumn  = 0;
  subplotSettings[1]              = subplotSettings[0];
  subplotSettings[1].indexColumn  = 1;

  subplotSettings[2]              = subplotSettings[0];
  subplotSettings[2].indexRow     = 1;

  subplotSettings[3]              = subplotSettings[1];
  subplotSettings[3].indexRow     = 1;

  JsonMetaData eleSharesA(fundamentalData);
  eleSharesA.address.push_back("outstandingShares");
  eleSharesA.address.push_back("annual");
  eleSharesA.fieldName="shares";
  eleSharesA.dateName="dateFormatted";
  eleSharesA.isArray=true;

  JsonMetaData eleSharesQ(fundamentalData);
  eleSharesQ.address.push_back("outstandingShares");
  eleSharesQ.address.push_back("quarterly");
  eleSharesQ.fieldName="shares";
  eleSharesQ.dateName="dateFormatted";
  eleSharesQ.isArray=true;

  JsonMetaData longTermDebtA(fundamentalData);
  longTermDebtA.address.push_back("Financials");
  longTermDebtA.address.push_back("Balance_Sheet");
  longTermDebtA.address.push_back("yearly");
  longTermDebtA.fieldName="longTermDebt";
  longTermDebtA.dateName="date";
  longTermDebtA.isArray=true;

  JsonMetaData longTermDebtQ(fundamentalData);
  longTermDebtQ.address.push_back("Financials");
  longTermDebtQ.address.push_back("Balance_Sheet");
  longTermDebtQ.address.push_back("quarterly");
  longTermDebtQ.fieldName="longTermDebt";
  longTermDebtQ.dateName="date";
  longTermDebtQ.isArray=true;  

  jsonMetricData.push_back(eleSharesA);
  jsonMetricData.push_back(eleSharesQ);  
  jsonMetricData.push_back(longTermDebtA);
  jsonMetricData.push_back(longTermDebtQ);

  std::vector< LineSettings > lineSettings;
  LineSettings lineA;
  lineA.colour = "black";
  lineA.lineWidth=1.0;
  lineA.name ="annual";

  LineSettings lineB;
  lineB.colour = "blue";
  lineB.lineWidth=1.0;
  lineB.name = "quarterly";
  
  lineSettings.push_back(lineA);
  lineSettings.push_back(lineB);
  lineSettings.push_back(lineA);
  lineSettings.push_back(lineB);

  std::vector< AxisSettings > axisSettings;
  
  AxisSettings axisAB,axisCD;
  axisAB.xAxisName="Years";
  axisAB.yAxisName="Shares Outstanding";
  if(tickerMetaData.primaryTicker.compare("LVMUY.US")==0){
    axisAB.yMin = 4.0e8;
    axisAB.yMax = 2.6e9;
  }

  axisCD.xAxisName="Years";
  axisCD.yAxisName="Long Term Debt";
  if(tickerMetaData.primaryTicker.compare("LVMUY.US")==0){
    axisCD.yMin = 0.0;
    axisCD.yMax = 2.5e10;
  }

  axisSettings.push_back(axisAB);
  axisSettings.push_back(axisAB);

  axisSettings.push_back(axisCD);
  axisSettings.push_back(axisCD);
  


  //Populate the array with empty plots  
  std::vector< std::vector< sciplot::Plot2D > > matrixOfPlots;
  matrixOfPlots.resize(nrows);  
  for(size_t indexRow=0; indexRow < nrows; ++indexRow){
    matrixOfPlots[indexRow].resize(ncols);
  }

  for(size_t indexMetric=0; indexMetric < subplotSettings.size(); ++indexMetric){

    size_t row = subplotSettings[indexMetric].indexRow;
    size_t col = subplotSettings[indexMetric].indexColumn;
    bool removeInvalidData=false;

    bool plotAdded = plotMetric(
                        jsonMetricData[indexMetric],
                        plotSettings,
                        lineSettings[indexMetric],
                        axisSettings[indexMetric],
                        matrixOfPlots[row][col],
                        removeInvalidData,
                        verbose);        
  }
  std::vector< std::vector < sciplot::PlotVariant > > arrayOfPlotVariants;    


  for(size_t indexRow=0; indexRow < nrows; ++indexRow){
    std::vector< sciplot::PlotVariant > rowOfPlotVariants;    
    for(size_t indexCol=0; indexCol < ncols; ++indexCol){
      rowOfPlotVariants.push_back(matrixOfPlots[indexRow][indexCol]);
    }
    arrayOfPlotVariants.push_back(rowOfPlotVariants);
  }



  std::string titleStr = tickerMetaData.companyName;
  titleStr.append(" (");
  titleStr.append(tickerMetaData.primaryTicker);
  titleStr.append(") - ");
  titleStr.append(tickerMetaData.country);
  titleStr.append(" debugging overview");
  
  sciplot::Figure figTicker(arrayOfPlotVariants);

  figTicker.title(titleStr);

  sciplot::Canvas canvas = {{figTicker}};

  size_t canvasWidth  = 
    static_cast<size_t>(
      plotSettingsUpd.plotWidthInPoints*static_cast<double>(ncols));
  size_t canvasHeight = 
    static_cast<size_t>(
      plotSettingsUpd.plotHeightInPoints*static_cast<double>(nrows));

  canvas.size(canvasWidth, canvasHeight) ;

  // Save the figure to a PDF file
  canvas.save(outputPlotPath);

  return success;  

}
//==============================================================================
bool plotTickerData(
    const TickerMetaData &tickerMetaData,
    const nlohmann::ordered_json &fundamentalData,
    const nlohmann::ordered_json &historicalData,
    const nlohmann::ordered_json &calculateData,
    const char* outputPlotPath,
    const std::vector< std::vector< std::string >> &subplotMetricNames,
    const PlottingFunctions::PlotSettings &plotSettings,
    bool verbose)
{
  bool success = true;               

  std::vector< std::vector < sciplot::PlotVariant > > arrayOfPlots;

  for(size_t indexRow=0; indexRow < subplotMetricNames.size(); 
      ++indexRow){

    std::vector < sciplot::PlotVariant > rowOfPlots;

    for(size_t indexCol=0; indexCol < subplotMetricNames[indexRow].size(); 
        ++indexCol){
      bool subplotAdded = false; 

      LineSettings lineA;
      lineA.colour = "black";
      lineA.lineWidth=1.0;
      lineA.name = "";

      AxisSettings axisA;
      axisA.xAxisName="Years";
      axisA.yAxisName=subplotMetricNames[indexRow][indexCol];
      formatJsonFieldAsLabel(axisA.yAxisName);      
      

      if(subplotMetricNames[indexRow][indexCol].compare("historicalData")==0){
        sciplot::Plot2D plotHistoricalData;
        JsonMetaData jsonHistData(historicalData);
        jsonHistData.address.clear();
        jsonHistData.dateName="date";
        jsonHistData.fieldName="adjusted_close";
        jsonHistData.isArray=true;
        jsonHistData.ticker=tickerMetaData.primaryTicker;

        LineSettings lineSettings;
        lineSettings.colour ="black";
        lineSettings.lineWidth=1.0;
        lineSettings.name = tickerMetaData.primaryTicker;

        AxisSettings axisSettings;
        axisSettings.xAxisName="Years";
        axisSettings.yAxisName=tickerMetaData.currencyCode;

        bool removeInvalidData=true;

        bool isHistDataPlotted = 
          plotMetric( jsonHistData,
                      plotSettings,
                      lineSettings,
                      axisSettings,
                      plotHistoricalData,
                      true,
                      verbose);

        rowOfPlots.push_back(plotHistoricalData);
        subplotAdded=true;
      }      

      if(subplotMetricNames[indexRow][indexCol].compare("empty")!=0 
        && !subplotAdded){
          sciplot::Plot2D subplot;  
          bool removeInvalidData=true;

          JsonMetaData jsonEle(calculateData);
          jsonEle.ticker=tickerMetaData.primaryTicker;
          jsonEle.address.clear();
          jsonEle.fieldName=subplotMetricNames[indexRow][indexCol];
          jsonEle.dateName="date";


          bool plotAdded = plotMetric(
                              jsonEle,
                              plotSettings,
                              lineA,
                              axisA,
                              subplot,
                              removeInvalidData,
                              verbose);

          if(plotAdded){
            rowOfPlots.push_back(subplot);
            subplotAdded=true;
          }   
        }     
      }      
    
    if(rowOfPlots.size()>0){
      arrayOfPlots.push_back(rowOfPlots);
    }
  }

  std::string titleStr = tickerMetaData.companyName;
  titleStr.append(" (");
  titleStr.append(tickerMetaData.primaryTicker);
  titleStr.append(") - ");
  titleStr.append(tickerMetaData.country);
  
  sciplot::Figure figTicker(arrayOfPlots);

  figTicker.title(titleStr);

  sciplot::Canvas canvas = {{figTicker}};
  unsigned int nrows = subplotMetricNames.size();
  unsigned int ncols=0;
  for(unsigned int i=0; i<subplotMetricNames.size();++i){
    if(ncols < subplotMetricNames[i].size()){
      ncols=subplotMetricNames[i].size();
    }
  }

  size_t canvasWidth  = 
    static_cast<size_t>(
      plotSettings.plotWidthInPoints*static_cast<double>(ncols));
  size_t canvasHeight = 
    static_cast<size_t>(
      plotSettings.plotHeightInPoints*static_cast<double>(nrows));

  canvas.size(canvasWidth, canvasHeight) ;

  // Save the figure to a PDF file
  canvas.save(outputPlotPath);

  return success;

};
//==============================================================================
bool generateLaTeXReportWrapper(
    const char* wrapperFilePath,
    const std::string &reportFileName,
    const TickerMetaData &tickerMetaData,
    bool verbose)
{
  bool success = true;
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
    success = false;
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

  return success;
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
    const std::vector< std::vector< std::string >> &subplotMetricNames,
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

    for(size_t i=0; i<subplotMetricNames.size(); ++i){
      for(size_t j=0; j<subplotMetricNames[i].size(); ++j){
        if(subplotMetricNames[i][j].compare("priceToValue")==0){
          ReportingFunctions::appendValuationTable(
            latexReport,
            tickerMetaData.primaryTicker,
            calculateData,
            date,
            verbose);
        }
      }  
    }      

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
  std::string plotConfigurationFilePath;  
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

    TCLAP::ValueArg<std::string> plotConfigurationFilePathInput("c",
      "configuration_file", 
      "The path to the csv file that contains the names of the metrics "
      " to plot. Note that the keywords historicalData"
      "and empty are special: historicalData will produce a plot of the "
      "adjusted end-of-day stock price, and empty will result in an empty plot",
      true,"","string");
    cmd.add(plotConfigurationFilePathInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    singleFileToEvaluate      = singleFileToEvaluateInput.getValue();
    exchangeCode              = exchangeCodeInput.getValue();  
    plotConfigurationFilePath = plotConfigurationFilePathInput.getValue();      
    calculateDataFolder       = calculateDataFolderInput.getValue();
    fundamentalFolder         = fundamentalFolderInput.getValue();
    historicalFolder          = historicalFolderInput.getValue();    
    reportFolder              = reportFolderOutput.getValue();
    dateOfTable               = dateOfTableInput.getValue();
    verbose                   = verboseInput.getValue();

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

      std::cout << "  Plot Configuration File" << std::endl;
      std::cout << "    " << plotConfigurationFilePath << std::endl;          
    }

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  bool replaceNansWithMissingData = true;

  PlottingFunctions::PlotSettings plotSettings;
  std::vector< std::vector < std::string > > subplotMetricNames;
  readConfigurationFile(plotConfigurationFilePath,subplotMetricNames);

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

        //
        // Generate and save the pdf
        //
        if(!isTickerProcessed && tickerMetaData.companyName.length() > 0){

          std::filesystem::path outputPlotFilePath = outputFolderPath;
          std::string plotFileName("fig_");
          plotFileName.append(tickerFolderName);
          plotFileName.append(".pdf");
          outputPlotFilePath.append(plotFileName);
        
          bool successPlotTickerData = 
            plotTickerData(
              tickerMetaData,
              fundamentalData,
              historicalData,
              calculateData,
              outputPlotFilePath.c_str(),
              subplotMetricNames,
              plotSettings,
              false);

          std::filesystem::path outputDebuggingPlotFilePath = outputFolderPath;
          std::string plotDebuggingFileName("fig_");
          plotDebuggingFileName.append(tickerFolderName);
          plotDebuggingFileName.append("_Debugging.pdf");
          outputDebuggingPlotFilePath.append(plotDebuggingFileName);

          bool successPlotDebuggingData=
            plotDebuggingData(
              tickerMetaData,
              fundamentalData,
              historicalData,
              calculateData,
              outputDebuggingPlotFilePath.c_str(),
              plotSettings,
              false);

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
              plotFileName.c_str(),
              plotDebuggingFileName.c_str(),
              outputReportFilePath.c_str(),    
              subplotMetricNames,
              replaceNansWithMissingData,
              false); 

          std::string wrapperFileName("report_");
          wrapperFileName.append(reportFileName);

          outputReportFilePath = outputFolderPath;
          outputReportFilePath.append(wrapperFileName);

          bool successGenerateLatexReportWrapper =
            generateLaTeXReportWrapper(outputReportFilePath.c_str(), 
                                       reportFileName, 
                                       tickerMetaData,
                                       false);

          processedTickers.push_back(tickerMetaData.primaryTicker);

          if(verbose){
            if(  !successPlotTickerData 
              || !successGenerateLaTeXReport  
              || !successGenerateLatexReportWrapper){
              std::cout << '\t' << "skipping" << std::endl;
              std::cout << '\t' << successPlotTickerData 
                        << '\t' << "plotting" << std::endl;
              std::cout << '\t' << successGenerateLaTeXReport 
                        << '\t' << "report generation" << std::endl;
              std::cout << '\t' << successGenerateLatexReportWrapper 
                        << '\t' << "report wrapper generation" << std::endl;

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
