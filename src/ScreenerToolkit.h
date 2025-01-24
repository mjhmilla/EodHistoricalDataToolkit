//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef SCREENER_TOOLKIT
#define SCREENER_TOOLKIT

#include <cmath>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <sstream>

#include "date.h"
#include <nlohmann/json.hpp>
#include "JsonFunctions.h"


class ScreenerToolkit {

  public:

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
    
    //==========================================================================
    // Fun example to get the ranked index of an entry
    // https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes

    template <typename T>
    static std::vector<size_t> calcSortedVectorIndicies(
                                  const std::vector<T> &v, 
                                  bool sortAscending){

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

    //==========================================================================
    template <typename T>
    static std::vector< size_t > getRank(
                                  const std::vector<T> &indiciesOfSortedVector){

      std::vector<size_t> rank(indiciesOfSortedVector.size());
      for(size_t i = 0; i < indiciesOfSortedVector.size(); ++i){
        rank[indiciesOfSortedVector[i]] = i;
      }     
      return rank;                           

    };

    //==========================================================================
    static void rankMetricData(
          const nlohmann::ordered_json &screenReportConfig, 
          MetricSummaryDataSet &metricDataSetUpd,             
          bool verbose)
    {

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

      for( auto const &rankingItem : screenReportConfig["ranking"].items()){
        //Retrieve the metric data
        std::string metricName(rankingItem.key());

        double lowerBound = 
          JsonFunctions::getJsonFloat(rankingItem.value()["lowerBound"]);

        double upperBound = 
          JsonFunctions::getJsonFloat(rankingItem.value()["upperBound"]);

        std::string direction;
        JsonFunctions::getJsonString(rankingItem.value()["direction"],direction);


        int typeOfMeasure = -1;

        if(!rankingItem.value()["measure"].is_null()){
          std::string typeOfMeasureName;
          JsonFunctions::getJsonString(rankingItem.value()["measure"],typeOfMeasureName);

          if(typeOfMeasureName == "recent"){
            typeOfMeasure = -1;
          }else if(typeOfMeasureName == "p05"){
            typeOfMeasure = 0;
          }else if(typeOfMeasureName == "p25"){
            typeOfMeasure = 1;
          }else if(typeOfMeasureName == "p50"){
            typeOfMeasure = 2;
          }else if(typeOfMeasureName == "p75"){
            typeOfMeasure = 3;
          }else if(typeOfMeasureName == "p95"){
            typeOfMeasure = 4;
          }else{
            std::cout << "Error: value field must be recent,p05,p25,p50,p75, or p95 "
                      << "instead this was entered: " << typeOfMeasureName;
            std::abort();                
          }
        }


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
          
          double value = std::nan("1");
          
          if(metricDataSetUpd.summaryStatistics[indexRow][indexMetric].percentiles.size()>0){

            switch(typeOfMeasure){
              case -1:
              {
                value=metricDataSetUpd.metric[indexRow][indexMetric];
              }
              break;
              case 0:
              {
                value = metricDataSetUpd
                        .summaryStatistics[indexRow][indexMetric].percentiles[0];
              }
              break;
              case 1:
              {
                value = metricDataSetUpd
                        .summaryStatistics[indexRow][indexMetric].percentiles[1];
              }
              break;
              case 2:
              {
                value = metricDataSetUpd
                        .summaryStatistics[indexRow][indexMetric].percentiles[2];
              }
              break;
              case 3:
              {
                value = metricDataSetUpd
                        .summaryStatistics[indexRow][indexMetric].percentiles[3];
              }
              break;
              case 4:
              {
                value = metricDataSetUpd
                        .summaryStatistics[indexRow][indexMetric].percentiles[4];
              }
              break;
              default:{
                std::cout << "Error: value field must be "
                          << "recent,p05,p25,p50,p75, or p95."
                          << std::endl;
                std::abort();  
              };
            }
          }
          
          if(!std::isnan(lowerBound)){
            if(value < lowerBound){
              value = std::nan("1");
            }
          }
          if(!std::isnan(upperBound)){
            if(value > upperBound){
              value = std::nan("1");
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

        std::vector< size_t > columnRank = 
          getRank(indicesOfSortedVector);

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
        calcSortedVectorIndicies(
          metricDataSetUpd.metricRankSum, true);

      metricDataSetUpd.rank = 
        getRank(metricDataSetUpd.sortedIndex);    


      bool here=true;

    };


    //==========================================================================
    static bool appendMetricData(
            const std::string &tickerFileName,  
            const std::string &fundamentalFolder,                 
            const std::string &historicalFolder,
            const std::string &calculateDataFolder,
            const nlohmann::ordered_json &screenReportConfig, 
            const date::year_month_day &targetDate,
            int maxTargetDateErrorInDays,
            MetricSummaryDataSet &metricDataSetUpd,             
            bool verbose){


      bool inputsAreValid=true;

      bool rankingFieldExists = screenReportConfig.contains("ranking");

      bool weightFieldExists = screenReportConfig.contains("weighting");
      

      if(rankingFieldExists){

        std::vector< double > metricVector;
        std::vector< date::sys_days > dateVector;
        std::vector< PlottingFunctions::SummaryStatistics> percentileVector;
        double weight=1.0;

        nlohmann::ordered_json targetJsonTable;
        bool loadedCalculateData = false;

        for( auto const &rankingItem : screenReportConfig["ranking"].items()){
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


            //Extract the metric data closest to the target date
            std::string closestDate;
            date::sys_days closestDay;
            double targetMetricValue = 0.;

            int smallestDayError = 
              JsonFunctions::findClosestDate( targetJsonTable,
                                              targetDate,
                                              "%Y-%m-%d",
                                              closestDate,
                                              closestDay);

            double metricValue = 
              JsonFunctions::getJsonFloat(targetJsonTable[closestDate],fieldVector);
            bool metricValueValid = JsonFunctions::isJsonFloatValid(metricValue);

            if(metricValueValid){
              targetMetricValue = metricValue;         
            }

            //Build the historical list
            std::vector< double > metricData;
            for(auto &metricItem : targetJsonTable.items()){
              std::string itemDate(metricItem.key());

              double metricValue = 
                JsonFunctions::getJsonFloat(targetJsonTable[itemDate],fieldVector);
              bool metricValueValid = JsonFunctions::isJsonFloatValid(metricValue);

              if(metricValueValid){
                metricData.push_back(metricValue);          
              }
            }

            if(weightFieldExists && closestDate.length() > 0){
              std::string folderWeighting; 
              JsonFunctions::getJsonString( 
                screenReportConfig["weighting"]["folder"],folderWeighting);

              if( (folderWeighting.compare("calculateData") != 0) ){
                std::cout << "Error: weighting:folder in the "
                          << "screen configuration  file should be calculateData "
                          << " but is instead " << folderWeighting
                          << std::endl;
              }

              std::vector< std::string > fieldAddress;
              for(auto &el : screenReportConfig["weighting"]["field"]){
                fieldAddress.push_back(el);
              }

              weight = JsonFunctions::getJsonFloat(
                            targetJsonTable[closestDate],fieldAddress);

            }
                                    
            PlottingFunctions::SummaryStatistics percentileSummary;
            PlottingFunctions::extractSummaryStatistics(metricData,
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
    };

    //==========================================================================
    static bool applyFilter(  const std::string &tickerFileName,
                              const nlohmann::ordered_json &fundamentalData,
                              const nlohmann::ordered_json &historicalData,
                              const nlohmann::ordered_json &calculateData,    
                              const std::string &tickerReportFolder,
                              const nlohmann::ordered_json &screenReportConfig,
                              const date::year_month_day &targetDate,
                              int maxTargetDateErrorInDays,                 
                              bool replaceNansWithMissingData,
                              bool verbose){

      bool valueFilter = true;

      bool filterFieldExists = screenReportConfig.contains("filter");
      

      if(filterFieldExists){

        for( auto const &el : screenReportConfig["filter"].items() ){
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
          nlohmann::ordered_json *targetJsonTable;

          std::string fullPathFileName(""); 
          bool jsonTableItemizedByDate=false;     
          if(folder == "fundamentalData"){
            targetJsonTable =
              const_cast<nlohmann::ordered_json*>(&fundamentalData);
            jsonTableItemizedByDate=false;                
          }else if(folder == "historicalData"){
            targetJsonTable = 
              const_cast<nlohmann::ordered_json*>(&historicalData);
            jsonTableItemizedByDate=true;                
          }else if(folder == "calculateData"){
            targetJsonTable = 
              const_cast<nlohmann::ordered_json*>(&calculateData);
            jsonTableItemizedByDate=true;                
          }else{
            std::cerr << "Error: in filter " << filterName 
                      << " the folder field is " << folder 
                      << " but should be fundamentalData, historicalData,"
                      << " or calculateData" 
                      << std::endl;
            std::abort();                  
          }







          //Go fetch the most recent date and prepend it to the field vector
          if(jsonTableItemizedByDate){
            std::string closestDate;
            date::sys_days closestDay;
            int smallestDayError = 
              JsonFunctions::findClosestDate( *targetJsonTable,
                                              targetDate,
                                              "%Y-%m-%d",
                                              closestDate,
                                              closestDay);

            if(closestDate.length()==0){
              valueFilter=false;
              break;
            }else{
              fieldVector.insert(fieldVector.begin(),closestDate);
            }                                         
            
          }

          std::string valueString;
          double valueFloat;
          std::vector < bool > valueBoolVector;

          if(valueType == "string"){
            JsonFunctions::getJsonString( *targetJsonTable, 
                                          fieldVector, 
                                          valueString);

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
            valueFloat = JsonFunctions::getJsonFloat(*targetJsonTable, 
                                                  fieldVector,
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

          bool valueFilterSingle = valueBoolVector[0];      
          if(valueBoolVector.size()>1){

            for(size_t i = 1; i < valueBoolVector.size(); ++i){
              if(operatorName == "||"){
                valueFilterSingle = (valueFilterSingle || valueBoolVector[i]);
              }else if( operatorName == "&&"){
                valueFilterSingle = (valueFilterSingle && valueBoolVector[i]);
              }else{
                std::cerr << "Error: in filter " << filterName 
                      << " the operator field should be " 
                      << " || or && for a string"
                      << " but is instead " << operatorName  
                      << std::endl;
                std::abort();             
              }
            }
          }else{
           valueFilterSingle = valueBoolVector[0];
          }
          valueFilter = (valueFilter && valueFilterSingle); 
        }

      }

      //Check to see if the ticker report file exists
      if(valueFilter){
        std::string tickerReportPath    = tickerReportFolder;
        std::string tickerFolder        = tickerFileName;
        std::string tickerLaTeXFileName = tickerFileName;

        ReportingFunctions::sanitizeFolderName(tickerFolder,true);
        ReportingFunctions::sanitizeFolderName(tickerLaTeXFileName,true);
        tickerLaTeXFileName.append(".tex");

        std::filesystem::path tickerLaTeXReportPath=tickerReportFolder;
        tickerLaTeXReportPath /= tickerFolder;
        tickerLaTeXReportPath /= tickerLaTeXFileName;

        if( !std::filesystem::exists(tickerLaTeXReportPath)  ){
          valueFilter = false;
        }
      }  

      return valueFilter;
    };    
};

#endif