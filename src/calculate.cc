//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <limits>

#include "date.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"


//============================================================================
struct TaxFoundationDataSet{
  std::vector< int > index;
  std::vector< int > year;
  std::vector< std::string > CountryISO2;
  std::vector< std::string > CountryISO3;
  std::vector< std::string > continent;
  std::vector< std::string > country;
  std::vector< std::vector < double > > taxTable; 
};
//============================================================================

struct AnalysisDates{
  std::vector< std::string > common;
  std::vector< std::string > financial;
  std::vector< std::string > outstandingShares;
  std::vector< std::string > historical;
  std::vector< std::string > bond;

  std::vector< unsigned int > indicesFinancial;
  std::vector< unsigned int > indicesOutstandingShares;
  std::vector< unsigned int > indicesHistorical;
  std::vector< unsigned int > indicesBond;
};


//============================================================================
bool extractTTM(int indexA,
                const std::vector<std::string> &dateSet,
                const char* dateFormat, 
                std::vector<std::string> &dateSetTTMUpd,
                std::vector<double> &weightTTMUpd,
                int maximumTTMDateSetErrorInDays){

  dateSetTTMUpd.clear();
  //weightingTTMUpd.clear();

  int indexB = indexA;

  std::istringstream dateStream(dateSet[indexA]);
  dateStream.exceptions(std::ios::failbit);
  date::sys_days daysA;
  dateStream >> date::parse(dateFormat,daysA);


  int indexPrevious = indexA;
  date::sys_days daysPrevious = daysA;
  int count = 0;
  bool flagDateSetFilled = false;

  int daysInAYear = 365;
  int countError = maximumTTMDateSetErrorInDays*2.0;

  while((indexB+1) < dateSet.size() 
          && std::abs(countError) > maximumTTMDateSetErrorInDays){
    ++indexB;

    dateStream.clear();
    dateStream.str(dateSet[indexB]);
    dateStream.exceptions(std::ios::failbit);
    date::sys_days daysB;
    dateStream >> date::parse(dateFormat,daysB);

    int daysInterval  = (daysPrevious-daysB).count();    
    countError        = daysInAYear - (count+daysInterval);
    count             += daysInterval;

    dateSetTTMUpd.push_back(dateSet[indexPrevious]);
    weightTTMUpd.push_back(static_cast<double>(daysInterval));
    
    daysPrevious = daysB;
    indexPrevious=indexB;

  }

  for(size_t i =0; i<weightTTMUpd.size(); ++i){
    weightTTMUpd[i] = weightTTMUpd[i] / static_cast<double>(count);
  }


  if( std::abs(daysInAYear-count) <= maximumTTMDateSetErrorInDays){
    return true;
  }else{
    return false;
  }

};


//============================================================================
bool extractDatesOfClosestMatch(
              std::vector< std::string > &datesSetA,
              const char* dateAFormat,
              std::vector< std::string > &datesSetB,
              const char* dateBFormat,
              unsigned int maxNumberOfDaysInError,
              std::vector< std::string > &datesCommonToAandB,
              std::vector< unsigned int > &indicesOfCommonSetADates,
              std::vector< unsigned int > &indicesOfCommonSetBDates,
              bool allowRepeatedDates ){

    
  indicesOfCommonSetADates.clear();
  indicesOfCommonSetBDates.clear();
  datesCommonToAandB.clear();

  bool validInput=true;

  //Get the indicies of the most recent and oldest indexes "%Y-%m-%d"
  int firstMinusLastSetA = 
    FinancialAnalysisToolkit::calcDifferenceInDaysBetweenTwoDates(
                                datesSetA[0],
                                dateAFormat,        
                                datesSetA[datesSetA.size()-1],
                                dateAFormat);


  //Get the indicies of the most recent and oldest indexes
  int firstMinusLastSetB = 
    FinancialAnalysisToolkit::calcDifferenceInDaysBetweenTwoDates(
                                datesSetB[0],
                                dateBFormat,        
                                datesSetB[datesSetB.size()-1],
                                dateBFormat);

  //The indices stored to match the order of datesSetA
  int indexSetBFirst = 0;
  int indexSetBLast = 0;
  int indexSetBDelta = 0;

  if(firstMinusLastSetB*firstMinusLastSetA > 0){
    indexSetBFirst = 0;
    indexSetBLast = static_cast<int>(datesSetB.size())-1;
    indexSetBDelta = 1;
  }else{
    indexSetBFirst = static_cast<int>(datesSetB.size())-1;
    indexSetBLast = 0;
    indexSetBDelta = -1;
  }


  int indexSetB    = indexSetBFirst;
  int indexSetBEnd = indexSetBLast+indexSetBDelta;

  std::string tempDateSetB("");
  std::string tempDateSetA("");

  for(unsigned int indexSetA=0; 
      indexSetA < datesSetA.size(); 
      ++indexSetA ){

    //Find the closest date in SetB data leading up to the fundamental
    //date     
    tempDateSetA = datesSetA[indexSetA];

    if(allowRepeatedDates){
      indexSetB = indexSetBFirst;
    }

    int daysInError;
    std::string lastValidDate;
    bool found=false;

    while( indexSetB != (indexSetBEnd) && !found){


      tempDateSetB = datesSetB[indexSetB];

      int daysSetALessSetB = 
        FinancialAnalysisToolkit::calcDifferenceInDaysBetweenTwoDates(
                                    tempDateSetA,
                                    dateAFormat,        
                                    tempDateSetB,
                                    dateBFormat);

      //As long as the historical date is less than or equal to the 
      //fundamental date, increment
      if(daysSetALessSetB >= 0){
        daysInError   = daysSetALessSetB;
        lastValidDate = tempDateSetB;
        found=true;
      }else{
        indexSetB += indexSetBDelta;
      }

    }

    //Go back to the last valid date and save its information
    if(indexSetB < datesSetB.size() && indexSetB >= 0 
        && daysInError <= maxNumberOfDaysInError){  
        indicesOfCommonSetADates.push_back(indexSetA);
        indicesOfCommonSetBDates.push_back(indexSetB);
        datesCommonToAandB.push_back(tempDateSetA);           
    }
    
  }

  if(datesCommonToAandB.size() == 0){
    validInput=false;
  }

  return validInput;
};
//============================================================================

