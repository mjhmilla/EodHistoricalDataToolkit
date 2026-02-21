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

    enum StringConversion{
      StringToFloat = 0,
      StringToNumericalDate,
      STRING_CONVERSION
    };

    struct StringConversionSettings{
      StringConversion typeOfConversion;
      const char* dateFormat;
      StringConversionSettings(StringConversion type, const char* format):
        typeOfConversion(type),
        dateFormat(format){};
    };
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
    static nlohmann::ordered_json* getTableReference(
                              const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields){

      switch( fields.size() ){
        case 1:{
            return const_cast<nlohmann::ordered_json*>(&jsonTable[fields[0]]);
          }
          break;
        case 2:{
          return const_cast<nlohmann::ordered_json*>(
                &jsonTable[fields[0]][fields[1]]);
          }
          break;
        case 3:{
          return const_cast<nlohmann::ordered_json*>(
                &jsonTable[fields[0]][fields[1]][fields[2]]);          
          }
          break;
        case 4:{
          return const_cast<nlohmann::ordered_json*>(
                &jsonTable[fields[0]][fields[1]][fields[2]][fields[3]]);
          }
          break;
        default: {
          return NULL;  
        }
      };         

    };

//==============================================================================
    static bool doesFieldExist(const nlohmann::ordered_json &jsonTable,
                               const std::vector< std::string > &fields){
      bool fieldExists = false;
      if(fields.size()>0){   

        fieldExists = true;

        for(size_t i=0; i<fields.size();++i){

          if(fieldExists){            
            //Check each parent until the final field is reached
            //Why? If one of the children (e.g. fields[1]) doesn't exist 
            //nlohmann::json will throw an exception 
            //(e.g. jsonTable[fields[0]][fields[1]].contains ..)
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
        }
      }
      return fieldExists;
    };


//==============================================================================
    static bool getJsonBool(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields){

      bool value = false;

      bool fieldsExist = doesFieldExist(jsonTable,fields);
      

      if(fieldsExist){
        switch( fields.size() ){
          case 1:{
                value = getJsonBool(
                          jsonTable[fields[0]]);
            }
            break;
          case 2:{

              value = getJsonBool(
                        jsonTable[fields[0]][fields[1]]);
            }
            break;
          case 3:{
              value = getJsonBool(
                        jsonTable[fields[0]][fields[1]][fields[2]]);
            }
            break;
          case 4:{
              value = getJsonBool(
                        jsonTable[fields[0]][fields[1]][fields[2]][fields[3]]);
            }
            break;
          default: {
            value=false;
          }
        };            
      }
      return value;
    };


//==============================================================================
    static double getJsonFloat(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields,
                               bool setNansToMissingValue=false){

      double value = std::nan("1");
      if(setNansToMissingValue){
        value = MISSING_VALUE;
      }

      bool fieldsExist = doesFieldExist(jsonTable,fields);
      

      if(fieldsExist){
        switch( fields.size() ){
          case 1:{
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
      }
      return value;
    };


//==============================================================================
    static void getJsonString(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields,
                               std::string &stringUpd){
    
      bool fieldsExist = doesFieldExist(jsonTable,fields);
      if(fieldsExist){
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
      }           
    };    

//==============================================================================
    static bool getJsonElement(
                    const nlohmann::ordered_json &jsonTable,
                    const std::vector< std::string > &fields,
                    nlohmann::ordered_json &jsonElement){
    
      
      bool validElement = doesFieldExist(jsonTable,fields);

      if(validElement){
        switch( fields.size() ){
          case 1:{
              jsonElement = jsonTable[fields[0]];
          } break;
          case 2:{
              jsonElement = jsonTable[fields[0]][fields[1]];
          } break;
          case 3:{
              jsonElement = jsonTable[fields[0]][fields[1]][fields[2]];
          } break;
          case 4:{
            jsonElement = 
              jsonTable[fields[0]][fields[1]][fields[2]][fields[3]];
          }break;
          default: {
            validElement=false;          
          }
        };
      }



      return validElement;            
    };

//==============================================================================
    static void getJsonStringArray(const nlohmann::ordered_json &jsonEntry,
                               std::vector< std::string > &data){
      if(  jsonEntry.is_null()){
        data.clear();
      }else{
          data = jsonEntry.get<std::vector<std::string> >();
      }

    };    
//==============================================================================
    static void getJsonFloatArray(const nlohmann::ordered_json &jsonEntry,
                               std::vector< double > &data){
      if(  jsonEntry.is_null()){
        data.clear();
      }else{
        try{
          data = jsonEntry.get<std::vector<double> >();
        }catch(nlohmann::json_abi_v3_12_0::detail::type_error &e){
          data.clear();
        }
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
    static bool getJsonBool(const nlohmann::ordered_json &jsonEntry){
      if(  jsonEntry.is_null()){
          return false;        
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
    static nlohmann::json::value_t getDataType(         
          const nlohmann::ordered_json &jsonData,
          const std::vector<std::string> &address,
          const char* fieldName,
          bool isArray)
    {
      nlohmann::json::value_t dataType = nlohmann::json::value_t::null; 

      nlohmann::ordered_json jsonElement;

      bool isElementValid = false;

      if(address.size()>0){
        isElementValid = JsonFunctions::getJsonElement(
                            jsonData, address,jsonElement);
      }else{
        jsonElement = jsonData;
        if(jsonElement.size()>0){
          isElementValid=true;
        }
      }      
      if(isArray){
        dataType = jsonElement[fieldName].type();
      }else{
        for(auto &el: jsonElement.items()){
          dataType = el.value()[fieldName].type();
          if(dataType != nlohmann::json::value_t::null){
            break;
          }
        }          
      }
      return dataType;

    };
//==============================================================================
    static void extractStringConvertToFloatSeries(
          const nlohmann::ordered_json &jsonData,
          const std::vector<std::string> &addressToData,
          const char* fieldName,
          const StringConversionSettings& settings,
          std::vector<double> &dataSeries)
    {
      dataSeries.clear();

      nlohmann::ordered_json jsonElement;

      bool isElementValid = false;

      if(addressToData.size()>0){
        isElementValid = JsonFunctions::getJsonElement(
                            jsonData, addressToData,jsonElement);
      }else{
        jsonElement = jsonData;
        if(jsonElement.size()>0){
          isElementValid=true;
        }
      }
            
      if(isElementValid){
          int count = 0;
          for(auto &el: jsonElement.items()){
            std::string stringData;
            JsonFunctions::getJsonString(el.value()[fieldName],stringData);

            double value = std::nan("1");
            switch(settings.typeOfConversion){
              case StringConversion::StringToFloat:{
                value = std::nan("1");
                if(stringData.size()>0){
                  value = std::stod(stringData);
                }
              }; break;
              case StringConversion::StringToNumericalDate:{
                value = DateFunctions::convertToFractionalYear(stringData,
                                            settings.dateFormat);
              }; break;              
              default:{
                std::cout << "Error: unrecognized value in "
                          << "settings.typeOfConversion"
                          << std::endl;
                std::abort();
              }
            };
            dataSeries.push_back(value);
          }          
      }
    };

    
//==============================================================================
    static void extractStringSeries(
          const nlohmann::ordered_json &jsonData,
          const std::vector<std::string> &addressToData,
          const char* fieldName,
          bool isArray,
          std::vector<std::string> &stringSeries,
          int numberOfItems = -1)
    {
      stringSeries.clear();

      nlohmann::ordered_json jsonElement;

      bool isElementValid = false;

      if(addressToData.size()>0){
        isElementValid = JsonFunctions::getJsonElement(
                            jsonData, addressToData,jsonElement);
      }else{
        jsonElement = jsonData;
        if(jsonElement.size()>0){
          isElementValid=true;
        }
      }
            
      if(isElementValid){
        if(isArray){
          JsonFunctions::getJsonStringArray(jsonElement[fieldName],
                                           stringSeries);
        }else{
          int count = 0;
          for(auto &el: jsonElement.items()){
            std::string stringData;
            JsonFunctions::getJsonString(el.value()[fieldName],stringData);
            stringSeries.push_back(stringData);            
            ++count;
            if(count > numberOfItems && numberOfItems != -1){
              break;
            }
          }          
        }
      }
    };

//==============================================================================
    static void extractFloatSeries(
          const nlohmann::ordered_json &jsonData,
          const std::vector<std::string> &addressToData,
          const char* fieldName,
          bool isArray,
          std::vector<double> &dataSeries)
    {
      dataSeries.clear();

      nlohmann::ordered_json jsonElement;

      bool isElementValid = false;

      if(addressToData.size()>0){
        isElementValid = JsonFunctions::getJsonElement(
                            jsonData, addressToData,jsonElement);
      }else{
        jsonElement = jsonData;
        if(jsonElement.size()>0){
          isElementValid=true;
        }
      }
            
      if(isElementValid){
        if(isArray){
          JsonFunctions::getJsonFloatArray(jsonElement[fieldName],
                                           dataSeries);
        }else{
          for(auto &el: jsonElement.items()){
            double numData = JsonFunctions::getJsonFloat(el.value()[fieldName]);
            //if(JsonFunctions::isJsonFloatValid(numData)){
            dataSeries.push_back(numData);
            //}
          }          
        }
      }

    };

    


};


#endif