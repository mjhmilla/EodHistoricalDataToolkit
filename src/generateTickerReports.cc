//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT


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

#include "FinancialAnalysisFunctions.h"
#include "JsonFunctions.h"
#include "ReportingFunctions.h"
#include "PlottingFunctions.h"

//==============================================================================
enum DataType{
  DateData = 0,
  NumericalData,
  DATA_TYPE
};

//==============================================================================
struct JsonMetaData{
  //std::string ticker;
  const nlohmann::ordered_json& jsonData;
  std::string jsonDataName;
  std::vector<std::string> address;
  std::string fieldName;
  std::string dateName;
  DataType type;
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

  std::string fileName;
  std::vector< std::string > fieldNames;
  std::string label;
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
DataType extractDataVector(const JsonMetaData &jsonMetaData,
                      std::vector< double >& dataUpd)
{
  DataType typeOfData = DataType::DATA_TYPE;
  nlohmann::json::value_t dataType = 
    JsonFunctions::getDataType( jsonMetaData.jsonData, 
                                jsonMetaData.address,
                                jsonMetaData.fieldName.c_str(),
                                jsonMetaData.isArray);
  
  //If the data is a series of numbers                                
  if(    (dataType == nlohmann::json::value_t::number_float) 
      || (dataType == nlohmann::json::value_t::number_integer)
      || (dataType == nlohmann::json::value_t::number_unsigned)
      || (dataType == nlohmann::json::value_t::array)
      || (dataType == nlohmann::json::value_t::null)){
    JsonFunctions::extractFloatSeries(jsonMetaData.jsonData,
                                      jsonMetaData.address,
                                      jsonMetaData.fieldName.c_str(),
                                      jsonMetaData.isArray,
                                      dataUpd);
    typeOfData = DataType::NumericalData;

  }else if(dataType == nlohmann::json::value_t::string){
    //Get one element. Check to see if it is a date
    std::vector< std::string > stringTest;
    JsonFunctions::extractStringSeries(jsonMetaData.jsonData,
                                      jsonMetaData.address,
                                      jsonMetaData.fieldName.c_str(),
                                      jsonMetaData.isArray,
                                      stringTest,
                                      1);
      

    if(DateFunctions::isDate(stringTest[0])){

      JsonFunctions::StringConversionSettings settings(
          JsonFunctions::StringConversion::StringToNumericalDate,
          DefaultDateFormat);   
      
      JsonFunctions::extractStringConvertToFloatSeries(
                        jsonMetaData.jsonData,
                        jsonMetaData.address,
                        jsonMetaData.fieldName.c_str(),
                        settings,
                        dataUpd);
      typeOfData = DataType::DateData;
    }else{
      JsonFunctions::StringConversionSettings settings(
          JsonFunctions::StringConversion::StringToFloat,
          DefaultDateFormat);   
      
      JsonFunctions::extractStringConvertToFloatSeries(
                        jsonMetaData.jsonData,
                        jsonMetaData.address,
                        jsonMetaData.fieldName.c_str(),
                        settings,
                        dataUpd);     
      typeOfData = DataType::NumericalData;                                     
    }  
  }else{
    std::cout << "Error: Unrecognized json type"
              << std::endl;
    std::abort();
  }
  
  return typeOfData;
};

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

  std::vector< std::vector< PlottingFunctions::AxisSettings > > axisSettings;


  if(matrixOfPlots.size() != nrows){
    matrixOfPlots.resize(nrows);
    axisSettings.resize(nrows);
  }
  for(size_t i=0; i < nrows; ++i){
    if(matrixOfPlots[i].size() != ncols){
      matrixOfPlots[i].resize(ncols);
      axisSettings[i].resize(ncols);
    }
  }

  //When data in the record is nan, I'll plot a flat line. This is
  //to prevent the case of an empty Plot2D structure from being plotted,
  //which causes sciplot to plot a blank.
  std::vector< double > xEmpty;
  std::vector< double > yEmpty;
  double xDelta=0.0; //Set to 0.01 before so that the axis were widened a bit
  double yDelta=0.0;

  xEmpty.push_back(std::numeric_limits<double>::infinity());
  xEmpty.push_back(-std::numeric_limits<double>::infinity());
  for(auto& el : fundamentalData[FIN][BAL][Y].items()){
    std::string dateString; 
    JsonFunctions::getJsonString(el.value()["date"],dateString);
    double date = DateFunctions::convertToFractionalYear(dateString);
    if(date < xEmpty[0]){
      xEmpty[0] = date;
    }
    if(date > xEmpty[1]){
      xEmpty[1] = date;
    }
  }
  yEmpty.push_back(0);
  yEmpty.push_back(0);    


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

  
  std::vector< std::string > coordStringVector;
  coordStringVector.push_back("_x");
  coordStringVector.push_back("_y");