bool extractAnalysisDates(
      AnalysisDates &analysisDates,
      const nlohmann::ordered_json &fundamentalData,
      const nlohmann::ordered_json &historicalData,
      const nlohmann::ordered_json &bondData,
      const std::string &timePeriod,
      const std::string &timePeriodOutstandingShares,
      int maxDayErrorHistoricalData,
      int maxDayErrorOutstandingShareData,
      int maxDayErrorBondData,
      bool allowRepeatedDates)
{
  bool validDates=true;

  //Clear the analysis structure
  analysisDates.common.clear();
  analysisDates.financial.clear();
  analysisDates.outstandingShares.clear();
  analysisDates.historical.clear();
  analysisDates.bond.clear();
  analysisDates.indicesOutstandingShares.clear();
  analysisDates.indicesHistorical.clear();
  analysisDates.indicesBond.clear();


  //Extract dates only the dates common to all financial reports
  std::vector< std::string > datesBAL;
  std::vector< std::string > datesCF;
  std::vector< std::string > datesIS;
  
  std::vector< std::string > finStruct;
  finStruct.push_back(BAL);
  finStruct.push_back(CF);
  finStruct.push_back(IS);

  //Check that all needed fields exist and have data
  
  if(fundamentalData.contains(FIN)){
    for(size_t i=0; i<finStruct.size(); ++i){
      if(fundamentalData[FIN][finStruct[i].c_str()].contains(timePeriod.c_str())){
        if(fundamentalData[FIN][finStruct[i].c_str()][timePeriod.c_str()].size()==0){
          validDates=false;
        }
      }else{
        validDates=false;
      }
    }  
  }else{
    validDates=false;
  }

  if(validDates){
    for(size_t i=0; i<finStruct.size(); ++i){
      for(auto& el : fundamentalData[FIN][finStruct[i].c_str()][timePeriod.c_str()].items()){
        std::string dateString; 
        JsonFunctions::getJsonString(el.value()["date"],dateString);
        switch(i){
          case 0:
            datesBAL.push_back(dateString);
          break;        
          case 1:
            datesCF.push_back(dateString);
          break;
          case 2:
            datesIS.push_back(dateString);
          break;
          default:
            std::abort();          
        };
      }      
    }

    for(size_t i=0; i<datesBAL.size();++i){
      bool common = true;
    
      size_t j=0;
      bool found =false;
      while(!found && j < datesCF.size()){
        if(datesBAL[i].compare(datesCF[j]) == 0){
          found=true;
        }else{
          ++j;
        }
      }
      if(j == datesCF.size()){
        common=false;
      }

      size_t k=0;
      found=false;
      while(!found && k < datesIS.size()){
        if(datesBAL[i].compare(datesIS[k]) == 0 ){
          found=true;
        }else{
          ++k;
        }
      }
      if(k == datesIS.size()){
        common=false;
      }

      if(common){
        analysisDates.financial.push_back(datesBAL[i]);
      }

    }
  }

  validDates = (validDates && analysisDates.financial.size() > 0);

  if(validDates){
    for(auto& el : fundamentalData[OS][timePeriodOutstandingShares.c_str()].items()){
      std::string dateFormatted; 
      JsonFunctions::getJsonString(el.value()["dateFormatted"],dateFormatted);
      analysisDates.outstandingShares.push_back(dateFormatted);
    }   
    validDates = (validDates && analysisDates.outstandingShares.size() > 0);
  }

  if(validDates){
    for(auto& el: historicalData.items()){
      std::string dateString("");
      JsonFunctions::getJsonString(el.value()["date"],dateString);
      analysisDates.historical.push_back(dateString);
    }  
    validDates = (validDates && analysisDates.historical.size() > 0);

    for(auto& el : bondData.items()){
      analysisDates.bond.push_back(el.key());
    }
    validDates = (validDates && analysisDates.bond.size() > 0);
  }

  
  if(validDates){

    //
    //Make sure the financial data proceed from most recent to oldest
    //
    int dateProgression=
      FinancialAnalysisToolkit::calcDifferenceInDaysBetweenTwoDates(
        analysisDates.financial[0],
        "%Y-%m-%d",
        analysisDates.financial[analysisDates.financial.size()-1],
        "%Y-%m-%d");
    if(dateProgression < 0){
      std::reverse( analysisDates.financial.begin(), 
                    analysisDates.financial.end());
    }


    //Extract common dates between
    // financial
    // historical
    validDates =     
      extractDatesOfClosestMatch(
        analysisDates.financial,
        "%Y-%m-%d",
        analysisDates.historical,
        "%Y-%m-%d",
        maxDayErrorHistoricalData,
        analysisDates.common,
        analysisDates.indicesFinancial,
        analysisDates.indicesHistorical,
        allowRepeatedDates);


    //Extract common dates between
    // financial
    // historical
    // outstandingShares
    //
    // Do this by getting common dates between
    // common
    // outstandingShares
    //
    // And removing missing dates in 
    // indicesFinancial 
    // indicesHistorical
    //
    // That are not a part of the new common
    if(validDates){
      std::vector< std::string > commonAB;
      std::vector< unsigned int> indicesA;

      validDates =     
        extractDatesOfClosestMatch(
          analysisDates.common,
          "%Y-%m-%d",
          analysisDates.outstandingShares,
          "%Y-%m-%d",
          maxDayErrorOutstandingShareData,
          commonAB,
          indicesA,
          analysisDates.indicesOutstandingShares,
          allowRepeatedDates);

      //Go through commonAB and common and erase any entries in common that
      //don't exist in commonAB;
      if(commonAB.size() < analysisDates.common.size() && validDates){
        size_t indexA = 0;
        while(indexA < analysisDates.common.size()){
          std::string dateA = analysisDates.common[indexA];
          bool found = false;
          for(auto& dateB :commonAB){
            if(dateA.compare(dateB) == 0){
              found = true;
              break;
            }
          }
          if(found == false){
            analysisDates.common.erase(
                analysisDates.common.begin()+indexA);
            analysisDates.indicesFinancial.erase(
                analysisDates.indicesFinancial.begin()+indexA);
            analysisDates.indicesHistorical.erase(
                analysisDates.indicesHistorical.begin()+indexA);
          }else{
            ++indexA;
          }        
        }                                    
      }
    }

    //Extract common dates between
    // financial
    // historical
    // outstandingShares
    // bond

    // Do this by getting common dates between
    // common
    // bond
    //
    // And removing missing dates in 
    // indicesFinancial 
    // indicesHistorical
    // indicesOutstandingShares

    // That are not a part of the new common
    if(validDates){
      std::vector< std::string > commonAB;
      std::vector< unsigned int> indicesA;

      validDates =     
        extractDatesOfClosestMatch(
          analysisDates.common,
          "%Y-%m-%d",
          analysisDates.bond,
          "%Y-%m-%d",
          maxDayErrorHistoricalData,
          commonAB,
          indicesA,
          analysisDates.indicesBond,
          allowRepeatedDates);

      //Go through commonAB and common and erase any entries in common that
      //don't exist in commonAB;
      if(commonAB.size() < analysisDates.common.size() && validDates){
        size_t indexA = 0;
        while(indexA < analysisDates.common.size()){
          std::string dateA = analysisDates.common[indexA];
          bool found = false;
          for(auto& dateB :commonAB){
            if(dateA.compare(dateB) == 0){
              found = true;
              break;
            }
          }
          if(found == false){
            analysisDates.common.erase(
                analysisDates.common.begin()+indexA);
            analysisDates.indicesFinancial.erase(
                analysisDates.indicesFinancial.begin()+indexA);
            analysisDates.indicesHistorical.erase(
                analysisDates.indicesHistorical.begin()+indexA);
            analysisDates.indicesOutstandingShares.erase(
                analysisDates.indicesOutstandingShares.begin()+indexA);
          }else{
            ++indexA;
          }        
        }                                    
      }
    }

    //Check to make sure that all date and index vectors are the same
    //length. Note that they won't all necessarily have the same date
    //because some error is allowed to accomodate for the fact that
    //sometimes financial data is filed on a day when an exchange is closed
    if(validDates){
      
      validDates = validDates 
        && (analysisDates.common.size()
            ==analysisDates.indicesFinancial.size());
      
      validDates = validDates 
        && (analysisDates.common.size()
            ==analysisDates.indicesHistorical.size());
      
      validDates = validDates 
        && (analysisDates.common.size()
            ==analysisDates.indicesOutstandingShares.size());
      
      validDates = validDates 
        && (analysisDates.common.size()
            ==analysisDates.indicesBond.size());
    }

  }


  return validDates;

};

