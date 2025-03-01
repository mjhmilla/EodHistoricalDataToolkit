//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef JSON_FUNCTIONS
#define JSON_FUNCTIONS

#include <string>
#include <nlohmann/json.hpp>
#include <stdlib.h>
#include <numeric>

//#include <chrono>
#include "date.h"
#include "DateFunctions.h"

class JsonFunctions {

  public:

    //When an accounting value is missing this number is inserted to 
    //signal that it is missing. This value has been chosen:
    //
    //1. To be less than 1 dollar/Eur/Yen etc. Why? Fractions of a dollar/Eur/Yen
    //   never (?) in financial statements. Seeing this show up in a table later 
    //   will cause the reader to wonder about it. This also means that this value
    //   can be used as a signal for a missing value later.
    //
    //2. It rounds to zero. 
    //
    static constexpr double MISSING_VALUE = 0.000001;

//==============================================================================
//From:
//https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes
    template <typename T>
    static std::vector< size_t > sort_indices(std::vector<T> &v){
      std::vector< size_t > idx(v.size());
      std::iota(idx.begin(),idx.end(),0);
      std::stable_sort(idx.begin(),idx.end(),
                        [&v](size_t i1, size_t i2){return v[i1]<v[i2];});
      std::stable_sort(v.begin(),v.end());
      return idx;
    }


//==============================================================================
    static bool loadJsonFile(const std::string &fileName, 
                             const std::string &folder,
                             nlohmann::ordered_json &jsonData,
                             bool verbose){
       std::stringstream ss;
       ss << folder << fileName;                              
       return loadJsonFile(ss.str(),jsonData,verbose);

    }

//==============================================================================
    static bool loadJsonFile(const std::string &fullFilePath,
                             nlohmann::ordered_json &jsonData,
                             bool verbose){

      bool success=true;
      std::string filePath = fullFilePath;
      try{
        //Load the json file                           
        if(filePath.length() < 5){
          filePath.append(".json");
        }else if(filePath.substr(filePath.length()-5,5).compare(".json") != 0){
          filePath.append(".json");
        }
        std::ifstream inputJsonFileStream(filePath.c_str());
        jsonData = nlohmann::ordered_json::parse(inputJsonFileStream);

        if(jsonData.empty()){
          success=false;
        }

      }catch(const nlohmann::json::parse_error& e){
        std::cout << e.what() << std::endl;
        if(verbose){
          std::cout << "  Skipping: failed while reading json file" << std::endl; 
        }
        success=false;
      }  

      return success;
    };

//==============================================================================
    static bool isJsonFloatValid(double value){
      if(std::isnan(value) || std::isinf(value)){
        return false;
      }else if(std::abs(value-MISSING_VALUE) < 
                std::numeric_limits< double >::epsilon()*10.0){
        return false;
      }else{
        return true;
      }
    }


//==============================================================================
    static bool doesFieldExist(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields){
      bool fieldExists = true;                                
      for(size_t i=0; i<fields.size();++i){
        switch(i){
          case 0:{
            fieldExists = jsonTable.contains(fields[0]);
          }break;
          case 1:{
            if(fieldExists){
              fieldExists = jsonTable[fields[0]].contains(fields[1]);
            }
          }break;          
          case 2:{
            if(fieldExists){
              fieldExists = jsonTable[fields[0]][fields[1]].contains(fields[2]);
            }
          }break;          
          case 3:{
            if(fieldExists){
              fieldExists = jsonTable[fields[0]][fields[1]]
                                                [fields[2]].contains(fields[3]);
            }
          }break;          
          case 4:{
            if(fieldExists){
              fieldExists = jsonTable[fields[0]][fields[1]]
                                     [fields[2]][fields[3]].contains(fields[4]);
            }
          }break;        
          default:
            fieldExists=false;  
        }
      }
      return fieldExists;
    };


//==============================================================================
    static bool getJsonBool(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields,
                               bool setNansToMissingValue=false){

      bool value = 0;
    
      switch( fields.size() ){
        case 1:{
            value = getJsonBool(
                      jsonTable[fields[0]],
                      setNansToMissingValue);
          }
          break;
        case 2:{
            value = getJsonBool(
                      jsonTable[fields[0]][fields[1]],
                      setNansToMissingValue);
          }
          break;
        case 3:{
            value = getJsonBool(
                      jsonTable[fields[0]][fields[1]][fields[2]],
                      setNansToMissingValue);
          }
          break;
        case 4:{
            value = getJsonBool(
                      jsonTable[fields[0]][fields[1]][fields[2]][fields[3]],
                      setNansToMissingValue);
          }
          break;
        default: {
          if(setNansToMissingValue){
            value = false;
          }else{
            value = std::nan("1");
          }          
        }
      };            

      return value;
    };


//==============================================================================
    static double getJsonFloat(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields,
                               bool setNansToMissingValue=false){

      double value = 0;
    
      switch( fields.size() ){
        case 1:{
            bool valid = true;
            value = getJsonFloat(
                      jsonTable[fields[0]],
                      setNansToMissingValue);
          }
          break;
        case 2:{
            value = getJsonFloat(
                      jsonTable[fields[0]][fields[1]],
                      setNansToMissingValue);
          }
          break;
        case 3:{
            value = getJsonFloat(
                      jsonTable[fields[0]][fields[1]][fields[2]],
                      setNansToMissingValue);
          }
          break;
        case 4:{
            value = getJsonFloat(
                      jsonTable[fields[0]][fields[1]][fields[2]][fields[3]],
                      setNansToMissingValue);
          }
          break;
        default: {
          if(setNansToMissingValue){
            value = MISSING_VALUE;
          }else{
            value = std::nan("1");
          }          
        }
      };            

      return value;
    };


//==============================================================================
    static void getJsonString(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields,
                               std::string &stringUpd){
    
      switch( fields.size() ){
        case 1:{
            getJsonString(  jsonTable[fields[0]],
                            stringUpd);
          }
          break;
        case 2:{
            getJsonString(  jsonTable[fields[0]][fields[1]],
                            stringUpd);
          }
          break;
        case 3:{
            getJsonString(  jsonTable[fields[0]][fields[1]][fields[2]],
                            stringUpd);
          }
          break;
        case 4:{
            getJsonString(  jsonTable[fields[0]][fields[1]][fields[2]][fields[3]],
                            stringUpd);
          }
          break;
        default: {
          stringUpd = "";          
        }
      };            
    };    

//==============================================================================
    static bool getJsonElement(
                    const nlohmann::ordered_json &jsonTable,
                    const std::vector< std::string > &fields,
                    nlohmann::ordered_json &jsonElement){
    
      bool success = false;
      switch( fields.size() ){
        case 1:{
            jsonElement = jsonTable[fields[0]];
            success=true;
          }
          break;
        case 2:{
            jsonElement = jsonTable[fields[0]][fields[1]];
            success=true;
          }
          break;
        case 3:{
            jsonElement = jsonTable[fields[0]][fields[1]][fields[2]];
            success=true;
          }
          break;
        case 4:{
            jsonElement = jsonTable[fields[0]][fields[1]][fields[2]][fields[3]];
            success=true;
          }
          break;
        default: {
          success=false;          
        }
      };
      return success;            
    };

//==============================================================================
    static void getJsonFloatArray(const nlohmann::ordered_json &jsonEntry,
                               std::vector< double > &data){
      if(  jsonEntry.is_null()){
        data.clear();
      }else{
          data = jsonEntry.get<std::vector<double> >();
      }

    };
//==============================================================================
    static double getJsonFloat(const nlohmann::ordered_json &jsonEntry,
                               bool setNansToMissingValue=false){
      if(  jsonEntry.is_null()){
        if(setNansToMissingValue){
          return MISSING_VALUE;
        }else{
          return std::nan("1");
        }
      }else{
        if(jsonEntry.is_number_unsigned()){
          return jsonEntry.get<double>();
        }else if(jsonEntry.is_number_integer()){
          return jsonEntry.get<double>();
        }else if( jsonEntry.is_number_float()){
          return jsonEntry.get<double>();
        }else if (jsonEntry.is_string()){
          return std::atof(jsonEntry.get<std::string>().c_str());
        }else{
          throw std::invalid_argument("json entry is not a float or string");      
        }
      }

    };

//==============================================================================
    static bool getJsonBool(const nlohmann::ordered_json &jsonEntry,
                            bool replaceNanWithFalse=false){
      if(  jsonEntry.is_null()){
        if(replaceNanWithFalse){
          return false;
        }else{
          return std::nan("1");
        }
      }else{
        if(  jsonEntry.is_boolean()){
          return jsonEntry.get<bool>();
        }else{
          throw std::invalid_argument("json entry is not a boolean");      
        }
      }
    };

//==============================================================================
    static void getJsonString(const nlohmann::ordered_json &jsonEntry,
                              std::string &updString){
      if( jsonEntry.is_null()){
        updString="";
      }else{
        updString=jsonEntry.get<std::string>();
      }                            
    };

//==============================================================================
    static void getPrimaryTickerName(const std::string &folder, 
                              const std::string &fileName, 
                              std::string &updPrimaryTickerName){

      //Create the path and file name                          
      std::stringstream ss;
      ss << folder << fileName;
      std::string filePathName = ss.str();
      
      using json = nlohmann::ordered_json;
      std::ifstream jsonFileStream(filePathName.c_str());
      json jsonData = json::parse(jsonFileStream);  

      if( jsonData.contains("General") ){
        if(jsonData["General"].contains("PrimaryTicker")){
          if(jsonData["General"]["PrimaryTicker"].is_null() == false){
            updPrimaryTickerName = 
              jsonData["General"]["PrimaryTicker"].get<std::string>();
          }
        }
      }
    };

//==============================================================================
    static int findClosestDate(const nlohmann::ordered_json &jsonTable,
                               const date::sys_days &targetDay,
                               const char* dateFormat,
                               std::string &dateClosestUpd,
                               date::sys_days &dayClosestUpd,
                               bool acceptDatesAfterTargetDay=false)
    {

        int smallestDayError=std::numeric_limits<int>::max();
        for(auto &metricItem : jsonTable.items()){
          std::string itemDate(metricItem.key());
          std::istringstream itemDateStream(itemDate);
          itemDateStream.exceptions(std::ios::failbit);
          date::sys_days itemDays;
          itemDateStream >> date::parse(dateFormat,itemDays);


          int dayError = std::abs((targetDay-itemDays).count());
          if(acceptDatesAfterTargetDay){
            dayError = std::abs(dayError);
          }
          if(dayError < smallestDayError && dayError > 0){
            smallestDayError  = dayError;
            dateClosestUpd    = itemDate;
            dayClosestUpd     = itemDays;
          }            
        }
        return smallestDayError;
    };

//==============================================================================
    static void extractDataSeries(
          const nlohmann::ordered_json &jsonData,
          const std::vector<std::string> &addressToTimeSeries,
          const char* dateFieldName,
          const char* floatFieldName,
          bool isArray,
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
        if(isArray){
          JsonFunctions::getJsonFloatArray(jsonElement[dateFieldName],
                                           dateSeries);
          JsonFunctions::getJsonFloatArray(jsonElement[floatFieldName],
                                           floatSeries);
        }else{
          for(auto &el: jsonElement.items()){

            double floatData = 
              JsonFunctions::getJsonFloat(el.value()[floatFieldName]);

            if(JsonFunctions::isJsonFloatValid(floatData)){
          
              tmpFloatSeries.push_back(floatData);

              std::string dateEntryStr;
              JsonFunctions::getJsonString(el.value()[dateFieldName],
                              dateEntryStr); 

              double timeData = 
                DateFunctions::convertToFractionalYear(dateEntryStr);
              dateSeries.push_back(timeData);
            }
          
          }

          std::vector< size_t > indicesSorted = sort_indices(dateSeries);
          for(size_t i=0; i<indicesSorted.size();++i){
            floatSeries.push_back( tmpFloatSeries[ indicesSorted[i] ] );
          }
        }
      }

    };    

};


#endif