  for(auto &plotConfig : plotConfiguration["plots"].items()){

    std::vector< JsonMetaData > jsonMetaDataVector;
    bool isDateDataSeries=false;
    for(size_t i=0; i<coordStringVector.size();++i){
      std::string coordStr = coordStringVector[i]; 
      std::string jsonData;

      // In the case where both fieldName_x and fieldName_y share the same
      // json object and address, the code below makes will just copy over
      // the json object and address from the _x JsonMetadata to the _y 
      // JsonMetadata. This saves the person writing the plot config file
      // from having to perfectly copy over these fields.
      if(    coordStr.compare("_y") == 0 
          && plotConfig.value().contains("jsonData_x") 
          && plotConfig.value().contains("address_x")
          && plotConfig.value().contains("fieldName_x")
          && plotConfig.value().contains("fieldName_y")
          && !plotConfig.value().contains("jsonData_y")
          && !plotConfig.value().contains("address_y")){
        //The x and y fields have the values for jsonData and address. Copy 
        //these over
        size_t indexEnd = jsonMetaDataVector.size()-1;
        JsonMetaData jsonMetaData(jsonMetaDataVector[indexEnd]);
        
        if(jsonMetaDataVector[indexEnd].fieldName.find("date") 
            != std::string::npos){
          isDateDataSeries=true;
        }

        //Update the field name because this does differ
        std::string fieldName;
        JsonFunctions::getJsonString(
            plotConfig.value()["fieldName"+coordStr],fieldName);

        jsonMetaData.fieldName = fieldName;

        jsonMetaDataVector.push_back(jsonMetaData);



      }else{

        JsonFunctions::getJsonString(
            plotConfig.value()["jsonData"+coordStr], jsonData);  

        if(jsonData.compare("fundamentalData")==0){
          JsonMetaData jsonMetaData(fundamentalData);
          jsonMetaData.jsonDataName = std::string("fundamentalData");
          jsonMetaDataVector.push_back(jsonMetaData);
          

        }else if(jsonData.compare("historicalData")==0){
          JsonMetaData jsonMetaData(historicalData);
          jsonMetaData.jsonDataName = std::string("historicalData");
          jsonMetaDataVector.push_back(jsonMetaData);


        }else if(jsonData.compare("calculateData")==0){
          JsonMetaData jsonMetaData(calculateData);
          jsonMetaData.jsonDataName = std::string("calculateData");
          jsonMetaDataVector.push_back(jsonMetaData);

        }else{
          std::cout << "Error: jsonData type not recognized. The field jsonData"
                    << coordStr << " "
                    << "needs to be in the set [fundamentalData, historicalData, "
                    << "calculateData] but instead this was passed in: "
                    << jsonData
                    << std::endl;
          std::abort();                
        }

        size_t indexEnd = jsonMetaDataVector.size()-1;
        jsonMetaDataVector[indexEnd].address.clear();
        for(auto &addressItem : plotConfig.value()["address"+coordStr].items()){
          std::string fieldName;
          JsonFunctions::getJsonString(addressItem.value(),fieldName);
          jsonMetaDataVector[indexEnd].address.push_back(fieldName);
        }
        std::string fieldName;
        JsonFunctions::getJsonString(plotConfig.value()["fieldName"+coordStr],fieldName);
        jsonMetaDataVector[indexEnd].fieldName = fieldName;

        jsonMetaDataVector[indexEnd].isArray = 
          JsonFunctions::getJsonBool(plotConfig.value()["isArray"]);

        if(plotConfig.value().contains("dateName"+coordStr)){
          std::string dateName;
          JsonFunctions::getJsonString(plotConfig.value()["dateName"+coordStr],
                                      dateName);
          jsonMetaDataVector[indexEnd].dateName = dateName;
        }
      }

    }



    if(jsonMetaDataVector.size() != 2){
      std::cout << "Error: expected to find x and y data in " 
                << plotConfig.key().c_str()
                << " but instead found only " 
                << jsonMetaDataVector.size()
                << " valid series."
                << std::endl;
      std::abort();
    }

    //Get the data
    std::vector<double> xTmp;
    std::vector<double> xDate;
    std::vector<double> yTmp;
    std::vector<double> yDate;

    DataType xDataType = extractDataVector(jsonMetaDataVector[0],xTmp);
    DataType yDataType =  extractDataVector(jsonMetaDataVector[1],yTmp);    


    if(jsonMetaDataVector[0].dateName.size()>0){
      JsonMetaData jmd(jsonMetaDataVector[0]);
      jmd.fieldName=jsonMetaDataVector[0].dateName;
      DataType typeOfDataTmp = extractDataVector(jmd,xDate);
    }
    if(jsonMetaDataVector[1].dateName.size()>0){
      JsonMetaData jmd(jsonMetaDataVector[1]);
      jmd.fieldName=jsonMetaDataVector[1].dateName;
      DataType typeOfDataTmp = extractDataVector(jmd,yDate);
    }


    if(xDate.size() > 0 && yDate.size() > 0 && xTmp.size() != yTmp.size()){
      
      std::vector< double > xTmpUpd;
      std::vector< double > yTmpUpd;

      size_t jPrev =0;
      for(size_t i = 0; i < xDate.size(); ++i){
        size_t j=jPrev;
        double dateErr = 1.0;
        bool found = false;
        while(!found && j < yDate.size()){
          dateErr = xDate[i]-yDate[j];                    
          if(std::abs(dateErr) > 0.05){
            found = false;
            ++j;
          }else{
            found=true;
          }
        }
        jPrev=j;
        if(found){
          xTmpUpd.push_back(xTmp[i]);
          yTmpUpd.push_back(yTmp[j]);
        }
      }
      xTmp.clear();
      yTmp.clear();
      xTmp = xTmpUpd;
      yTmp = yTmpUpd;

    }

    if(xTmp.size() != yTmp.size()){
      std::cout << "Error: x and y data series do not have matching sizes"
                << std::endl;
      std::abort();
    }

    //Remove nan values
    size_t i=0;
    while(i < xTmp.size()){
      if(std::isnan(xTmp[i]) || std::isnan(yTmp[i])){
        xTmp.erase(xTmp.begin()+i);        
        yTmp.erase(yTmp.begin()+i);
        
        if(xDate.size()>0){
          xDate.erase(xDate.begin()+i);
        }
        if(yDate.size()>0){
          yDate.erase(yDate.begin()+i);
        }
      }else{
        ++i;
      }
    }


    //Get the LineSettings
    bool addLine = plotConfig.value().contains("lineWidth");
    PlottingFunctions::LineSettings lineSettings;        
    if(addLine){
      JsonFunctions::getJsonString(plotConfig.value()["lineColor"],
                                      lineSettings.colour);

      JsonFunctions::getJsonString(plotConfig.value()["legendEntry"],
                                      lineSettings.name); 

      lineSettings.lineWidth = 
        JsonFunctions::getJsonFloat(plotConfig.value()["lineWidth"],false);

      findReplaceKeywords(lineSettings.name,keywords,replacements);
    }

    bool addPoints = plotConfig.value().contains("pointSize");
    PlottingFunctions::PointSettings pointSettings;        
    if(addPoints){
      JsonFunctions::getJsonString(plotConfig.value()["pointColor"],
                                   pointSettings.colour);

      JsonFunctions::getJsonString(plotConfig.value()["legendEntry"],
                                   pointSettings.name); 

      pointSettings.pointSize = 
        JsonFunctions::getJsonFloat(plotConfig.value()["pointSize"],false);
      
      pointSettings.pointType = static_cast<int>(
          JsonFunctions::getJsonFloat(plotConfig.value()["pointType"],false)
        );
    

      findReplaceKeywords(pointSettings.name,keywords,replacements);
    }




    //Get SubplotSettings
    PlottingFunctions::SubplotSettings subplotSettings;

    tmp = JsonFunctions::getJsonFloat(plotConfig.value()["row"],false);
    subplotSettings.indexRow = static_cast<size_t>(tmp);
    size_t indexRow = subplotSettings.indexRow;

    tmp = JsonFunctions::getJsonFloat(plotConfig.value()["column"],false);
    subplotSettings.indexColumn = static_cast<size_t>(tmp);
    size_t indexColumn = subplotSettings.indexColumn;

    if(indexRow == 0 && indexColumn ==1){
      bool here=true;
    }

    //Get the AxisSettings
    std::string xAxisLabel, yAxisLabel;
    JsonFunctions::getJsonString(plotConfig.value()["xAxisLabel"],
                                 axisSettings[indexRow][indexColumn].xAxisName);        
    
    findReplaceKeywords(axisSettings[indexRow][indexColumn].xAxisName,
                        keywords,replacements);
                                 
    JsonFunctions::getJsonString(plotConfig.value()["yAxisLabel"],
                                 axisSettings[indexRow][indexColumn].yAxisName);  

    findReplaceKeywords(axisSettings[indexRow][indexColumn].yAxisName,
                        keywords,replacements);

    //Only update the axis if the values have not been already set
    if( std::isnan(axisSettings[indexRow][indexColumn].xMin) ){
      double xMin = JsonFunctions::getJsonFloat(plotConfig.value()["xMin"],false);
      axisSettings[indexRow][indexColumn].xMin = xMin;

      if(std::isnan(xMin)){
        axisSettings[indexRow][indexColumn].isXMinFixed=false;  
      }else{
        axisSettings[indexRow][indexColumn].isXMinFixed=true;  
      }
        
    }


    if(xTmp.size()>0 && yTmp.size()>0 && xTmp.size()==yTmp.size()){
      //
      // get the index of the most recent data.
      //
      int indexOfMostRecentData=-1;
      if(xDate.size()>0){
        auto iter = std::max_element(xDate.rbegin(), xDate.rend()).base();
        indexOfMostRecentData = std::distance(xDate.begin(), std::prev(iter));

      }else if(xDataType == DataType::DateData){
        auto iter = std::max_element(xTmp.rbegin(), xTmp.rend()).base();
        indexOfMostRecentData = std::distance(xTmp.begin(), std::prev(iter));

      }else if(yDataType == DataType::DateData){
        auto iter = std::max_element(yTmp.rbegin(), yTmp.rend()).base();
        indexOfMostRecentData = std::distance(yTmp.begin(), std::prev(iter));        
      }

      DataStructures::SummaryStatistics metricSummaryStatistics;
      bool validSummaryStats = 
        NumericalFunctions::extractSummaryStatistics(yTmp,
                            metricSummaryStatistics);
      metricSummaryStatistics.current = std::nan("1");
      metricSummaryStatistics.current = yTmp[indexOfMostRecentData];

    
      //
      // update the axes as necessary
      //
      double xMaxData = *std::max_element(xTmp.begin(),xTmp.end());
      double xMinData = *std::min_element(xTmp.begin(),xTmp.end());
      double yMaxData = *std::max_element(yTmp.begin(),yTmp.end());
      double yMinData = *std::min_element(yTmp.begin(),yTmp.end());


      double xMaxConfig = 
        JsonFunctions::getJsonFloat(plotConfig.value()["xMax"], false);
      double xMinConfig = 
        JsonFunctions::getJsonFloat(plotConfig.value()["xMin"], false);
      double yMaxConfig = 
        JsonFunctions::getJsonFloat(plotConfig.value()["yMax"], false);
      double yMinConfig = 
        JsonFunctions::getJsonFloat(plotConfig.value()["yMin"], false);
      

      if(std::isnan(xMaxConfig)){
        axisSettings[indexRow][indexColumn].isXMaxFixed=false;
        if(std::isnan(axisSettings[indexRow][indexColumn].xMax)){
          axisSettings[indexRow][indexColumn].xMax = xMaxData+xDelta;
        }else{
          axisSettings[indexRow][indexColumn].xMax = 
            std::max(axisSettings[indexRow][indexColumn].xMax,xMaxData);
        }
      }else{
        axisSettings[indexRow][indexColumn].isXMaxFixed=true;  
        axisSettings[indexRow][indexColumn].xMax = xMaxConfig;        
      }
      if(xDataType != DataType::DateData){
        axisSettings[indexRow][indexColumn].xMax
          = NumericalFunctions::roundToNearest(
              axisSettings[indexRow][indexColumn].xMax, 3);
      }     

      if(std::isnan(xMinConfig)){
        axisSettings[indexRow][indexColumn].isXMinFixed=false;
        if(std::isnan(axisSettings[indexRow][indexColumn].xMin)){
          axisSettings[indexRow][indexColumn].xMin = xMinData-xDelta;
        }else{
          axisSettings[indexRow][indexColumn].xMin = 
            std::min(axisSettings[indexRow][indexColumn].xMin,xMinData);
        }
      }else{      
        axisSettings[indexRow][indexColumn].isXMinFixed=true;  
        axisSettings[indexRow][indexColumn].xMin =xMinConfig;        
      }
      if(xDataType != DataType::DateData){
        axisSettings[indexRow][indexColumn].xMin 
          = NumericalFunctions::roundToNearest(
              axisSettings[indexRow][indexColumn].xMin, 3);
      }        

      if(std::isnan(yMaxConfig)){
        axisSettings[indexRow][indexColumn].isYMaxFixed=false;
        if(std::isnan(axisSettings[indexRow][indexColumn].yMax)){
          axisSettings[indexRow][indexColumn].yMax = yMaxData+yDelta;
        }else{          
          axisSettings[indexRow][indexColumn].yMax = 
            std::max(axisSettings[indexRow][indexColumn].yMax,yMaxData+yDelta);
        }
      }else{
        axisSettings[indexRow][indexColumn].isYMaxFixed=true;  
        axisSettings[indexRow][indexColumn].yMax = yMaxConfig;
      }
      if(yDataType != DataType::DateData){
        axisSettings[indexRow][indexColumn].yMax 
          = NumericalFunctions::roundToNearest(
              axisSettings[indexRow][indexColumn].yMax, 3);
      }


      if(std::isnan(yMinConfig)){
        axisSettings[indexRow][indexColumn].isYMinFixed=false;
        if(std::isnan(axisSettings[indexRow][indexColumn].yMin)){
          axisSettings[indexRow][indexColumn].yMin = yMinData-yDelta;
        }else{
          axisSettings[indexRow][indexColumn].yMin = 
            std::min(axisSettings[indexRow][indexColumn].yMin,yMinData-yDelta);
        }
      }else{
        axisSettings[indexRow][indexColumn].isYMinFixed=true;  
        axisSettings[indexRow][indexColumn].yMin =yMinConfig;
      }
      if(yDataType != DataType::DateData){
        axisSettings[indexRow][indexColumn].yMin 
          = NumericalFunctions::roundToNearest(
              axisSettings[indexRow][indexColumn].yMin, 3);
      }


      if(!std::isnan(xMaxData) && !std::isnan(xMinData)){
          if(xDataType==DataType::DateData){
            plotSettingsUpd.xticMinimumIncrement = 
              std::round((axisSettings[indexRow][indexColumn].xMax
                        - axisSettings[indexRow][indexColumn].xMin)/5.0);
          }else{
            plotSettingsUpd.xticMinimumIncrement = 
              std::round((axisSettings[indexRow][indexColumn].xMax
                        - axisSettings[indexRow][indexColumn].xMin)/2.0);
            //plotSettingsUpd.xticMinimumIncrement = 
            //  NumericalFunctions::roundToNearest(
            //      plotSettingsUpd.xticMinimumIncrement, 3);
               
          }
      }      
      //
      //Get Box and Whisker Settings
      //
      PlottingFunctions::BoxAndWhiskerSettings boxWhiskerSettings;

      
      if(plotConfig.value().contains("boxWhiskerPosition")){
        tmp = JsonFunctions::getJsonFloat(
            plotConfig.value()["boxWhiskerPosition"],false);

        if(tmp >= 0){
          boxWhiskerSettings.xOffsetFromStart=tmp;
        }else{
          boxWhiskerSettings.xOffsetFromEnd = std::abs(tmp);
        }      
      }

      if(plotConfig.value().contains("boxWhiskerPositionPercentage")){
        tmp = JsonFunctions::getJsonFloat(
            plotConfig.value()["boxWhiskerPositionPercentage"],false);

        if(tmp >= 0){
          boxWhiskerSettings.xOffsetFromStart=tmp*(xMaxData-xMinData);
        }else{
          boxWhiskerSettings.xOffsetFromEnd = std::abs(tmp)*(xMaxData-xMinData);
        }      
      }


      JsonFunctions::getJsonString(plotConfig.value()["boxWhiskerColor"],
                                  boxWhiskerSettings.boxWhiskerColour);

      if(addLine){                                 
        boxWhiskerSettings.currentValueColour = lineSettings.colour;
      }else if(addPoints){
        boxWhiskerSettings.currentValueColour = pointSettings.colour;
      }else{
        boxWhiskerSettings.currentValueColour = "cyan";
      }      



      PlottingFunctions::updatePlot(
          xTmp,
          yTmp,
          metricSummaryStatistics,
          plotSettingsUpd,
          lineSettings,
          pointSettings,
          axisSettings[indexRow][indexColumn],
          boxWhiskerSettings,
          matrixOfPlots[subplotSettings.indexRow][subplotSettings.indexColumn],
          true,
          verbose);


  
    }
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
  latexReport << "\\usepackage{enumitem}" << std::endl;
  latexReport << "\\begin{document}"<<std::endl;
  latexReport << "\\input{" << reportFileName << "}" << std::endl;
  latexReport << "\\end{document}" << std::endl;
  latexReport.close();

};

//==============================================================================
bool generateLaTeXReport(
    const TickerMetaData &tickerMetaData,
    const nlohmann::ordered_json &fundamentalData,
    const nlohmann::ordered_json &historicalData,
    const nlohmann::ordered_json &calculateData,
    const std::vector< std::string > &plotFileNames,
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

  metric.fileName = "fundamentalData";
  metric.label="ISIN";
  metric.fieldNames.push_back("General");
  metric.fieldNames.push_back("ISIN");
  metric.type = JSON_FIELD_TYPE::STRING;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label="Currency Code";
  metric.fieldNames.push_back("General");
  metric.fieldNames.push_back("CurrencyCode");
  metric.type = JSON_FIELD_TYPE::STRING;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";  
  metric.label="IPO Date";
  metric.fieldNames.push_back("General");
  metric.fieldNames.push_back("IPODate");
  metric.type = JSON_FIELD_TYPE::STRING;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label="Market Cap (Mln)";
  metric.fieldNames.push_back("Highlights");
  metric.fieldNames.push_back("MarketCapitalizationMln");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.label.clear();
  metric.fileName.clear();
  metric.type = JSON_FIELD_TYPE::JSON_FIELD_TYPE_SIZE;
  tabularMetrics.push_back(metric);


  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label="Beta";
  metric.fieldNames.push_back("Technicals");
  metric.fieldNames.push_back("Beta");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label="EV/EBITA";
  metric.fieldNames.push_back("Valuation");
  metric.fieldNames.push_back("EnterpriseValueEbitda");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.label.clear();
  metric.fileName.clear();
  metric.type = JSON_FIELD_TYPE::JSON_FIELD_TYPE_SIZE;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label="Dividend Yield";
  metric.fieldNames.push_back("Highlights");
  metric.fieldNames.push_back("DividendYield");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "calculateData";
  metric.label="Avg. ATOI Growth";
  metric.fieldNames.push_back("atoi_growth_model_avg");
  metric.fieldNames.push_back("annualGrowthRateOfTrendline");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "calculateData";
  metric.label="Cyclic ATOI Percentile";
  metric.fieldNames.push_back("atoi_growth_model_avg");
  metric.fieldNames.push_back("yCyclicNormDataPercentilesRecent");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);
  