//============================================================================
double getTaxRateFromTable(
        const std::string& countryISO2, 
        int year, 
        int yearMin, 
        const TaxFoundationDataSet &taxDataSet)
{

  double taxRate = std::numeric_limits<double>::quiet_NaN();                              

  //Get the country index
  bool foundCountry=false;
  int indexCountry = 0;
  while(indexCountry < taxDataSet.CountryISO3.size() && !foundCountry ){
    if(countryISO2.compare(taxDataSet.CountryISO2[indexCountry])==0){
      foundCountry=true;
    }else{
      ++indexCountry;
    }
  }  

  int indexYearMin  = -1;
  int indexYear     = -1;

  if(foundCountry){
    //Get the closest index to yearMin
    int index         = 0;
    int err           = 1;

    int errYear       = std::numeric_limits<int>::max();

    while(index < taxDataSet.year.size() && err > 0 ){
      err = yearMin - taxDataSet.year[index];
      if(err < errYear ){
        errYear = err;
        indexYearMin = index;
      }else{
        ++index;
      }
    }  
    //Get the closest index to year
    index     = indexYearMin;
    err       = 1;
    errYear   = std::numeric_limits<int>::max();

    while(index < taxDataSet.year.size() && err > 0 ){
      err = year - taxDataSet.year[index];
      if(err < errYear ){
        errYear = err;
        indexYear = index;
      }else{
        ++index;
      }
    }  
  }

  //Scan backwards from the most recent acceptable year to year min
  //and return the first valid tax rate
  if(foundCountry && (indexYear > 0 && indexYearMin > 0)){
    bool validIndex=true;

    int index = indexYear;
    taxRate = taxDataSet.taxTable[indexCountry][index];
    while( std::isnan(taxRate) && index > indexYearMin && validIndex){
      --index;
      if(index < 0){
        validIndex = false;
      }else{
        taxRate = taxDataSet.taxTable[indexCountry][index];   
      }
    }
    
  }

  return taxRate;

};
//============================================================================
double calcAverageTaxRate(  const AnalysisDates &analysisDates,
                            const std::string& countryISO2, 
                            const TaxFoundationDataSet &corpWorldTaxTable,
                            double defaultTaxRate,                            
                            int maxYearErrorTaxRateTable,
                            bool quarterlyTTMAnalysis,
                            int maxDayErrorTTM)
{

  int indexDate                 = 0;
  int numberOfDatesPerIteration = 0;
  int numberOfIterations        = 0;

  double taxRate                            = 0.;
  double meanTaxRate                        = 0.;
  unsigned int taxRateEntryCount            = 0;

      while( (indexDate+1) < analysisDates.common.size()){

        ++indexDate;

        std::string date = analysisDates.common[indexDate]; 
        
        //The set of dates used for the TTM analysis
        std::vector < std::string > dateSet;
        std::vector < double > dateSetWeight;
        if(quarterlyTTMAnalysis){
          bool validDateSet = extractTTM(indexDate,
                                    analysisDates.common,
                                    "%Y-%m-%d",
                                    dateSet,
                                    dateSetWeight,
                                    maxDayErrorTTM);                                     
          if(!validDateSet){
            break;
          }

        }else{
            dateSet.push_back(date);
            dateSetWeight.push_back(1.0);
        }                             
        
        ++numberOfIterations;
        numberOfDatesPerIteration += static_cast<int>(dateSet.size());
        
        taxRate=0.0;
        for(unsigned int j=0; j<dateSet.size();++j){
          int year  = std::stoi(dateSet[j].substr(0,4));
          int yearMin = year-maxYearErrorTaxRateTable;
          double taxRateDate = getTaxRateFromTable(countryISO2, year, yearMin, 
                                        corpWorldTaxTable);
          taxRate += taxRateDate*dateSetWeight[j];
        }
        taxRate = taxRate*0.01;                                   
        if(std::isnan(taxRate)){
          taxRate=defaultTaxRate;
        }                              

        meanTaxRate += taxRate;        
        ++taxRateEntryCount;    
    }        
      //=======================================================================
      
      if(taxRateEntryCount > 0){
        meanTaxRate = meanTaxRate / static_cast<double>(taxRateEntryCount);
      }else{
        meanTaxRate = std::nan("1");
      }
      
      return meanTaxRate;

};

//============================================================================
bool loadTaxFoundationDataSet(std::string &fileName, 
                              TaxFoundationDataSet &dataSet){

  bool validFormat=false;                                  
  std::ifstream file(fileName);

  if(file.is_open()){
    validFormat=true;
    std::string line;
    std::string entry;

    //First line is the header: check entries + read in years    
    std::getline(file,line); 

    std::size_t idx0=0;
    std::size_t idx1=0;
    int column = 0;
    std::vector< double > dataRow;
    dataRow.clear();

    idx1 = line.find_first_of(',',idx0);
    entry = line.substr(idx0,idx1-idx0);

    do{
        if(column <= 4){
          switch(column){
            case 1:{
                if(entry.compare("iso_2") != 0){
                  validFormat=false;
                }
            }break;
            case 2:{
              if(entry.compare("iso_3") != 0){
                  validFormat=false;
              }
            } break;
            case 3:{
              if(entry.compare("continent") != 0){
                  validFormat=false;
              }
            }break;
            case 4:{
              if(entry.compare("country") != 0){
                  validFormat=false;
              }                
            }break;
          };
        }else{
          if(entry.compare("NA") == 0){
            dataSet.year.push_back(-1);
            validFormat=false;
          }else{
            dataSet.year.push_back(std::stoi(entry));
          }
        }

        idx0 = idx1+1;
        idx1 = line.find_first_of(',',idx0);
        entry = line.substr(idx0,idx1-idx0);

        ++column;
    }while(    idx0 !=  std::string::npos 
            && idx1 !=  std::string::npos 
            && validFormat);


    //Read in the data table
    while(std::getline(file,line) && validFormat){

        idx0=0;
        idx1=0;
        dataRow.clear();
        column = 0;
        idx1 = line.find_first_of(',',idx0);
        entry = line.substr(idx0,idx1-idx0);

        do{


          if(column <= 4){
            switch(column){
              case 0:{
                  dataSet.index.push_back(std::stoi(entry));
              } break;
              case 1:{
                  dataSet.CountryISO2.push_back(entry);
              }break;
              case 2:{
                dataSet.CountryISO3.push_back(entry);
              } break;
              case 3:{
                dataSet.continent.push_back(entry);
              }break;
              case 4:{
                dataSet.country.push_back(entry);                
              }break;
            };
          }else{
            if(entry.compare("NA") == 0){
              dataRow.push_back(std::nan("1"));
            }else{
              dataRow.push_back(std::stod(entry));
            }
          }
          idx0 = idx1+1;
          idx1 = line.find_first_of(',',idx0);
          entry = line.substr(idx0,idx1-idx0);

          if(entry.find_first_of('"') != std::string::npos){
            //Opening bracket
            idx1 = line.find_first_of('"',idx0);
            //Closing bracket
            idx1 = line.find_first_of('"',idx1+1);
            //Final comma
            idx1 = line.find_first_of(',',idx1);
            entry = line.substr(idx0,idx1-idx0);
          }

          ++column;
        }while( idx0 !=  std::string::npos && idx1 !=  std::string::npos );

        dataSet.taxTable.push_back(dataRow);


    }
    file.close();
  }else{
   std::cout << " Warning: Reverting to default tax rate. Failed to " 
             << " read in " << fileName << std::endl; 
  }

  return validFormat;
};
//============================================================================
double calcAverageInterestCover(  
          const AnalysisDates &analysisDates,
          const nlohmann::ordered_json &fundamentalData,
          double defaultInterestCover,
          const std::string &timePeriod,
          bool quarterlyTTMAnalysis,
          int maxDayErrorTTM,
          bool setNansToMissingValue,
          bool appendTermRecord)
{

  int indexDate                       = 0;
  bool validDateSet                   = true;      

  double meanInterestCover            = 0.;
  double meanInterestCoverEntryCount  = 0;


  bool appendTermRecordLocal=false;


  while( (indexDate+1) < analysisDates.common.size() && validDateSet){

    ++indexDate;

    std::string date = analysisDates.common[indexDate]; 
    
    //The set of dates used for the TTM analysis
    std::vector < std::string > dateSet;
    std::vector < double > dateSetWeight;
    if(quarterlyTTMAnalysis){
      validDateSet = extractTTM(indexDate,
                                analysisDates.common,
                                "%Y-%m-%d",
                                dateSet,
                                dateSetWeight,
                                maxDayErrorTTM);                                     
      if(!validDateSet){
        break;
      }

    }else{
      dateSet.push_back(date);
      dateSetWeight.push_back(1.0);
    }

    std::vector<std::string> localTermNames;
    std::vector<double> localTermValues;

    double interestCover = FinancialAnalysisToolkit::
        calcInterestCover(fundamentalData,
                          dateSet,
                          defaultInterestCover,
                          timePeriod.c_str(),
                          appendTermRecord,
                          setNansToMissingValue,
                          localTermNames,
                          localTermValues);
    meanInterestCover += interestCover;                          

    meanInterestCoverEntryCount += 1.0; 

  }

  meanInterestCover = meanInterestCover / meanInterestCoverEntryCount;

  return meanInterestCover;

};
//============================================================================
double calcLastValidDateIndex(
          const AnalysisDates &analysisDates,
          bool quarterlyTTMAnalysis,
          int maxDayErrorTTM)
{

  int indexDate                    = 0;
  bool validDateSet                = true;      
  double numberOfDatesPerIteration = 0;
  double numberOfIterations        = 0;

  while( (indexDate+1) < analysisDates.common.size() && validDateSet){

    ++indexDate;

    std::string date = analysisDates.common[indexDate]; 
    
    //The set of dates used for the TTM analysis
    std::vector < std::string > dateSet;
    std::vector < double > dateSetWeight;
    if(quarterlyTTMAnalysis){
      validDateSet = extractTTM(indexDate,
                                analysisDates.common,
                                "%Y-%m-%d",
                                dateSet,
                                dateSetWeight,
                                maxDayErrorTTM);                                     
      if(!validDateSet){
        break;
      }

    }                               
    ++numberOfIterations;
    numberOfDatesPerIteration += static_cast<int>(dateSet.size());    
  }        
  

  double tmp =  (static_cast<double>(numberOfDatesPerIteration) 
                /static_cast<double>(numberOfIterations));

  numberOfDatesPerIteration =  static_cast<int>( std::round(tmp) );

  --indexDate;
  return indexDate;


};


//============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string eodFolder;
  std::string analyseFolder;
  bool quarterlyTTMAnalysis;
  std::string timePeriod;
  
  std::string defaultSpreadJsonFile;  
  std::string bondYieldJsonFile;  
  std::string corpTaxesWorldFile;

  double defaultInterestCover;

  double defaultRiskFreeRate;
  double equityRiskPremium;
  double defaultBeta;

  std::string singleFileToEvaluate;

  double defaultTaxRate;
  int numberOfYearsToAverageCapitalExpenditures;

  int numberOfYearsForTerminalValuation;
  int maxDayErrorTabularData;
  bool relaxedCalculation;

  double matureFirmFractionOfDebtCapital=0;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will analyze fundamental and end-of-data"
    "data (from https://eodhistoricaldata.com/) and "
    "write the results of the calculation to a json file in an output directory"
    ,' ', "0.0");


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

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");
    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> defaultSpreadJsonFileInput("d",
      "default_spread_json_file_input", 
      "The path to the json file that contains a table relating interest"
      " coverage to default spread",false,"","string");
    cmd.add(defaultSpreadJsonFileInput);  

    TCLAP::ValueArg<std::string> bondYieldJsonFileInput("y",
      "bond_yield_file_input", 
      "The path to the json file that contains a long historical record"
      " of 10 year bond yield values",false,"","string");
    cmd.add(bondYieldJsonFileInput);  
    

    TCLAP::ValueArg<double> defaultInterestCoverInput("c",
      "default_interest_cover", 
      "The default interest cover that is used if an average value cannot"
      "be computed from the data provided. This can happen with older "
      "EOD records",
      false,2.5,"double");
    cmd.add(defaultInterestCoverInput);  

    TCLAP::ValueArg<double> defaultTaxRateInput("t",
      "default_tax_rate", 
      "The tax rate used if the tax rate cannot be found in tabular data."
      " The default is 0.256 (25.6%) which is the world wide average"
      "weighted by GDP reported by the tax foundation.",
      false,0.256,"double");
    cmd.add(defaultTaxRateInput);  

    TCLAP::ValueArg<std::string> corpTaxesWorldFileInput("w","global_corporate_tax_rate_file", 
      "Corporate taxes reported around the world from the tax foundation"
      " in csv format (https://taxfoundation.org/data/all/global/corporate-tax-rates-by-country-2023/)",
      false,"","string");
    cmd.add(corpTaxesWorldFileInput);  
    TCLAP::ValueArg<double> defaultRiskFreeRateInput("r",

      "default_risk_free_rate", 
      "The risk free rate of return, which is often set to the return on "
      "a 10 year or 30 year bond as noted from Ch. 3 of Damodran.",
      false,0.025,"double");
    cmd.add(defaultRiskFreeRateInput);  

    TCLAP::ValueArg<double> equityRiskPremiumInput("e",
      "equity_risk_premium", 
      "The extra return that the stock should return given its risk. Often this"
      " is set to the historical incremental rate of return provided by the "
      " stock market relative to the bond market. In the U.S. this is somewhere"
      " around 4 percent between 1928 and 2010 as noted in Ch. 3 Damodran.",
      false,0.05,"double");
    cmd.add(equityRiskPremiumInput);  

    TCLAP::ValueArg<double> defaultBetaInput("b",
      "default_beta", 
      "The default beta value to use when one is not reported",
      false,1.0,"double");
    cmd.add(defaultBetaInput);  

    //Default value from Ch. 3 Damodaran
    TCLAP::ValueArg<double> matureFirmFractionOfDebtCapitalInput("u",
      "mature_firm_fraction_debt_capital", 
      "The fraction of capital from debt for a mature firm.",
      false,0.2,"double");
    cmd.add(matureFirmFractionOfDebtCapitalInput);  

    TCLAP::ValueArg<int> numberOfYearsToAverageCapitalExpendituresInput("n",
      "number_of_years_to_average_capital_expenditures", 
      "Number of years used to evaluate capital expenditures."
      " Default value of 3 taken from Ch. 12 of Lev and Gu.",
      false,3,"int");
    cmd.add(numberOfYearsToAverageCapitalExpendituresInput);  

    TCLAP::ValueArg<int>numberOfYearsForTerminalValuationInput("m",
      "number_of_years_for_terminal_valuation_calculation", 
      "Number of years for terminal valuation calculation."
      " Default value of 5 taken from Ch. 3 of Damodran.",
      false,5,"int");
    cmd.add(numberOfYearsForTerminalValuationInput); 


    TCLAP::ValueArg<int>maxDayErrorTabularDataInput("a",
      "day_error", 
      "The bond value and stock value tables do not have entries for"
      " every day. This parameter specifies the maximum amount of error"
      " that is acceptable. Note: the bond table is sometimes missing a "
      " 30 days of data while the stock value tables are sometimes missing"
      " 10 days of data.",
      false,35,"int");
    cmd.add(maxDayErrorTabularDataInput); 

    TCLAP::SwitchArg relaxedCalculationInput("l","relaxed",
      "Relaxed calculation: nulls for some values (short term debt,"
      " research and development, dividends, depreciation) are set to "
      " zero, while substitute calculations are used to replace "
      " null values (shortLongDebt replaced with longDebt)", false);
    cmd.add(relaxedCalculationInput); 

    TCLAP::SwitchArg quarterlyTTMAnalysisInput("q","trailing_twelve_months",
      "Analyze trailing twelve moneths using quarterly data.", false);
    cmd.add(quarterlyTTMAnalysisInput); 

    TCLAP::ValueArg<std::string> analyseFolderOutput("o","output_folder_path", 
      "The path to the folder that will contain the output json files "
      "produced by these calculations",
      true,"","string");
    cmd.add(analyseFolderOutput);


    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    fundamentalFolder     = fundamentalFolderInput.getValue();
    singleFileToEvaluate = singleFileToEvaluateInput.getValue();
    historicalFolder      = historicalFolderInput.getValue();
    exchangeCode          = exchangeCodeInput.getValue();    
    analyseFolder         = analyseFolderOutput.getValue();
    quarterlyTTMAnalysis  = quarterlyTTMAnalysisInput.getValue();

    defaultTaxRate      = defaultTaxRateInput.getValue();
    corpTaxesWorldFile  = corpTaxesWorldFileInput.getValue();
    defaultRiskFreeRate = defaultRiskFreeRateInput.getValue();
    defaultBeta         = defaultBetaInput.getValue();
    equityRiskPremium   = equityRiskPremiumInput.getValue();


    defaultSpreadJsonFile = defaultSpreadJsonFileInput.getValue();
    bondYieldJsonFile     = bondYieldJsonFileInput.getValue();
    defaultInterestCover  = defaultInterestCoverInput.getValue();

    matureFirmFractionOfDebtCapital = 
      matureFirmFractionOfDebtCapitalInput.getValue();

    numberOfYearsToAverageCapitalExpenditures 
      = numberOfYearsToAverageCapitalExpendituresInput.getValue();              

    numberOfYearsForTerminalValuation
      = numberOfYearsForTerminalValuationInput.getValue();

    maxDayErrorTabularData
      = maxDayErrorTabularDataInput.getValue();

    relaxedCalculation  = relaxedCalculationInput.getValue() ;

    verbose             = verboseInput.getValue();

    if(quarterlyTTMAnalysis){
      timePeriod                  = Q;
    }else{
      timePeriod                  = Y;
    }   

    if(verbose){
      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;
      
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Single file name to evaluate" << std::endl;
      std::cout << "    " << singleFileToEvaluate << std::endl;

      std::cout << "  Default Spread Json File" << std::endl;
      std::cout << "    " << defaultSpreadJsonFile << std::endl;

      std::cout << "  Bond Yield Json File" << std::endl;
      std::cout << "    " << bondYieldJsonFile << std::endl;

      std::cout << "  Default interest cover value" << std::endl;
      std::cout << "    " << defaultInterestCover << std::endl;

      std::cout << "  Analyze TTM using Quaterly Data" << std::endl;
      std::cout << "    " << quarterlyTTMAnalysis << std::endl;

      std::cout << "  Default tax rate" << std::endl;
      std::cout << "    " << defaultTaxRate << std::endl;

      std::cout << "  Corporate tax rate file from https://taxfoundation.org"
                << std:: endl;
      std::cout << "    " << corpTaxesWorldFile << std::endl;

      std::cout << "  Annual default risk free rate" << std::endl;
      std::cout << "    " << defaultRiskFreeRate << std::endl;

      std::cout << "  Annual equity risk premium" << std::endl;
      std::cout << "    " << equityRiskPremium << std::endl;

      std::cout << "  Default beta value" << std::endl;
      std::cout << "    " << defaultBeta << std::endl;

      std::cout << "  Assumed mature firm fraction of capital from debt" << std::endl;
      std::cout << "    " << matureFirmFractionOfDebtCapital << std::endl;

      std::cout << "  Number of years to use when evaluating the terminal value " 
                << std::endl;
      std::cout << "    " << numberOfYearsForTerminalValuation 
                << std::endl;

      std::cout << "  Maximum number of days in error allowed for tabular " 
                << "data of bond yields and historical stock prices " 
                << std::endl;                
      std::cout << "    " << maxDayErrorTabularData 
                << std::endl;    

      std::cout << "  Using relaxed calculations?" << std::endl;
      std::cout << relaxedCalculation << std::endl;

      std::cout << "  Analyse Folder" << std::endl;
      std::cout << "    " << analyseFolder << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  int acceptableBackwardsYearErrorForTaxRate = 5;
  //This defines how far back in time data from the tax table can be taken
  //to approximate the tax of this year.

  int maxDayErrorHistoricalData = maxDayErrorTabularData; 
  // Historical data has a resolution of 1 day
  
  int maxDayErrorBondYieldData  = maxDayErrorTabularData; 
  // Bond yield data has a resolution of 1 month

  int maxDayErrorTTM = maxDayErrorTabularData;

  int maxDayErrorOutstandingShareData = 365; 


  //2024/8/4 
  //  Note: some fields used in the functions in the FinanacialToolkit
  //  will replace nan's with a coded MISSING_NUMBER. Why?
  //  Some reports are getting trashed by nans in fields that should 
  //  actually be nan. For example, META is not a business that has inventory
  //  and so this is not reported. Naturally when inventory doesn't
  //  appear, or is set to nan, it then turns the results of all
  //  analysis done using this term to nan. And this, unfortunately,
  //  later means that these securities are ignored in later analysis.

  bool setNansToMissingValue                  = relaxedCalculation;


  //When this is true all intermediate values of a calculation are saved
  //and written to file to permit manual inspection.  
  bool appendTermRecord                       = true;
  std::vector< std::string >  termNames;
  std::vector< double >       termValues;  

  bool loadSingleTicker=false;
  if(singleFileToEvaluate.size() > 0){
    loadSingleTicker = true;
  }

  std::string validFileExtension = exchangeCode;
  validFileExtension.append(".json");

  auto startingDirectory = std::filesystem::current_path();
  std::filesystem::current_path(fundamentalFolder);

  unsigned int count=0;

  //============================================================================
  // Load the corporate tax rate table
  //============================================================================
  bool flag_usingTaxTable=false;
  TaxFoundationDataSet corpWorldTaxTable;
  if(corpTaxesWorldFile.length() > 0){
    flag_usingTaxTable=true;
    bool validFormat = 
      loadTaxFoundationDataSet(corpTaxesWorldFile,corpWorldTaxTable);

    if(!validFormat){
      flag_usingTaxTable=false;
      std::cout << "Warning: could not load the world corporate tax rate file "
                << corpTaxesWorldFile << std::endl;
      std::cout << "Reverting to the default rate " << std::endl;
    }
  }
  //============================================================================
  // Load the default spread array 
  //============================================================================
  using json = nlohmann::ordered_json;    

  std::ifstream defaultSpreadFileStream(defaultSpreadJsonFile.c_str());
  json jsonDefaultSpread = 
    nlohmann::ordered_json::parse(defaultSpreadFileStream);
  //std::cout << jsonDefaultSpread.at(1).at(2);
  if(verbose){
    std::cout << std::endl;
    std::cout << "default spread table" << std::endl;
    std::cout << '\t'
              << "Interest Coverage Interval" 
              << '\t'
              << '\t' 
              << "default spread" << std::endl;
    for(auto &row: jsonDefaultSpread["US"]["default_spread"].items()){
      for(auto &ele: row.value()){
        std::cout << '\t' << ele << '\t';
      }
      std::cout << std::endl;
    }
  }

  //============================================================================
  // Load the 10 year bond yield table 
  // Note: 1. Right now I only have historical data for the US, and so I'm
  //          approximating the bond yield of all countries using US data.
  //          This is probably approximately correct for wealthy developed
  //          countries that have access to international markets, but is 
  //          terrible for countries outside of this group.
  //       
  //============================================================================
  using json = nlohmann::ordered_json;    

  std::ifstream bondYieldFileStream(bondYieldJsonFile.c_str());
  json jsonBondYield = nlohmann::ordered_json::parse(bondYieldFileStream);

  if(verbose){
    std::size_t numberOfEntries = jsonBondYield["US"]["10y_bond_yield"].size();
    std::string startKey = jsonBondYield["US"]["10y_bond_yield"].begin().key();
    std::string endKey = (--jsonBondYield["US"]["10y_bond_yield"].end()).key();
    std::cout << std::endl;
    std::cout << "bond yield table with " 
              << numberOfEntries
              << " entries from "
              << startKey
              << " to "
              << endKey
              << std::endl;
    std::cout << "  Warning**" << std::endl; 
    std::cout << "    The 10 year bond yields from the US are being used to " 
              << std::endl;          
    std::cout << "    approximate the bond yields from all countries. The "   
              << std::endl; 
    std::cout << "    bond yields, in turn, are being used to approximate"    
              << std::endl; 
    std::cout << "    the risk free rate which is used in the calculation "   
              << std::endl; 
    std::cout << "    of the cost of capital and the cost of debt."           
              << std::endl; 
    std::cout << std::endl;
  }



  //============================================================================
  //
  // Evaluate every file in the fundamental folder
  //
  //============================================================================  
  int validFileCount=0;
  for ( const auto & entry 
          : std::filesystem::directory_iterator(fundamentalFolder)){


    bool validInput = true;

    //==========================================================================
    //Load the (primary) fundamental ticker file
    //==========================================================================
    std::string fileName=entry.path().filename();

    if(loadSingleTicker){
      fileName  = singleFileToEvaluate;
      size_t idx = singleFileToEvaluate.find(".json");
      if(idx == std::string::npos){
        fileName.append(".json");
      }
    }
    size_t lastIndex = fileName.find_last_of(".");
    std::string tickerName = fileName.substr(0,lastIndex);
    std::size_t foundExtension = fileName.find(validFileExtension);

    nlohmann::ordered_json fundamentalData;

    if( foundExtension != std::string::npos ){
        std::string primaryTickerName("");
        JsonFunctions::getPrimaryTickerName(fundamentalFolder, 
                                            fileName,
                                            primaryTickerName);
        ++validFileCount;                                            
        if(verbose){
          std::cout << validFileCount << "." << '\t' << fileName << std::endl;
          if(primaryTickerName.compare(tickerName) != 0){
            std::cout << "  " << '\t' << primaryTickerName 
                      << " (PrimaryTicker) " << std::endl;
          }
        }

        //Try to load the primary ticker
        if(validInput && primaryTickerName.length()>0){
          std::string primaryFileName = primaryTickerName;
          primaryFileName.append(".json");
          validInput = JsonFunctions::loadJsonFile(primaryFileName, 
                        fundamentalFolder, fundamentalData, verbose);
          if(validInput){
            fileName = primaryFileName;
            tickerName = primaryTickerName;
          }                                                  
        }

        //If the primary ticker doesn't load (or exist) then use the file
        //from the local exchange
        if(!validInput || primaryTickerName.length()==0){
          validInput = JsonFunctions::loadJsonFile(fileName, fundamentalFolder, 
                                                  fundamentalData, verbose);
          if(verbose){
            if(validInput){
              std::cout << "  Proceeding with "<< tickerName 
                        << " : " << primaryTickerName 
                        << " failed to load " << std::endl;

            }else{
              std::cout << "  Skipping: both " 
                        << tickerName << " and " << primaryTickerName
                        << " failed to load" << std::endl;
            }
          }                                                  
        }

    }else{
      //Skip: this file doesn't have an extension
      validInput = false;      
    }

    //Extract the list of entry dates for the fundamental data
    //std::vector< std::string > datesFundamental;
    //std::vector< std::string > datesOutstandingShares;
    std::string timePeriodOS(timePeriod);
    if(timePeriodOS.compare(Y)==0){
      timePeriodOS = A;
    }

    //Extract the countryName and ISO
    std::string countryName;
    std::string countryISO2;
    if(validInput){
      JsonFunctions::getJsonString(fundamentalData[GEN]["CountryName"],
                                   countryName);
      JsonFunctions::getJsonString(fundamentalData[GEN]["CountryISO"],
                                   countryISO2);

    }

    //==========================================================================
    //Load the (primary) historical (price) file
    //==========================================================================
    nlohmann::ordered_json historicalData;
    if(validInput){
      validInput=JsonFunctions::loadJsonFile(fileName, historicalFolder, 
                                            historicalData, verbose);
    }

    std::vector< std::string > datesBondYields;
    
    AnalysisDates analysisDates;
    bool allowRepeatedDates=false;

    if(validInput){   

      bool validDates= 
        extractAnalysisDates(
          analysisDates,
          fundamentalData,
          historicalData,
          jsonBondYield["US"]["10y_bond_yield"],
          timePeriod,
          timePeriodOS,
          maxDayErrorHistoricalData,
          maxDayErrorOutstandingShareData,
          maxDayErrorBondYieldData,
          allowRepeatedDates);    

      validInput = (validInput && validDates);
      if(verbose && !validDates){
        std::cout << "    Skipping: no valid dates" << std::endl;
      }
    }
    //==========================================================================
    //
    // Process these files, if all of the inputs are valid
    //
    //==========================================================================
    nlohmann::ordered_json analysis;
    if(validInput){

      std::vector< std::vector< std::string >> trailingPastPeriods;
      std::string previousTimePeriod("");
      std::vector< std::string > previousDateSet;
      std::vector< double > previousDateSetWeight;
      unsigned int entryCount = 0;

      //========================================================================
      //Calculate 
      //  average tax rate
      //  average interest cover    
      //========================================================================
      std::vector< std::string > tmpNames;
      std::vector< double > tempValues;


      //This assumes that the beta is the same for all time. This is 
      //obviously wrong, but I only have one data point for beta from EOD's
      //data.      
      //
      double betaUnlevered = 
        JsonFunctions::getJsonFloat(fundamentalData[TECH]["Beta"]);
      if(std::isnan(betaUnlevered)){
        betaUnlevered=defaultBeta;
      }

      //========================================================================
      // Evaluate the average tax rate and interest cover
      //========================================================================

      double meanTaxRate = 0.;
      if(flag_usingTaxTable){        
        meanTaxRate=
        calcAverageTaxRate( analysisDates,
                            countryISO2, 
                            corpWorldTaxTable,
                            defaultTaxRate,                            
                            acceptableBackwardsYearErrorForTaxRate,
                            quarterlyTTMAnalysis,
                            maxDayErrorTabularData);        

      }else{
        meanTaxRate=defaultTaxRate;
      }

      double meanInterestCover = 
        calcAverageInterestCover(  
          analysisDates,
          fundamentalData,
          defaultInterestCover,
          timePeriod,
          quarterlyTTMAnalysis,
          maxDayErrorTTM,
          setNansToMissingValue,
          appendTermRecord);

      //========================================================================
      // Evaluate the last valid index
      //========================================================================

      int indexLastCommonDate = 
        calcLastValidDateIndex(
            analysisDates,
            quarterlyTTMAnalysis,
            maxDayErrorTTM);

      //=======================================================================
      //
      //  Calculate the metrics for every data entry in the file
      //
      //======================================================================= 

      int indexDate       = -1;

      bool validDateSet    = true;

      while( (indexDate+1) < indexLastCommonDate && validDateSet){

        ++indexDate;
        std::string date = analysisDates.common[indexDate]; 

                
        //The set of dates used for the TTM analysis
        std::vector < std::string > dateSet;
        std::vector < double > dateSetWeight;

      
        if(quarterlyTTMAnalysis){
          validDateSet = extractTTM(indexDate,
                                    analysisDates.common,
                                    "%Y-%m-%d",
                                    dateSet,                                    
                                    dateSetWeight,
                                    maxDayErrorTTM); 
          if(!validDateSet){
            break;
          }     
        }else{
          dateSet.push_back(date);
        }           
             
        //======================================================================
        //Update the list of previous time periods
        //======================================================================        

        //Check if we have enough data to get the previous time period
        int indexPrevious = indexDate+static_cast<int>(dateSet.size());        

        //Fetch the previous TTM
        previousTimePeriod = analysisDates.common[indexPrevious];
        previousDateSet.resize(0);

        if(quarterlyTTMAnalysis){
          validDateSet = extractTTM(indexPrevious,
                                    analysisDates.common,
                                    "%Y-%m-%d",
                                    previousDateSet,
                                    previousDateSetWeight,
                                    maxDayErrorTTM); 
          if(!validDateSet){
            break;
          }     
        }else{
          previousDateSet.push_back(previousTimePeriod);
        } 

        termNames.clear();
        termValues.clear();

        //======================================================================
        //Update the list of past periods
        //======================================================================        
        for(size_t i=0; i<trailingPastPeriods.size();++i){
          trailingPastPeriods[i].clear();
        }
        trailingPastPeriods.clear();

        bool sufficentPastPeriods=true;
        int indexPastPeriods = indexDate;
        bool validPreviousDateSet = true;

        for(unsigned int i=0; 
                         i<(numberOfYearsToAverageCapitalExpenditures); 
                       ++i)
        {

          std::vector< std::string > pastDateSet;
          std::vector< double > pastDateSetWeight;

          if(validPreviousDateSet){
            if(quarterlyTTMAnalysis){
              validPreviousDateSet = extractTTM(indexPastPeriods,
                                        analysisDates.common,
                                        "%Y-%m-%d",
                                        pastDateSet,
                                        pastDateSetWeight,
                                        maxDayErrorTTM); 
            }else{
              if(indexPastPeriods < analysisDates.common.size()){
                pastDateSet.push_back(analysisDates.common[indexPastPeriods]);
              }else{
                validPreviousDateSet=false;
              }
            }           
          }
          if(validPreviousDateSet){
            trailingPastPeriods.push_back(pastDateSet);
            indexPastPeriods += pastDateSet.size();
          }

        }


        //======================================================================
        //Evaluate the risk free rate as the yield on a 10 year US bond
        //  It would be ideal, of course, to have the bond yields in the
        //  home country of the stock. I have not yet endevoured to find this
        //  information. Since US bonds are internationally traded
        //  (in London and Tokyo) this is perhaps not a horrible approximation.
        //======================================================================
        int indexBondYield = analysisDates.indicesBond[indexDate];
        std::string closestBondYieldDate= analysisDates.bond[indexBondYield]; 

        double bondYield = std::nan("1");
        try{
          bondYield = JsonFunctions::getJsonFloat(
              jsonBondYield["US"]["10y_bond_yield"][closestBondYieldDate],
              setNansToMissingValue); 
          bondYield = bondYield * (0.01); //Convert from percent to decimal form      
        }catch( std::invalid_argument const& ex){
          std::cout << " Bond yield record (" << closestBondYieldDate << ")"
                    << " is missing a value. Reverting to the default"
                    << " risk free rate."
                    << std::endl;
          bondYield=defaultRiskFreeRate;
        }        

        double riskFreeRate = bondYield;

        //======================================================================
        //Evaluate the cost of debt
        //======================================================================
        double interestCover = 
          FinancialAnalysisToolkit::calcInterestCover(
                                        fundamentalData,
                                        dateSet,
                                        defaultInterestCover,
                                        timePeriod.c_str(),
                                        appendTermRecord,
                                        setNansToMissingValue,
                                        termNames,
                                        termValues);

        double defaultSpread = FinancialAnalysisToolkit::
            calcDefaultSpread(fundamentalData,
                              dateSet,
                              timePeriod.c_str(),
                              interestCover,
                              jsonDefaultSpread,
                              appendTermRecord,
                              setNansToMissingValue,
                              termNames,
                              termValues);

        std::string parentName = "afterTaxCostOfDebt_";
        
        //======================================================================
        //Get the tax rate
        //======================================================================
        double taxRate=0.;
        if(flag_usingTaxTable){
          int year  = std::stoi(date.substr(0,4));
          int yearMin = year-acceptableBackwardsYearErrorForTaxRate;
          taxRate = getTaxRateFromTable(countryISO2, year, yearMin, 
                                        corpWorldTaxTable);
          taxRate = taxRate*0.01;  //convert from percent to decimal                                     
          if(std::isnan(taxRate)){
            taxRate=meanTaxRate;
          }                    
          if(std::isnan(taxRate)){
            taxRate=defaultTaxRate;
          }          
        }else{
          taxRate = defaultTaxRate;
        }        
                                
        double afterTaxCostOfDebt = (riskFreeRate+defaultSpread)*(1.0-taxRate);
        termNames.push_back("afterTaxCostOfDebt_riskFreeRate");
        termNames.push_back("afterTaxCostOfDebt_defaultSpread");
        termNames.push_back("afterTaxCostOfDebt_taxRate");
        termNames.push_back("afterTaxCostOfDebt");
        
        termValues.push_back(riskFreeRate);
        termValues.push_back(defaultSpread);
        termValues.push_back(taxRate);
        termValues.push_back(afterTaxCostOfDebt);

        //======================================================================
        //Evaluate the current total short and long term debt
        //======================================================================
        double longTermDebt = 
          JsonFunctions::getJsonFloat(
            fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                           ["longTermDebt"]);

        //======================================================================        
        //Evaluate the current market capitalization
        //======================================================================
        double outstandingShares = std::nan("1");
        int smallestDateDifference=std::numeric_limits<int>::max();        
        std::string closestDate("");
        for(auto& el : fundamentalData[OS][timePeriodOS.c_str()]){
          std::string dateOS("");
          JsonFunctions::getJsonString(el["dateFormatted"],dateOS);         
          int dateDifference = 
            FinancialAnalysisToolkit::calcDifferenceInDaysBetweenTwoDates(
              date,"%Y-%m-%d",dateOS,"%Y-%m-%d");
          if(std::abs(dateDifference)<smallestDateDifference){
            closestDate = dateOS;
            smallestDateDifference=std::abs(dateDifference);
            outstandingShares = JsonFunctions::getJsonFloat(el["shares"]);
          }
        }

        unsigned int indexHistoricalData = 
          analysisDates.indicesHistorical[indexDate];   

        std::string closestHistoricalDate= 
          analysisDates.historical[indexHistoricalData]; 

        double adjustedClosePrice = std::nan("1");
        double closePrice = std::nan("1");
        try{
          adjustedClosePrice = JsonFunctions::getJsonFloat(
                      historicalData[ indexHistoricalData ]["adjusted_close"],
                      setNansToMissingValue); 
          closePrice = JsonFunctions::getJsonFloat(
                      historicalData[ indexHistoricalData ]["close"],
                      setNansToMissingValue);                       

        }catch( std::invalid_argument const& ex){
          std::cout << " Historical record (" << closestHistoricalDate << ")"
                    << " is missing an opening share price."
                    << std::endl;
        }

        double marketCapitalization = 
          adjustedClosePrice*outstandingShares;


        //======================================================================
        //Evaluate the cost of equity
        //======================================================================        
        double beta = betaUnlevered*(1.0 + 
          (1.0-taxRate)*(longTermDebt/marketCapitalization));


        double annualcostOfEquityAsAPercentage = 
          riskFreeRate + equityRiskPremium*beta;

        double costOfEquityAsAPercentage=annualcostOfEquityAsAPercentage;
        //if(quarterlyTTMAnalysis){
        //  costOfEquityAsAPercentage = costOfEquityAsAPercentage/4.0;
        //}        

        termNames.push_back("costOfEquityAsAPercentage_riskFreeRate");
        termNames.push_back("costOfEquityAsAPercentage_equityRiskPremium");
        termNames.push_back("costOfEquityAsAPercentage_betaUnlevered");
        termNames.push_back("costOfEquityAsAPercentage_taxRate");
        termNames.push_back("costOfEquityAsAPercentage_longTermDebt");
        termNames.push_back("costOfEquityAsAPercentage_adjustedClose");
        termNames.push_back("costOfEquityAsAPercentage_outstandingShares");
        termNames.push_back("costOfEquityAsAPercentage_marketCapitalization");
        termNames.push_back("costOfEquityAsAPercentage_beta");
        termNames.push_back("costOfEquityAsAPercentage");

        termValues.push_back(riskFreeRate);
        termValues.push_back(equityRiskPremium);
        termValues.push_back(betaUnlevered);
        termValues.push_back(taxRate);
        termValues.push_back(longTermDebt);
        termValues.push_back(adjustedClosePrice);
        termValues.push_back(outstandingShares);
        termValues.push_back(marketCapitalization);
        termValues.push_back(beta);
        termValues.push_back(costOfEquityAsAPercentage);



        //======================================================================        
        //Evaluate a weighted cost of capital
        //======================================================================        

        double costOfCapital = 
          (costOfEquityAsAPercentage*marketCapitalization
          +afterTaxCostOfDebt*longTermDebt)
          /(marketCapitalization+longTermDebt);


        termNames.push_back("costOfCapital_longTermDebt");
        termNames.push_back("costOfCapital_outstandingShares");
        termNames.push_back("costOfCapital_adjustedClose");
        termNames.push_back("costOfCapital_marketCapitalization");
        termNames.push_back("costOfCapital_costOfEquityAsAPercentage");
        termNames.push_back("costOfCapital_afterTaxCostOfDebt");
        termNames.push_back("costOfCapital");

        termValues.push_back(longTermDebt);
        termValues.push_back(outstandingShares);
        termValues.push_back(adjustedClosePrice);
        termValues.push_back(marketCapitalization);
        termValues.push_back(costOfEquityAsAPercentage);
        termValues.push_back(afterTaxCostOfDebt);
        termValues.push_back(costOfCapital);


        //As companies mature they use cheaper forms of capital: debt.
        double costOfCapitalMature = 
          (costOfEquityAsAPercentage*(1-matureFirmFractionOfDebtCapital) 
          + afterTaxCostOfDebt*matureFirmFractionOfDebtCapital);

        if(costOfCapitalMature > costOfCapital){
          costOfCapitalMature=costOfCapital;
        }

        termNames.push_back("costOfCapitalMature_matureFirmDebtCapitalFraction");
        termNames.push_back("costOfCapitalMature_costOfEquityAsAPercentage");
        termNames.push_back("costOfCapitalMature_afterTaxCostOfDebt");
        termNames.push_back("costOfCapitalMature");

        termValues.push_back(matureFirmFractionOfDebtCapital);
        termValues.push_back(costOfEquityAsAPercentage);
        termValues.push_back(afterTaxCostOfDebt);        
        termValues.push_back(costOfCapitalMature);

        //======================================================================
        //Evaluate the metrics
        //  At the moment residual cash flow and the company's valuation are
        //  most of interest. The remaining metrics are useful, however, and so,
        //  have been included.
        //======================================================================        

        double totalStockHolderEquity =  
          FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            fundamentalData,FIN,BAL,timePeriod.c_str(),dateSet,
            "totalStockholderEquity", setNansToMissingValue);


        std::string emptyParentName("");
        double roic = FinancialAnalysisToolkit::
          calcReturnOnInvestedCapital(fundamentalData,
                                      dateSet,
                                      timePeriod.c_str(),
                                      appendTermRecord, 
                                      emptyParentName,
                                      setNansToMissingValue,
                                      termNames, 
                                      termValues);

        double roicLessCostOfCapital = roic - costOfCapitalMature;  
        termNames.push_back("returnOnInvestedCapitalLessCostOfCapital");
        termValues.push_back(roicLessCostOfCapital);                                    

        double roce = FinancialAnalysisToolkit::
          calcReturnOnCapitalDeployed(  fundamentalData,
                                        dateSet,
                                        timePeriod.c_str(), 
                                        appendTermRecord, 
                                        emptyParentName,
                                        setNansToMissingValue,
                                        termNames, 
                                        termValues);

        double grossMargin = FinancialAnalysisToolkit::
          calcGrossMargin(  fundamentalData,
                            dateSet,
                            timePeriod.c_str(),
                            appendTermRecord,
                            setNansToMissingValue,
                            termNames,
                            termValues);

        double operatingMargin = FinancialAnalysisToolkit::
          calcOperatingMargin(  fundamentalData,
                                dateSet,
                                timePeriod.c_str(), 
                                appendTermRecord,
                                setNansToMissingValue,
                                termNames,
                                termValues);          

        double cashConversion = FinancialAnalysisToolkit::
          calcCashConversionRatio(  fundamentalData,
                                    dateSet,
                                    timePeriod.c_str(), 
                                    taxRate,
                                    appendTermRecord,
                                    setNansToMissingValue,
                                    termNames,
                                    termValues);

        double debtToCapital = FinancialAnalysisToolkit::
          calcDebtToCapitalizationRatio(  fundamentalData,
                                          dateSet,
                                          timePeriod.c_str(),
                                          appendTermRecord,
                                          setNansToMissingValue,
                                          termNames,
                                          termValues);

        double ownersEarnings = FinancialAnalysisToolkit::
          calcOwnersEarnings( fundamentalData, 
                              dateSet, 
                              previousDateSet,
                              timePeriod.c_str(),
                              appendTermRecord, 
                              setNansToMissingValue,
                              termNames, 
                              termValues);  

        double residualCashFlow = std::nan("1");

        if(trailingPastPeriods.size() > 0){

          residualCashFlow = FinancialAnalysisToolkit::
            calcResidualCashFlow( fundamentalData,
                                  dateSet,
                                  timePeriod.c_str(),
                                  costOfEquityAsAPercentage,
                                  trailingPastPeriods,
                                  appendTermRecord,
                                  setNansToMissingValue,
                                  termNames,
                                  termValues);
        }

        double freeCashFlowToEquity=std::nan("1");
        if(previousTimePeriod.length()>0){
          freeCashFlowToEquity = FinancialAnalysisToolkit::
            calcFreeCashFlowToEquity(fundamentalData, 
                                     dateSet,
                                     previousDateSet,
                                     timePeriod.c_str(),
                                     appendTermRecord,
                                     setNansToMissingValue,
                                     termNames,
                                     termValues);
        }

        double freeCashFlowToFirm=std::nan("1");
        freeCashFlowToFirm = FinancialAnalysisToolkit::
          calcFreeCashFlowToFirm(fundamentalData, 
                                 dateSet, 
                                 previousDateSet, 
                                 timePeriod.c_str(),
                                 taxRate,
                                 appendTermRecord,
                                 setNansToMissingValue,
                                 termNames, 
                                 termValues);

        //Valuation (discounted cash flow)
        double presentValueOfFutureCashFlows = FinancialAnalysisToolkit::
            calcPresentValueOfDiscountedFutureCashFlows(  
              fundamentalData,
              dateSet,
              previousDateSet,
              timePeriod.c_str(),
              riskFreeRate,
              costOfCapital,
              costOfCapitalMature,
              taxRate,
              numberOfYearsForTerminalValuation,
              appendTermRecord,
              setNansToMissingValue,
              termNames,
              termValues);

        //Market value (make adjustments as described in Damodaran Ch. 3)
        double cash = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][BAL][timePeriod.c_str()][dateSet[0].c_str()]["cash"],
          true);

        double crossHoldings = JsonFunctions::MISSING_VALUE;

        double shortTermDebt = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                          ["shortTermDebt"],true);

        double shortLongTermDebtTotal = longTermDebt+shortTermDebt; //here

        double potentialLiabilities = JsonFunctions::MISSING_VALUE;

        double optionValue  = JsonFunctions::MISSING_VALUE;

        double presentValue = presentValueOfFutureCashFlows
                            + cash
                            + crossHoldings
                            - shortLongTermDebtTotal
                            - potentialLiabilities
                            - optionValue;

        double priceToValue = marketCapitalization / presentValue;

        if(  !JsonFunctions::isJsonFloatValid(presentValueOfFutureCashFlows)
          || !JsonFunctions::isJsonFloatValid(marketCapitalization)){
          if(setNansToMissingValue){
            priceToValue = JsonFunctions::MISSING_VALUE;
          }else{
            priceToValue = std::nan("1");
          }

        }
           
        //Ratio: price to value
        if(appendTermRecord){
          termNames.push_back("priceToValue_presentValueOfFutureCashFlows");
          termNames.push_back("priceToValue_cash");
          //termNames.push_back("priceToValue_netDebt");

          termNames.push_back("priceToValue_crossHolding");
          termNames.push_back("priceToValue_shortLongTermDebtTotal");
          termNames.push_back("priceToValue_potentialLiabilities");
          termNames.push_back("priceToValue_stockOptionValuation");
          termNames.push_back("priceToValue_presentValue_approximation");
          termNames.push_back("priceToValue_marketCapitalization");
          termNames.push_back("priceToValue");

          termValues.push_back(presentValueOfFutureCashFlows);
          termValues.push_back(cash);
          termValues.push_back(crossHoldings);          
          termValues.push_back(shortLongTermDebtTotal);
          termValues.push_back(potentialLiabilities);
          termValues.push_back(optionValue);
          termValues.push_back(presentValue);
          termValues.push_back(marketCapitalization);
          termValues.push_back(priceToValue);

        }

        //Residual cash flow to enterprise value
        std::string rFcfToEvLabel = "residualCashFlowToEnterpriseValue_"; 
        double enterpriseValue = FinancialAnalysisToolkit::
            calcEnterpriseValue(fundamentalData, 
                                marketCapitalization, 
                                dateSet,
                                timePeriod.c_str(),
                                appendTermRecord,                                
                                rFcfToEvLabel,
                                setNansToMissingValue,
                                termNames,
                                termValues);

        double residualCashFlowToEnterpriseValue = 
          residualCashFlow/enterpriseValue;

        if(   !JsonFunctions::isJsonFloatValid(residualCashFlow) 
           || !JsonFunctions::isJsonFloatValid(enterpriseValue)){
          if(setNansToMissingValue){
            residualCashFlowToEnterpriseValue = JsonFunctions::MISSING_VALUE;
          }else{
            residualCashFlowToEnterpriseValue = std::nan("1");
          }
        }

        if(appendTermRecord){
          termNames.push_back("residualCashFlowToEnterpriseValue_residualCashFlow");
          termNames.push_back("residualCashFlowToEnterpriseValue");
          termValues.push_back(residualCashFlow);
          termValues.push_back(residualCashFlowToEnterpriseValue);
        }


        //it.c_str(), 
        nlohmann::ordered_json analysisEntry=nlohmann::ordered_json::object();
        analysisEntry.push_back({"date", date});        
        for( unsigned int i=0; i < termNames.size();++i){
          analysisEntry.push_back({termNames[i],
                                   termValues[i]});
        }

        analysis[date]= analysisEntry;        
        ++entryCount;
      }


      std::string outputFilePath(analyseFolder);
      std::string outputFileName(fileName.c_str());    
      outputFilePath.append(outputFileName);

      std::ofstream outputFileStream(outputFilePath,
          std::ios_base::trunc | std::ios_base::out);
      outputFileStream << analysis;
      outputFileStream.close();
    }

    ++count;

    if(loadSingleTicker){
      std::cout << "Done evaluating: " << singleFileToEvaluate << std::endl;
      break;
    }
  }




  //For each file in the list
    //Open it
    //Get fundamentalData["General"]["PrimaryTicker"] and use this to generate an output file name
    //Create a new json file to contain the analysis
    //Call each analysis function individually to analyze the json file and
    //  to write the results of the analysis into the output json file
    //Write the analysis json file 

  return 0;
}