  metric.fieldNames.clear();
  metric.fileName.clear();
  metric.label.clear();
  metric.type = JSON_FIELD_TYPE::JSON_FIELD_TYPE_SIZE;
  tabularMetrics.push_back(metric);


  metric.fieldNames.clear();
  metric.fileName = "calculateData";
  metric.label="Avg. Price Growth";
  metric.fieldNames.push_back("price_growth_model");
  metric.fieldNames.push_back("annualGrowthRateOfTrendline");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "calculateData";
  metric.label="Cyclic Price Percentile";
  metric.fieldNames.push_back("price_growth_model");
  metric.fieldNames.push_back("yCyclicNormDataPercentilesRecent");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);


  metric.fieldNames.clear();
  metric.fileName.clear();
  metric.label.clear();
  metric.type = JSON_FIELD_TYPE::JSON_FIELD_TYPE_SIZE;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Percent Insiders";
  metric.fieldNames.push_back("SharesStats");
  metric.fieldNames.push_back("PercentInsiders");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Percent Institutions";
  metric.fieldNames.push_back("SharesStats");
  metric.fieldNames.push_back("PercentInstitutions");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName.clear();
  metric.label.clear();
  metric.type = JSON_FIELD_TYPE::JSON_FIELD_TYPE_SIZE;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "historicalData";
  metric.label = "Price (adj. close)";
  metric.fieldNames.push_back("adjusted_close");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Target Price";
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("TargetPrice");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "52 Week High";
  metric.fieldNames.push_back("Technicals");
  metric.fieldNames.push_back("52WeekHigh");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "52 Week Low";
  metric.fieldNames.push_back("Technicals");
  metric.fieldNames.push_back("52WeekLow");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);


  metric.fieldNames.clear();
  metric.fileName.clear();
  metric.label.clear();
  metric.type = JSON_FIELD_TYPE::JSON_FIELD_TYPE_SIZE;
  tabularMetrics.push_back(metric);


  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.fieldNames.push_back("AnalystRatings");
  metric.label = "Strong Buy";
  metric.fieldNames.push_back("StrongBuy");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Buy";  
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("Buy");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Hold";
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("Hold");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Sell";
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("Sell");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Strong Sell";
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("StrongSell");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName.clear();
  metric.label.clear();
  metric.type = JSON_FIELD_TYPE::JSON_FIELD_TYPE_SIZE;
  tabularMetrics.push_back(metric);


  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Shares Short";
  metric.fieldNames.push_back("Technicals");
  metric.fieldNames.push_back("SharesShort");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Shares Short Prior Month";
  metric.fieldNames.push_back("Technicals");
  metric.fieldNames.push_back("SharesShortPriorMonth");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Short Ratio";
  metric.fieldNames.push_back("Technicals");
  metric.fieldNames.push_back("ShortRatio");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fileName = "fundamentalData";
  metric.label = "Short Percent";
  metric.fieldNames.push_back("Technicals");
  metric.fieldNames.push_back("ShortPercent");
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

    for(auto &el : plotFileNames){
      latexReport << "\\includegraphics{" 
                  <<      el << "}\\\\" << std::endl;
      latexReport << companyNameString
                  << " (" << primaryTickerString <<") "
                  << tickerMetaData.country << " ( \\url{" << webURL << "} )\\\\"
                  << std::endl;
      latexReport << std::endl;
      latexReport << "\\break"          << std::endl;
    }


    latexReport << "\\begin{multicols}{2}" << std::endl;
    latexReport << "\\raggedcolumns" << std::endl;

    //latexReport << "\\begin{center}" << std::endl;
    latexReport << "\\begin{center}" << std::endl;
    latexReport << "\\Large{\\underline{I. Preliminaries}} \\\\" << std::endl;
    latexReport << "\\end{center}" << std::endl;

    latexReport << description << std::endl;
    latexReport << std::endl;


    latexReport << "\\bigskip" << std::endl;
    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\multicolumn{2}{c}{\\textbf{Contextual Data}} \\\\" << std::endl;

    for(size_t  indexTabularMetrics=0; 
                indexTabularMetrics < tabularMetrics.size();
              ++indexTabularMetrics)
    {
      JsonFieldAddress entryMetric = tabularMetrics[indexTabularMetrics];
      std::string lastFieldName("");

      if(entryMetric.fieldNames.size()>0){
        lastFieldName = entryMetric.fieldNames.back();
        ReportingFunctions::convertCamelCaseToSpacedText(lastFieldName);
      }


      bool fieldExists = false;
      if(entryMetric.fileName.compare("calculateData")==0){
        double value = JsonFunctions::getJsonFloat(calculateData,
                          entryMetric.fieldNames,false);

        latexReport << entryMetric.label  
                    << " & " << value << "\\\\" << std::endl;  

        fieldExists=true;
      }

      if(entryMetric.fileName.compare("historicalData")==0){
        double value=-1;
        std::string todaysDate;
        DateFunctions::getTodaysDate(todaysDate);

        int closestDifferenceDays=std::numeric_limits<int>::infinity();
        int indexStart=0;
        int indexNext = 1;
        int indexEnd = historicalData.size()-1;

        std::string dateStart("");
        JsonFunctions::getJsonString(historicalData.front()["date"],dateStart);
        std::string dateEnd("");
        JsonFunctions::getJsonString(historicalData.back()["date"],dateEnd);
      
        int differenceStart = 
          DateFunctions::calcDifferenceInDaysBetweenTwoDates(
              todaysDate,"%Y-%m-%d",dateStart,"%Y-%m-%d");

        int differenceEnd = 
          DateFunctions::calcDifferenceInDaysBetweenTwoDates(
              todaysDate,"%Y-%m-%d",dateEnd,"%Y-%m-%d");

        if(std::abs(differenceEnd) < std::abs(differenceStart)){
          indexStart = historicalData.size()-1;
          indexNext = -1;
          indexEnd = 0;
        }

        int differenceBest = std::numeric_limits<int>::max();
        std::string dateBest("");
        int previousDifference = -1;

        for(int index=indexStart; index != indexEnd;index=index+indexNext){
          
          std::string dateString("");
          JsonFunctions::getJsonString(historicalData.at(index)["date"],dateString);
          int differenceDays = 
          DateFunctions::calcDifferenceInDaysBetweenTwoDates(
              todaysDate,"%Y-%m-%d",dateString,"%Y-%m-%d");

          if(differenceDays >= 0 && differenceDays < differenceBest){
            differenceBest = differenceDays;
            dateBest=dateString;
            value = JsonFunctions::getJsonFloat(
                      historicalData.at(index)[entryMetric.fieldNames[0]],false);
          }
          if(previousDifference > -1 
              && std::abs(previousDifference) < std::abs(differenceDays)){
            break;
          }
          previousDifference = std::abs(differenceDays);

        }  

        latexReport << "Price (" << dateBest << ")" 
                    << " & " << value << "\\\\" << std::endl;  
        fieldExists=true;                        
              
      }

      if(entryMetric.fileName.compare("fundamentalData")==0 
        && lastFieldName.length()>0){

        if(lastFieldName.length()>0){
          fieldExists = JsonFunctions::doesFieldExist(fundamentalData, 
                                                  entryMetric.fieldNames);
        }

        if(fieldExists){
          switch(entryMetric.type){
            case JSON_FIELD_TYPE::BOOL : {
              bool value = JsonFunctions::getJsonBool(fundamentalData,
                                entryMetric.fieldNames);

              latexReport << entryMetric.label << " & " << value  << "\\\\" << std::endl;                          
            }
            break;
            case JSON_FIELD_TYPE::FLOAT : {
              double value = JsonFunctions::getJsonFloat(fundamentalData,
                              entryMetric.fieldNames, 
                              replaceNansWithMissingData);

              latexReport << entryMetric.label << " & " 
                          << ReportingFunctions::formatJsonEntry( value ) 
                          << "\\\\" << std::endl;                          
            }
            break;
            case JSON_FIELD_TYPE::STRING : {
              std::string value;
              JsonFunctions::getJsonString(fundamentalData,
                                entryMetric.fieldNames, 
                                value);

              latexReport << entryMetric.label << " & " << value << "\\\\" << std::endl;                          
            }
            break;
            case JSON_FIELD_TYPE::JSON_FIELD_TYPE_SIZE :{
              latexReport  << " & \\\\" << std::endl;
            }
            break;
          };
        }
      }

      if(!fieldExists){
        latexReport << lastFieldName << " & "  << "\\\\" << std::endl;
      }

    }
    latexReport << "\\end{tabular}" << std::endl << std::endl;
    latexReport << "\\bigskip" << std::endl;
    //std::string date = calculateData.begin().key();

    //Add a summary table
    latexReport << "\\bigskip" << std::endl;
    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\multicolumn{2}{c}{\\textbf{Annual Milestone Data}} \\\\" 
                << std::endl;

    latexReport << "Years since IPO & "
      << ReportingFunctions::formatJsonEntry(JsonFunctions::getJsonFloat(
          calculateData["annual_milestones"]["years_since_IPO"],true))
      << "\\\\" << std::endl;
    latexReport << "Years in EOD record & "
      << ReportingFunctions::formatJsonEntry(JsonFunctions::getJsonFloat(
          calculateData["annual_milestones"]["years_reported"],true))
      << "\\\\" << std::endl;      
    latexReport << "Years of value creation (ROIC-CC $>$ 0) & "
      << ReportingFunctions::formatJsonEntry(JsonFunctions::getJsonFloat(
          calculateData["annual_milestones"]["years_value_created"],true))
      << "\\\\" << std::endl;      
    latexReport << "Years with a dividend & "
      << ReportingFunctions::formatJsonEntry(JsonFunctions::getJsonFloat(
          calculateData["annual_milestones"]["years_with_dividend"],true))
      << "\\\\" << std::endl;      
    latexReport << "Fraction of years with a dividend increase & "
      << ReportingFunctions::formatJsonEntry(JsonFunctions::getJsonFloat(
          calculateData["annual_milestones"]["fraction_of_years_with_a_dividend_increases"],true))
      << "\\\\" << std::endl;      


    latexReport << "\\end{tabular}" << std::endl << std::endl;
    latexReport << "\\bigskip" << std::endl;

    latexReport << "\\break" << std::endl;
    latexReport << "\\newpage" << std::endl;


    latexReport << "\\begin{center}" << std::endl;
    latexReport << "\\Large{\\underline{II. Supplementary Tables}} \\\\" << std::endl;
    latexReport << "\\end{center}" << std::endl << std::endl;

    //==========================================================================
    // priceToValue - Damodaran's DCF analysis
    //==========================================================================    
    std::string date;
    bool dateFound=false;
    bool isDateMostRecent=true;
    std::vector< std::string > fieldsToTest;
    fieldsToTest.push_back("priceToValue");

    for(const auto& item : calculateData["metric_data"].items()){
      date = item.key();
      bool valid = true;

      for(size_t j=0; j < fieldsToTest.size();++j){
        double value = std::nan("1");
        if(item.value().contains(fieldsToTest[j])){
          value = JsonFunctions::getJsonFloat(
                      item.value()[fieldsToTest[j]], false);
        }
        if(std::isnan(value)){
          valid=false;
          isDateMostRecent=false;
        }
      }
      
      if(valid){
        break;
      }          
    }

    latexReport << "\\begin{center}" << std::endl;    
    latexReport << "\\begin{tabular}{c}" << std::endl;
    if(isDateMostRecent){
        latexReport << "\\hline " << date  
                    << " \\\\ \\hline \\\\" << std::endl;
    }else{
        latexReport << "\\hline \\cellcolor{RedOrange} " 
                    << date <<" \\\\ \\hline \\\\" << std::endl;
    }
    latexReport << "\\end{tabular}" << std::endl;
    latexReport << "\\end{center}" 
                << std::endl << std::endl;   


    ReportingFunctions::appendOperatingMarginTable(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData["metric_data"],
      date,
      verbose);

    ReportingFunctions::appendCashFlowConversionTable(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData["metric_data"],
      date,
      verbose);      

    ReportingFunctions::appendResidualCashflowToEnterpriseValueTable(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData["metric_data"],
      date,
      verbose);

    ReportingFunctions::appendDebtTable(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData["metric_data"],
      date,
      verbose);

    //Append gross margin
    //Append cash conversion ratio


    latexReport << "\\break" << std::endl;
    latexReport << "\\newpage" << std::endl;

    latexReport << "\\begin{center}" << std::endl;
    latexReport << "\\Large{\\underline{III. Business Valuation Tables}} \\\\" << std::endl;
    latexReport << "\\end{center}" << std::endl << std::endl;

    latexReport << "\\begin{center}" << std::endl;    
    latexReport << "\\begin{tabular}{c}" << std::endl;
    if(isDateMostRecent){
        latexReport << "\\hline " << date  
                    << " \\\\ \\hline \\\\" << std::endl;
    }else{
        latexReport << "\\hline \\cellcolor{RedOrange} " 
                    << date <<" \\\\ \\hline \\\\" << std::endl;
    }
    latexReport << "\\end{tabular}" << std::endl;
    latexReport << "\\end{center}" 
                << std::endl << std::endl;   
  


    ReportingFunctions::appendCostOfCapitalTableForValuation(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData["country_data"],
      calculateData["metric_data"],
      date,
      verbose);

    ReportingFunctions::appendReinvestmentRateTableForValuation(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData["metric_data"],
      date,
      verbose);
    
    ReportingFunctions::appendReturnOnInvestedCapitalTableForValuation(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData["metric_data"],
      date,
      verbose);

    ReportingFunctions::appendOperatingIncomeGrowthTableForValuation(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData["metric_data"],
      date,
      verbose);


    std::string tableTitle("Price to DCM-Value (Finance)");
    std::string jsonTableName("priceToValue");

    int tableId = 8;
    latexReport << "\\break" << std::endl;
    latexReport << "\\newpage" << std::endl;
    tableId=ReportingFunctions::appendValuationTable(
      latexReport,
      tickerMetaData.primaryTicker,
      calculateData["metric_data"],
      date,
      tableId,
      tableTitle,
      jsonTableName,
      isDateMostRecent,
      verbose);

    latexReport << "\\break" << std::endl;
    latexReport << "\\newpage" << std::endl;
    ++tableId;

    //==========================================================================
    // priceToValueEmpirical
    //==========================================================================    
    
    dateFound=false;
    fieldsToTest.resize(0);
    fieldsToTest.push_back("priceToValueEmpirical");
    fieldsToTest.push_back("priceToValueEmpiricalAvg");
    isDateMostRecent=true;
    for(const auto& item : calculateData["metric_data"].items()){
      date = item.key();
      bool valid = true;

      for(size_t j=0; j < fieldsToTest.size();++j){
        double value = std::nan("1");
        if(item.value().contains(fieldsToTest[j])){
          value = JsonFunctions::getJsonFloat(
                      item.value()[fieldsToTest[j]], false);
        }
        if(std::isnan(value)){
          valid=false;
          isDateMostRecent=false;
        }
      }      
      if(valid){
        break;
      }          
    }

    tableTitle ="Price to DCM-Value (Empirical)";
    jsonTableName = "priceToValueEmpirical";

    if(calculateData["metric_data"][date].contains(jsonTableName+"_riskFreeRate")){
      tableId = ReportingFunctions::appendValuationTable(
                                      latexReport,
                                      tickerMetaData.primaryTicker,
                                      calculateData["metric_data"],
                                      date,
                                      tableId,
                                      tableTitle,
                                      jsonTableName,
                                      isDateMostRecent,
                                      verbose);

      std::string nameToPrepend("empirical_");
      std::string empTableTitle="Empirically Estimated Growth (recent)";
      ReportingFunctions::appendEmpiricalGrowthTable(
                                  latexReport,
                                  tickerMetaData.primaryTicker,
                                  calculateData["metric_data"],
                                  date,
                                  nameToPrepend,
                                  empTableTitle,
                                  isDateMostRecent,
                                  verbose);


      latexReport << "\\break" << std::endl;
      latexReport << "\\newpage" << std::endl;
      ++tableId;      
    }

    tableTitle ="Price to DCM-Value (Empirical Avg.)";
    jsonTableName = "priceToValueEmpiricalAvg";

    if(calculateData["metric_data"][date].contains(jsonTableName+"_riskFreeRate")){
      tableId=ReportingFunctions::appendValuationTable(
                                    latexReport,
                                    tickerMetaData.primaryTicker,
                                    calculateData["metric_data"],
                                    date,
                                    tableId,
                                    tableTitle,
                                    jsonTableName,
                                    isDateMostRecent,
                                    verbose);

      std::string nameToPrepend="empiricalAvg_";
      std::string empTableTitle="Empirically Estimated Growth (avg. of all)";      
      ReportingFunctions::appendEmpiricalGrowthTable(
                            latexReport,
                            tickerMetaData.primaryTicker,
                            calculateData["metric_data"],
                            date,
                            nameToPrepend,
                            empTableTitle,
                            isDateMostRecent,
                            verbose);
    }

    latexReport << "\\break"          << std::endl;
    latexReport << "\\newpage"        << std::endl;
   

    //==========================================================================
    // priceToValueEpsGrowth
    //==========================================================================
    dateFound=false;
    isDateMostRecent=true;
    fieldsToTest.resize(0);
    fieldsToTest.push_back("priceToValueEpsGrowth_price_to_value");
    fieldsToTest.push_back("priceToValueEpsGrowth_price_to_value_P25");
    fieldsToTest.push_back("priceToValueEpsGrowth_price_to_value_P50");
    fieldsToTest.push_back("priceToValueEpsGrowth_price_to_value_P75");

    for(const auto& item : calculateData["metric_data"].items()){
      date = item.key();
      bool valid = true;

      for(size_t j=0; j < fieldsToTest.size();++j){
        double value = std::nan("1");
        if(item.value().contains(fieldsToTest[j])){
          value = JsonFunctions::getJsonFloat(
                      item.value()[fieldsToTest[j]], false);
        }
        if(std::isnan(value)){
          valid=false;
          isDateMostRecent=false;
        }
      }      
      if(valid){
        break;
      }          
    }

    jsonTableName="priceToValueEpsGrowth";
    if(calculateData["metric_data"][date].contains(jsonTableName+"_equityGrowth")){

      latexReport << "\\begin{center}" << std::endl;
      latexReport << "\\Large{\\underline{IV. Investor Earnings Valuation Tables}} \\\\" 
                  << std::endl;                  
      latexReport << "\\end{center}" << std::endl;

      ReportingFunctions::appendEarningPerShareGrowthValuationTable(
                              latexReport,
                              tickerMetaData.primaryTicker,
                              calculateData["metric_data"],
                              date,
                              jsonTableName,
                              isDateMostRecent,
                              verbose);
    }
    
  }
  latexReport << "\\end{multicols}" << std::endl;
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
  std::string jsonConfigurationFilePath;
  std::string plotSummaryConfigurationFilePath;  
  std::string plotOverviewConfigurationFilePath;  
  std::string reportFolder;

  std::string singleFileToEvaluate;

  bool gapFill;
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

    TCLAP::ValueArg<std::string> jsonConfigurationFilePathInput("c",
      "json_configuration_file", 
      "The path to the json file that contains configuration data for "
      "generateTickerReports",
      true,"","string");
    cmd.add(jsonConfigurationFilePathInput);    

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

    TCLAP::SwitchArg gapFillInput("g","gapfill",
      "Generate ticker reports that appear in calculate data but not"
      " in the output folder.", false);
    cmd.add(gapFillInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    singleFileToEvaluate     = singleFileToEvaluateInput.getValue();
    exchangeCode             = exchangeCodeInput.getValue();  
    jsonConfigurationFilePath = jsonConfigurationFilePathInput.getValue();
    plotSummaryConfigurationFilePath 
                            = plotSummaryConfigurationFilePathInput.getValue();      
    plotOverviewConfigurationFilePath 
                            = plotOverviewConfigurationFilePathInput.getValue();      
    calculateDataFolder      = calculateDataFolderInput.getValue();
    fundamentalFolder        = fundamentalFolderInput.getValue();
    historicalFolder         = historicalFolderInput.getValue();    
    reportFolder             = reportFolderOutput.getValue();
    gapFill                  = gapFillInput.getValue();
    verbose                  = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Single file name to evaluate" << std::endl;
      std::cout << "    " << singleFileToEvaluate << std::endl;



      std::cout << "  Calculate Data Input Folder" << std::endl;
      std::cout << "    " << calculateDataFolder << std::endl;

      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;

      std::cout << "  Report Folder" << std::endl;
      std::cout << "    " << reportFolder << std::endl;  

      std::cout << "  Json Configuration File" << std::endl;
      std::cout << "    " << jsonConfigurationFilePath << std::endl;   

      std::cout << "  Summary Plot Configuration File" << std::endl;
      std::cout << "    " << plotSummaryConfigurationFilePath << std::endl;   

      std::cout << "  Overview Plot Configuration File" << std::endl;
      std::cout << "    " << plotOverviewConfigurationFilePath << std::endl;                

      std::cout << "  Gapfill mode " << std::endl;
      std::cout << "    " << gapFill << std::endl;      
    }

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  bool replaceNansWithMissingData = true;

  //Load the configuration file
  nlohmann::ordered_json config;
  bool loadedConfig = 
    JsonFunctions::loadJsonFile(jsonConfigurationFilePath,
                                config,
                                verbose);

  std::filesystem::path configFilePath(jsonConfigurationFilePath);
  std::filesystem::path configPath(configFilePath.parent_path());

  std::vector< nlohmann::ordered_json > plotSetConfig;
  //std::vector< std::filesystem::path > plotConfigFilePath;

  for( auto &plotItem : config["plots"].items()){
    std::filesystem::path plotItemConfigFilePath(configPath);
    for( auto &pathItem : plotItem.value()["localPath"].items()){
      std::string folderName;
      JsonFunctions::getJsonString(pathItem.value(),folderName);
      plotItemConfigFilePath.append(folderName);
    }
    std::string fileName;
    JsonFunctions::getJsonString(plotItem.value()["fileName"],fileName);
    plotItemConfigFilePath.append(fileName);

    nlohmann::ordered_json plotItemConfig;
    bool loadedPlotData = 
      JsonFunctions::loadJsonFile(plotItemConfigFilePath.string(),
                                  plotItemConfig, 
                                  verbose);
    if(!loadedConfig){
      std::cout << "Error: could not load " 
                << plotItemConfigFilePath.string()
                << " that appears in "
                << jsonConfigurationFilePath
                << std::endl;
      std::abort();
    }else{
      plotSetConfig.push_back(plotItemConfig);
    }                                         
  }

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

  int gapFillCount = 0;
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

    if(verbose && !gapFill){
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



    std::string ticker;
    std::string tickerFolderName;
    std::filesystem::path  outputFolderPath;

    if(validInput){


      //
      // Create the output folder
      //      

      ticker = fileName.substr(0,fileExtPos);
      tickerFolderName = ticker;
      std::replace(tickerFolderName.begin(),tickerFolderName.end(),'.','_');

      std::string outputFolderName = reportFolder;
      outputFolderName.append(tickerFolderName);

      outputFolderPath = outputFolderName;
      if( gapFill ){

        if( !std::filesystem::is_directory(outputFolderPath) ){
          ++gapFillCount;
          std::cout << gapFillCount << '\t' << fileName << std::endl;          
        }else{
          validInput = false;
        }
      }

    }

    if(validInput){

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

        bool isMetricDataNull = calculateData["metric_data"].is_null();
        //
        // Generate and save the pdf
        //           && !isMetricDataNull 
        if(!isTickerProcessed 
          && tickerMetaData.companyName.length() > 0){

          std::vector< std::string > pathToPlots;
          for(auto &plotItemConfig : plotSetConfig){
            //std::filesystem::path outputPlotFilePath = outputFolderPath;                  
            PlottingFunctions::PlotSettings plotSettings;
            std::vector< std::vector< sciplot::Plot2D >> plots;

            updatePlotArray(
              plots,
              plotSettings,
              tickerMetaData,
              fundamentalData,
              historicalData,
              calculateData,
              plotItemConfig,
              keywords,
              replacements,
              verbose);

            std::string title;
            JsonFunctions::getJsonString( plotItemConfig["settings"]["title"],
                                          title);
            findReplaceKeywords(title, keywords,replacements);                                        

            std::string plotFileName;
            JsonFunctions::getJsonString( plotItemConfig["settings"]["fileName"],
                                          plotFileName);                                        
            findReplaceKeywords(plotFileName, keywords,replacements); 

            std::filesystem::path outputPlotFilePath(outputFolderPath);          
            outputPlotFilePath.append(plotFileName);

            PlottingFunctions::writePlot(
                      plots,
                      plotSettings,
                      title,
                      outputPlotFilePath.c_str()); 

            pathToPlots.push_back(outputPlotFilePath);                               
          }
          /*
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

          PlottingFunctions::writePlot(
                    summaryPlots,
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

          PlottingFunctions::writePlot(
                    overviewPlots,
                    plotSettingsOverview,
                    titleOverview,
                    outputOverviewPlotFilePath.c_str());
          */

          std::filesystem::path outputReportFilePath = outputFolderPath;
          std::string reportFileName(tickerFolderName);
          reportFileName.append(".tex");
          outputReportFilePath.append(reportFileName);



          bool successGenerateLaTeXReport = 
            generateLaTeXReport(
              tickerMetaData,
              fundamentalData,
              historicalData,
              calculateData,
              pathToPlots,
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
    if(gapFill){
      std::cout << std::endl;
      std::cout << "Summary" << std::endl;
      std::cout << '\t' << gapFillCount 
                << '\t' << "number of ticker gaps filled" << std::endl;
      if(gapFillCount > 0){
        std::cout << '\t' << validFileCount
                  << '\t' << "number of tickers during gap filling"
                             " with valid data" << std::endl;
      }
    }else{
      std::cout << std::endl;
      std::cout << "Summary" << std::endl;
      std::cout << '\t' << totalFileCount 
                << '\t' << "number of tickers processed" << std::endl;
      std::cout << '\t' << validFileCount
                << '\t' << "number of tickers with valid data" << std::endl;
    }
  } 





  return 0;
}
