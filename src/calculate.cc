//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <limits>

#include <boost/math/statistics/linear_regression.hpp>

#include "date.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>
#include <filesystem>

#include "FinancialAnalysisFunctions.h"
#include "NumericalFunctions.h"
#include "JsonFunctions.h"
#include "DateFunctions.h"

//============================================================================
struct AnnualMilestoneDataSet{
  double yearsSinceIPO;
  double yearsOnRecord;
  double yearsOfPositiveValueCreation; //ROIC - CC > 0
  double yearsWithADividend;
  double yearsWithADividendIncrease;
  double lastDividend;
    AnnualMilestoneDataSet():
      yearsSinceIPO(0),
      yearsOnRecord(0),
      yearsOfPositiveValueCreation(0),
      yearsWithADividend(0),
      yearsWithADividendIncrease(0),
      lastDividend(0){};
};

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
struct CountryRiskDataSet{
  std::string Country;
  std::string CountryISO2;
  std::string CountryISO3;
  double PRS;
  double defaultSpread;
  double ERP;
  double taxRate;
  double CRP;
  double inflation_2019_2023;
  double inflation_2024_2028;
  double riskFreeRate;

  CountryRiskDataSet():
    PRS(std::nan("1")),
    defaultSpread(std::nan("1")),
    ERP(std::nan("1")),
    taxRate(std::nan("1")),
    CRP(std::nan("1")),
    inflation_2019_2023(std::nan("1")),
    inflation_2024_2028(std::nan("1")),
    riskFreeRate(std::nan("1")){}
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
    DateFunctions::calcDifferenceInDaysBetweenTwoDates(
                                datesSetA[0],
                                dateAFormat,        
                                datesSetA[datesSetA.size()-1],
                                dateAFormat);


  //Get the indicies of the most recent and oldest indexes
  int firstMinusLastSetB = 
    DateFunctions::calcDifferenceInDaysBetweenTwoDates(
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
        DateFunctions::calcDifferenceInDaysBetweenTwoDates(
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
      DataStructures::AnalysisDates &analysisDates,
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
  analysisDates.earningsHistory.clear();
  analysisDates.outstandingShares.clear();
  analysisDates.historical.clear();
  analysisDates.bond.clear();
  analysisDates.indicesFinancial.clear();
  analysisDates.indicesEarningsHistory.clear();  
  analysisDates.indicesOutstandingShares.clear();
  analysisDates.indicesHistorical.clear();
  analysisDates.indicesBond.clear();
  analysisDates.durationInYears=0.;

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
    for(auto& el : fundamentalData[EARN][HIST].items()){
      std::string date; 
      JsonFunctions::getJsonString(el.value()["date"],date);
      analysisDates.earningsHistory.push_back(date);
    }   
    validDates = (validDates && analysisDates.earningsHistory.size() > 0);
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
      DateFunctions::calcDifferenceInDaysBetweenTwoDates(
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

  
   if(validDates){
      std::vector< std::string > commonAB;
      std::vector< unsigned int> indicesA;

      validDates =     
        extractDatesOfClosestMatch(
          analysisDates.common,
          "%Y-%m-%d",
          analysisDates.earningsHistory,
          "%Y-%m-%d",
          maxDayErrorHistoricalData,
          commonAB,
          indicesA,
          analysisDates.indicesEarningsHistory,
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
            analysisDates.indicesBond.erase(
                analysisDates.indicesBond.begin()+indexA);
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
      
      validDates = validDates 
        && (analysisDates.common.size()
            ==analysisDates.indicesEarningsHistory.size());
    }

    //Go through all of the common dates and mark which ones coincide with
    //the date of the annual report
    if(validDates){

      //Get all of the annual reports
      std::vector< std::string > annualReportDateSet;
      for(auto& el : fundamentalData[FIN][BAL][Y].items() ){
        std::string annualReportDate;
        JsonFunctions::getJsonString(el.value()["date"],annualReportDate);
        annualReportDateSet.push_back(annualReportDate);
      }

      //Go through all of the common dates and see which ones match
      for(size_t i=0; i<analysisDates.common.size();++i){
        bool isAnnualReport=false;
        for(size_t j=0; j<annualReportDateSet.size(); ++j){
          if(analysisDates.common[i].compare(annualReportDateSet[j]) == 0){
            isAnnualReport=true;
            break;
          }
        }
        analysisDates.isAnnualReport.push_back(isAnnualReport);
      }
    }

  }

  if(validDates){
    double dateEnd = 
      DateFunctions::convertToFractionalYear(analysisDates.common[0]);
    double dateStart = 
      DateFunctions::convertToFractionalYear(
          analysisDates.common[analysisDates.common.size()-1]);
    analysisDates.durationInYears = dateEnd-dateStart;
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
double calcAverageTaxRate(  const DataStructures::AnalysisDates &analysisDates,
                            const std::string& countryISO2, 
                            const TaxFoundationDataSet &corpWorldTaxTable,
                            double defaultTaxRate,                            
                            int maxYearErrorTaxRateTable,
                            bool quarterlyTTMAnalysis,
                            int maxDayErrorTTM)
{

  int indexDate                 = -1;
  int numberOfDatesPerIteration = 0;
  int numberOfIterations        = 0;

  double taxRate                            = 0.;
  double meanTaxRate                        = 0.;
  unsigned int taxRateEntryCount            = 0;

  while( (indexDate+1) < analysisDates.common.size()){

    ++indexDate;

    std::string date = analysisDates.common[indexDate]; 
    
    //The set of dates used for the TTM analysis
    //std::vector < std::string > dateSet;
    //std::vector < double > dateSetWeight;
    DateFunctions::DateSetTTM dateSet;

    bool validDateSet = 
      DateFunctions::extractTTM(indexDate,
                              analysisDates.common,
                              "%Y-%m-%d",
                              dateSet,
                              maxDayErrorTTM,
                              quarterlyTTMAnalysis);                                     
    if(!validDateSet){
      break;
    }

                        
    
    ++numberOfIterations;
    numberOfDatesPerIteration += static_cast<int>(dateSet.dates.size());
    
    taxRate=0.0;

    for(unsigned int j=0; j<dateSet.dates.size();++j){
      int year  = std::stoi(dateSet.dates[j].substr(0,4));
      int yearMin = year-maxYearErrorTaxRateTable;
      double taxRateDate = getTaxRateFromTable(countryISO2, year, yearMin, 
                                    corpWorldTaxTable);
      taxRate += taxRateDate*dateSet.weightsNormalized[j];
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
          const DataStructures::AnalysisDates &analysisDates,
          const nlohmann::ordered_json &fundamentalData,
          double defaultInterestCover,
          const std::string &timePeriod,
          bool quarterlyTTMAnalysis,
          int maxDayErrorTTM,
          bool setNansToMissingValue,
          bool appendTermRecord)
{

  int indexDate                       = -1;
  bool validDateSet                   = true;      

  double meanInterestCover            = 0.;
  double meanInterestCoverEntryCount  = 0;


  bool appendTermRecordLocal=false;


  while( (indexDate+1) < analysisDates.common.size() && validDateSet){

    ++indexDate;

    std::string date = analysisDates.common[indexDate]; 
    
    //The set of dates used for the TTM analysis
    //std::vector < std::string > dateSet;
    //std::vector < double > dateSetWeight;
    DateFunctions::DateSetTTM dateSet;
    validDateSet = 
      DateFunctions::extractTTM( indexDate,
                                  analysisDates.common,
                                  "%Y-%m-%d",
                                  dateSet,
                                  maxDayErrorTTM,
                                  quarterlyTTMAnalysis);                                     
    if(!validDateSet){
      break;
    }

    std::vector<std::string> localTermNames;
    std::vector<double> localTermValues;

    double interestCover = FinancialAnalysisFunctions::
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
          const DataStructures::AnalysisDates &analysisDates,
          bool quarterlyTTMAnalysis,
          int maxDayErrorTTM)
{

  int indexDate                    = -1;
  bool validDateSet                = true;      
  double numberOfDatesPerIteration = 0;
  double numberOfIterations        = 0;

  while( (indexDate+1) < analysisDates.common.size() && validDateSet){

    ++indexDate;

    std::string date = analysisDates.common[indexDate]; 
    
    //The set of dates used for the TTM analysis
    //std::vector < std::string > dateSet;
    //std::vector < double > dateSetWeight;
    DateFunctions::DateSetTTM dateSet;

    validDateSet = 
      DateFunctions::extractTTM(indexDate,
                                analysisDates.common,
                                "%Y-%m-%d",
                                dateSet,
                                maxDayErrorTTM,
                                quarterlyTTMAnalysis);                                     
    if(!validDateSet){
      break;
    }

                                  
    ++numberOfIterations;
    numberOfDatesPerIteration += static_cast<int>(dateSet.dates.size());    
  }        
  

  double tmp =  (static_cast<double>(numberOfDatesPerIteration) 
                /static_cast<double>(numberOfIterations));

  numberOfDatesPerIteration =  static_cast<int>( std::round(tmp) );

  --indexDate;
  return indexDate;


};


//============================================================================
double getTaxRate(std::string &date, 
                  const CountryRiskDataSet &riskTable,
                  const std::string& countryISO2, 
                  const TaxFoundationDataSet &corpWorldTaxTable,
                  double meanTaxRate,
                  double defaultTaxRate,
                  int acceptableBackwardsYearErrorForTaxRate,
                  bool usingTaxTable,
                  bool riskTableFound){

  double taxRate=0.;

  bool taxRateTableFound=true;

  if(usingTaxTable){
    int year  = std::stoi(date.substr(0,4));
    int yearMin = year-acceptableBackwardsYearErrorForTaxRate;
    taxRate = getTaxRateFromTable(countryISO2, year, yearMin, 
                                  corpWorldTaxTable);
    taxRate = taxRate*0.01;  //convert from percent to decimal                                     
    if(std::isnan(taxRate)){
      taxRate=meanTaxRate;    
      taxRateTableFound=false;        
    }                    
    if(std::isnan(taxRate)){
      taxRate=defaultTaxRate;
      taxRateTableFound=false;
    }          
  }else{
    taxRate = defaultTaxRate;
    taxRateTableFound=false;
  }        
  
  //Update (if necessary) the tax rate
  if(!taxRateTableFound && riskTableFound 
        && !std::isnan(riskTable.taxRate)){
    taxRate = riskTable.taxRate;
  }    

  return taxRate;
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
  std::string riskByCountryFile;

  std::string nameOfHomeCountryISO3;

  double defaultInterestCover;

  double defaultRiskFreeRate;
  double erpUSADefault;
  double defaultBeta;

  std::string singleFileToEvaluate;

  double defaultTaxRate;
  //double defaultInflationRate;
  int numberOfYearsToAverageCapitalExpenditures;

  int numberOfYearsOfGrowthForDcmValuation;
  int maxDayErrorTabularData;
  bool relaxedCalculation;
  double matureFirmFractionOfDebtCapital=0;

  double discountRate=0.;

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

    //TCLAP::ValueArg<double> defaultInflationRateInput("g",
    //  "default_inflation_rate", 
    //  "The default inflation rate in the country that you are investing in."
    //  " In my case this is 0.0248 (2.48 percent) in Germany.",
    //  false,0.0248,"double");
    //cmd.add(defaultInflationRateInput);      


    TCLAP::ValueArg<std::string> nameOfHomeCountryISO3Input("g",
      "iso3_name_of_home_country", 
      "Name of your home country in ISO3 format (e.g. USA for the"
      " United States of America, DEU for Germany, etc)",
      false,"","string");
    cmd.add(nameOfHomeCountryISO3Input);  


    TCLAP::ValueArg<std::string> corpTaxesWorldFileInput("w","global_corporate_tax_rate_file", 
      "Corporate taxes reported around the world from the tax foundation"
      " in csv format (https://taxfoundation.org/data/all/global/corporate-tax-rates-by-country-2023/)",
      false,"","string");
    cmd.add(corpTaxesWorldFileInput);  

    TCLAP::ValueArg<double> defaultDiscountRateInput("s",
      "discount_rate", 
      "The discount rate used in more basic evaluation methods in which "
      "the cost of capital is not estimated.",
      false,0.10,"double");
    cmd.add(defaultDiscountRateInput);  


    TCLAP::ValueArg<double> defaultRiskFreeRateInput("r",
      "default_risk_free_rate", 
      "The risk free rate of return, which is often set to the return on "
      "a 10 year or 30 year bond as noted from Ch. 3 of Damodran.",
      false,0.025,"double");
    cmd.add(defaultRiskFreeRateInput);  

    TCLAP::ValueArg<double> equityRiskPremiumUSAInput("e",
      "equity_risk_premium", 
      "The extra return that the stock should return given its risk in the USA."
      " During 2024 this is somewhere around 4 percent between 1928 and 2010 as"
      " noted in Ch. 3 Damodran.",
      false,0.05,"double");
    cmd.add(equityRiskPremiumUSAInput);  

    
    TCLAP::ValueArg<std::string> riskByCountryFileInput("k",
      "equity_risk_premium_by_country", 
      "The extra return that the stock should return given its country "
      "dependent risk. This json file is typically "
      "data/equityRiskPremiumByCountry2024.json which has been extracted from"
      "the webpage of Professor Aswath Damodaran (see ctryprem.xlsx):" 
      "https://pages.stern.nyu.edu/~adamodar/New_Home_Page/datafile/ctryprem.html",
      false,"","string");
    cmd.add(riskByCountryFileInput);  

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

    TCLAP::ValueArg<int>numberOfYearsOfGrowthForDcmValuationInput("m",
      "number_of_years_of_growth", 
      "Number of years of growth prior to terminal valuation calculation "
      "in the discounted cash flow valuation."
      " Default value of 5 taken from Ch. 3 of Damodran.",
      false,5,"int");
    cmd.add(numberOfYearsOfGrowthForDcmValuationInput); 


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
    //defaultInflationRate= defaultInflationRateInput.getValue();
    nameOfHomeCountryISO3= nameOfHomeCountryISO3Input.getValue(); 
    corpTaxesWorldFile  = corpTaxesWorldFileInput.getValue();
    discountRate        = defaultDiscountRateInput.getValue();
    defaultRiskFreeRate = defaultRiskFreeRateInput.getValue();
    defaultBeta         = defaultBetaInput.getValue();
    erpUSADefault       = equityRiskPremiumUSAInput.getValue();

    riskByCountryFile=
      riskByCountryFileInput.getValue();


    defaultSpreadJsonFile = defaultSpreadJsonFileInput.getValue();
    bondYieldJsonFile     = bondYieldJsonFileInput.getValue();
    defaultInterestCover  = defaultInterestCoverInput.getValue();

    matureFirmFractionOfDebtCapital = 
      matureFirmFractionOfDebtCapitalInput.getValue();

    numberOfYearsToAverageCapitalExpenditures 
      = numberOfYearsToAverageCapitalExpendituresInput.getValue();              

    numberOfYearsOfGrowthForDcmValuation
      = numberOfYearsOfGrowthForDcmValuationInput.getValue();

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

      //std::cout << "  Default inflation rate" << std::endl;
      //std::cout << "    " << defaultInflationRate << std::endl;
      std::cout << "  Name of home country (ISO3)" << std::endl;
      std::cout << "    " << nameOfHomeCountryISO3 << std::endl;

      std::cout << "  Default tax rate" << std::endl;
      std::cout << "    " << defaultTaxRate << std::endl;

      std::cout << "  Corporate tax rate file from https://taxfoundation.org"
                << std:: endl;
      std::cout << "    " << corpTaxesWorldFile << std::endl;

      std::cout << "  Annual default risk free rate" << std::endl;
      std::cout << "    " << defaultRiskFreeRate << std::endl;

      std::cout << "  Annual default equity risk premium (U.S.A.)" << std::endl;
      std::cout << "    " << erpUSADefault << std::endl;

      std::cout << "  Risk by country" << std::endl;
      std::cout << "    " << riskByCountryFile << std::endl;

      std::cout << "  Default beta value" << std::endl;
      std::cout << "    " << defaultBeta << std::endl;

      std::cout << "  Assumed mature firm fraction of capital from debt" << std::endl;
      std::cout << "    " << matureFirmFractionOfDebtCapital << std::endl;

      std::cout << "  Number of years of growth in the DCM valuation " 
                << std::endl;
      std::cout << "    " << numberOfYearsOfGrowthForDcmValuation 
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

  double dateToday = DateFunctions::getTodaysDate();

  int acceptableBackwardsYearErrorForTaxRate = 5;
  //This defines how far back in time data from the tax table can be taken
  //to approximate the tax of this year.


  int maxDayErrorHistoricalData = maxDayErrorTabularData; 
  // Historical data has a resolution of 1 day
  
  int maxDayErrorBondYieldData  = maxDayErrorTabularData; 
  // Bond yield data has a resolution of 1 month

  int maxDayErrorTTM = maxDayErrorTabularData;

  int maxDayErrorOutstandingShareData = 365; 

  //In extractEmpiricalAfterTaxOperatingIncomeGrowthRates an exponential growth model is fit to
  //data. Such a model requires that the input data (after tax operating income
  // in this case) must be greater than 1. Here a certain proportion of the
  // data is allowed to be less than 1. These values are set to 1, so that
  // the low values (approximately) still influence the fit.
  //
  // 4/4/2025 As the decision of which model to choose is based on the R2
  //          between the model and the original data, it does not really
  //          make sense to set an upper limit on maxProportionOfOutliersInExpModel
  double maxProportionOfOutliersInExpModel = 1.0;

  double minPriceAllowedInPriceModel=2.0;

  // When fitting cyclic models, the period of smallest allowable cycle is 
  // defined by this term
  double minCycleTimeInYears = 1.0;

  // In order to accept a more complex cyclic model, it should provide at least
  // this amount of improvement in R2
  double exponentialModelR2Preference=0.25;

  //This date error is over 1 year to accomodate for firms that only
  //report financial data on an annual basis
  double maxDateErrorInYearsInEmpiricalData = 1.5;

  std::vector< double > peMarketVariationUpperBound(3);
  peMarketVariationUpperBound[0]=10;
  peMarketVariationUpperBound[1]=15;
  peMarketVariationUpperBound[2]=30;

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

  std::vector< std::string > vectorNames;
  std::vector< std::vector< double > > vectorValues;

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
  bool usingTaxTable=false;
  TaxFoundationDataSet corpWorldTaxTable;
  if(corpTaxesWorldFile.length() > 0){
    usingTaxTable=true;
    bool validFormat = 
      loadTaxFoundationDataSet(corpTaxesWorldFile,corpWorldTaxTable);

    if(!validFormat){
      usingTaxTable=false;
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
    std::cout << "    approximate the bond yields for countries that do not"
              << std::endl; 
    std::cout << "    appear in Prof. Damodaran's country risk tables."
              << std::endl;               
    std::cout << "    This will not make sense for a business in a country"
              << std::endl; 
    std::cout << "    with a risk free rate that differs substantially from"    
              << std::endl; 
    std::cout << "    the US."    
              << std::endl; 
    std::cout << std::endl;
  }

  //==========================================================================
  //Load the equity risk premium by country table
  //==========================================================================
  nlohmann::ordered_json riskByCountryData;

  bool validRiskTable = JsonFunctions::loadJsonFile( 
                                riskByCountryFile, 
                                riskByCountryData, 
                                verbose);

  if(validRiskTable && verbose){
    std::cout << std::endl;
    std::cout << "Loaded the equity-risk-premium by country table: "
              << std::endl;
  }                                
  
  if(!validRiskTable && verbose){
    std::cout << std::endl;
    std::cout << "Error: failed to load the equity-risk-premium by country table:"
              << riskByCountryFile 
              << std::endl;
  }                                

  //
  // Get the risk table entry for the home country
  //
  
  double defaultInflationRate = 0.;
  CountryRiskDataSet homeRiskTable;
  bool homeRiskTableFound=false;

  if(validRiskTable){
    for(auto &riskEntry : riskByCountryData){
      std::string erpCountryISO3;
      JsonFunctions::getJsonString(riskEntry["CountryISO3"],
                                      erpCountryISO3);    
      if(nameOfHomeCountryISO3.compare(erpCountryISO3)==0){
          homeRiskTable.CountryISO3=erpCountryISO3;

          JsonFunctions::getJsonString(riskEntry["CountryISO2"],
                                      homeRiskTable.CountryISO2);

          JsonFunctions::getJsonString(riskEntry["Country"],
                                      homeRiskTable.Country);

          homeRiskTable.PRS  
                = JsonFunctions::getJsonFloat(
                    riskEntry["PRS"]);

          homeRiskTable.defaultSpread 
                = JsonFunctions::getJsonFloat(
                    riskEntry["defaultSpread"])*0.01;

          homeRiskTable.ERP 
                = JsonFunctions::getJsonFloat(
                    riskEntry["ERP"])*0.01;

          homeRiskTable.taxRate 
                = JsonFunctions::getJsonFloat(
                    riskEntry["taxRate"])*0.01;

          homeRiskTable.CRP 
                = JsonFunctions::getJsonFloat(
                    riskEntry["CRP"])*0.01;

          homeRiskTable.inflation_2019_2023 
                = JsonFunctions::getJsonFloat(
                    riskEntry["inflation_2019_2023"])*0.01;                   
          homeRiskTable.inflation_2024_2028 
          
                = JsonFunctions::getJsonFloat(
                    riskEntry["inflation_2024_2028"])*0.01;

          homeRiskTable.riskFreeRate 
                = JsonFunctions::getJsonFloat(
                    riskEntry["riskFreeRate"])*0.01;                                                      
          homeRiskTableFound=true;

          break;
      }                                        
    }
  }

  if(homeRiskTableFound){
    if(dateToday >= 2024){
      defaultInflationRate = homeRiskTable.inflation_2024_2028;
    }else{
      defaultInflationRate = homeRiskTable.inflation_2019_2023;
    }
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
          }else{
            std::cout << "    Skipping: could not load fundamental data" << std::endl;     
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
          
          if(!validInput){
            std::cout << "    Skipping: could not load fundamental data" << std::endl;
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
    std::string companyName("");
    if(validInput){
      JsonFunctions::getJsonString(fundamentalData[GEN]["CountryName"],
                                   countryName);
      JsonFunctions::getJsonString(fundamentalData[GEN]["CountryISO"],
                                   countryISO2);
      JsonFunctions::getJsonString(fundamentalData[GEN]["Name"],
                                   companyName);  
      if(verbose){
        std::cout << '\t' << companyName << std::endl;
      }                                

    }

    //==========================================================================
    //Load the (primary) historical (price) file
    //==========================================================================
    nlohmann::ordered_json historicalData;
    if(validInput){
      validInput=JsonFunctions::loadJsonFile(fileName, historicalFolder, 
                                            historicalData, verbose);
      if(!validInput){
        std::cout << "    Skipping: could not load historical data" << std::endl;
      }                                            
    }

    std::vector< std::string > datesBondYields;
    
    DataStructures::AnalysisDates analysisDates;
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

      bool sufficientData=true;
      if(analysisDates.durationInYears < numberOfYearsOfGrowthForDcmValuation){
        sufficientData=false;
      }
      int minNumberOfEntries = 
        std::max(static_cast<int>(
                  std::round(numberOfYearsOfGrowthForDcmValuation*0.5)),
                3);

      if(analysisDates.common.size() < minNumberOfEntries){
        sufficientData=false;
      }

      if(!sufficientData){
        std::cout << "    Skipping: insufficient data. Only "
                  << analysisDates.durationInYears
                  << " years of data available available over "
                  << analysisDates.common.size()
                  << " entries."
                  << std::endl;
      }

      validInput = (validInput && validDates && sufficientData);
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

      std::vector< DateFunctions::DateSetTTM > trailingPastPeriods;
      std::string previousTimePeriod("");
      DateFunctions::DateSetTTM previousDateSet;
      //std::vector< std::string > previousDateSet;
      //std::vector< double > previousDateSetWeight;
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
      if(usingTaxTable){        
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

        //======================================================================
        //Evaluate the equity risk premium for this country.
        //======================================================================
        CountryRiskDataSet riskTable;        
        bool riskTableFound=false;

        double equityRiskPremium=erpUSADefault;
        double inflation = defaultInflationRate;

        if(validRiskTable){
          for(auto &riskEntry : riskByCountryData){
            std::string erpCountryISO2;
            JsonFunctions::getJsonString(riskEntry["CountryISO2"],erpCountryISO2);
            if(countryISO2.compare(erpCountryISO2)==0){

              riskTable.CountryISO2=erpCountryISO2;

              JsonFunctions::getJsonString(riskEntry["CountryISO3"],
                                          riskTable.CountryISO3);

              JsonFunctions::getJsonString(riskEntry["Country"],
                                          riskTable.Country);

              riskTable.PRS  
                    = JsonFunctions::getJsonFloat(
                        riskEntry["PRS"]);

              riskTable.defaultSpread 
                    = JsonFunctions::getJsonFloat(
                        riskEntry["defaultSpread"])*0.01;

              riskTable.ERP 
                    = JsonFunctions::getJsonFloat(
                        riskEntry["ERP"])*0.01;

              riskTable.taxRate 
                    = JsonFunctions::getJsonFloat(
                        riskEntry["taxRate"])*0.01;

              riskTable.CRP 
                    = JsonFunctions::getJsonFloat(
                        riskEntry["CRP"])*0.01;

              riskTable.inflation_2019_2023 
                    = JsonFunctions::getJsonFloat(
                        riskEntry["inflation_2019_2023"])*0.01;                   
              riskTable.inflation_2024_2028 
              
                    = JsonFunctions::getJsonFloat(
                        riskEntry["inflation_2024_2028"])*0.01;

              riskTable.riskFreeRate 
                    = JsonFunctions::getJsonFloat(
                        riskEntry["riskFreeRate"])*0.01;                                                      
              riskTableFound=true;

              break;
            }
          }


        }

      //=======================================================================
      //  Extract empirical growth rates from the time series of
      //  after tax operating income
      //======================================================================= 
      int indexDate = -1;

      std::vector< double > taxRateRecord;

      while( (indexDate+1) < indexLastCommonDate ){
          ++indexDate;

        double taxRate = 
          getTaxRate( analysisDates.common[indexDate],
                      riskTable,
                      countryISO2,
                      corpWorldTaxTable,
                      meanTaxRate,
                      defaultTaxRate,
                      acceptableBackwardsYearErrorForTaxRate,
                      usingTaxTable,
                      riskTableFound);

        taxRateRecord.push_back(taxRate);                      
      }


      //Evaluate the growth rate using all of the data available
      DataStructures::EmpiricalGrowthDataSet empiricalGrowthDataAll;

      double growthIntervalInYears = 
        static_cast<double>(numberOfYearsOfGrowthForDcmValuation);

      double totalNumberOfYearsInDataSet = 
        DateFunctions::convertToFractionalYear(analysisDates.common.front())
      - DateFunctions::convertToFractionalYear(analysisDates.common.back());
            
      double growthIntervalInYearsAll = 
        std::max(totalNumberOfYearsInDataSet-growthIntervalInYears,
                 2.0*growthIntervalInYears);

      bool approximateReinvestmentRate=true;

      DataStructures::EmpiricalGrowthSettings atoiGrowthMdlSettings;


      atoiGrowthMdlSettings.maxDateErrorInDays = maxDayErrorTTM;
      atoiGrowthMdlSettings.growthIntervalInYears = totalNumberOfYearsInDataSet;
      atoiGrowthMdlSettings.maxOutlierProportionInEmpiricalModel 
        = maxProportionOfOutliersInExpModel;
      atoiGrowthMdlSettings.minCycleDurationInYears = minCycleTimeInYears;
      atoiGrowthMdlSettings.exponentialModelR2Preference 
        = exponentialModelR2Preference;
      atoiGrowthMdlSettings.calcOneGrowthRateForAllData=true;
      atoiGrowthMdlSettings.typeOfEmpiricalModel= -1;


      NumericalFunctions::extractEmpiricalAfterTaxOperatingIncomeGrowthRates(
                                  empiricalGrowthDataAll,            
                                  fundamentalData,
                                  taxRateRecord,
                                  analysisDates,
                                  timePeriod,
                                  indexLastCommonDate,
                                  quarterlyTTMAnalysis,
                                  approximateReinvestmentRate,
                                  atoiGrowthMdlSettings);  

      if(empiricalGrowthDataAll.model.size()>0){
        if(empiricalGrowthDataAll.model[0].validFitting){
          atoiGrowthMdlSettings.typeOfEmpiricalModel 
            = empiricalGrowthDataAll.model[0].modelType;
        }
      }
      //Evaluate the growth rate data over the same relatively short
      //period of time that is used for the growth period during
      //the valution (5 years)
      DataStructures::EmpiricalGrowthDataSet empiricalGrowthData;
      

      atoiGrowthMdlSettings.growthIntervalInYears=growthIntervalInYears;
      atoiGrowthMdlSettings.calcOneGrowthRateForAllData=false;

      NumericalFunctions::extractEmpiricalAfterTaxOperatingIncomeGrowthRates(
                                  empiricalGrowthData,            
                                  fundamentalData,
                                  taxRateRecord,
                                  analysisDates,
                                  timePeriod,
                                  indexLastCommonDate,
                                  quarterlyTTMAnalysis,
                                  approximateReinvestmentRate,
                                  atoiGrowthMdlSettings);

      //======================================================================= 
      //
      // Fit an empirical model to the price history
      //
      //=======================================================================
      
      std::vector<double> datesHistorical;
      std::vector<double> priceHistorical;


      std::string dateStr;
      double price;
      for(auto &el : historicalData){
        JsonFunctions::getJsonString(el["date"],dateStr);
        price = JsonFunctions::getJsonFloat(el["adjusted_close"],false);  
        if(price > minPriceAllowedInPriceModel){      
          double dateNumerical = DateFunctions::convertToFractionalYear(dateStr);          
          datesHistorical.push_back(dateNumerical);
          priceHistorical.push_back(price);
        }
      }



      DataStructures::EmpiricalGrowthModel linearPriceModel;
      DataStructures::EmpiricalGrowthModel exponentialPriceModel;
      DataStructures::EmpiricalGrowthModel priceModel;

      bool forceZeroSlope=false;
      NumericalFunctions::fitLinearGrowthModel(
          datesHistorical,priceHistorical,forceZeroSlope,linearPriceModel);

      NumericalFunctions::fitCyclicalModelWithExponentialBaseline(
                                datesHistorical, 
                                priceHistorical,
                                minCycleTimeInYears,
                                maxProportionOfOutliersInExpModel,
                                exponentialPriceModel);


      if(exponentialPriceModel.validFitting && linearPriceModel.validFitting){
       if(exponentialPriceModel.r2 > linearPriceModel.r2){
          NumericalFunctions::fitCyclicalModelWithExponentialBaseline(
                                datesHistorical, 
                                priceHistorical,
                                minCycleTimeInYears,
                                maxProportionOfOutliersInExpModel,
                                priceModel);        
        }else{
          NumericalFunctions::fitCyclicalModelWithLinearBaseline(
                               datesHistorical,
                               priceHistorical,
                               minCycleTimeInYears,
                               forceZeroSlope,
                               priceModel);
        }
      }else{
        if(exponentialPriceModel.validFitting){
          NumericalFunctions::fitCyclicalModelWithExponentialBaseline(
                                datesHistorical, 
                                priceHistorical,
                                minCycleTimeInYears,
                                maxProportionOfOutliersInExpModel,
                                priceModel);

        }else if(linearPriceModel.validFitting){
          NumericalFunctions::fitCyclicalModelWithLinearBaseline(
                               datesHistorical,
                               priceHistorical,
                               minCycleTimeInYears,
                               forceZeroSlope,
                               priceModel);
        }
      }
      
      //=======================================================================
      /*
        Extract the growth rates for the Rule Number 1 investing Big 5 criteria
        from Phil Town's book Rule #1: The simple strategy for successful 
        investing 
        
        1.* roic
              I have a function for this and this is recorded. I do not need to
              do anything
        2.  equity
             eod: Financials:Balance_Sheet:time:totalStockholderEquity
        3.  earnings per share
              Earnings:Annual:epsActual
        4.  sales/gross profit
              Income_Statement:grossProfit
        5.  cash flow
              Cash_Flow:time:freeCashFlow
        
        *Do not need to compute the growth rate of the roic
      */
      //=======================================================================
      DataStructures::MetricGrowthDataSet equityGrowthModel, 
                                              equityGrowthModelAvg;
      //bool includeTimeUnitInAddress = true;
      //calcOneGrowthRateForAllData = false;

      DataStructures::EmpiricalGrowthSettings empGrowthSettings;
      empGrowthSettings.maxDateErrorInDays            = maxDayErrorTTM;
      empGrowthSettings.growthIntervalInYears         = growthIntervalInYears;
      empGrowthSettings.maxOutlierProportionInEmpiricalModel
                      = maxProportionOfOutliersInExpModel;
      empGrowthSettings.minCycleDurationInYears       = minCycleTimeInYears;
      empGrowthSettings.exponentialModelR2Preference 
                      = exponentialModelR2Preference;
      empGrowthSettings.calcOneGrowthRateForAllData   = false;        
      empGrowthSettings.includeTimeUnitInAddress      = true;
      empGrowthSettings.typeOfEmpiricalModel          = -1;

      //I'm not fitting to all of the data any more, because I'm seeing this 
      //common case 
      //
      //- One growth model nicely fits data from the first half of 
      //  companies life
      //- Another growth model nicely fits data from the second half of a 
      //  companies life
      //- No single model fits the entire span well.
      //
      // Given that I care more about most recent data, I'm revising the 
      // average to be 2x longer than the growth interval in years 
      // so 10 years with my current settings

      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        FIN,
        BAL,
        Y,
        "totalStockholderEquity",
        equityGrowthModel,
        empGrowthSettings);

      empGrowthSettings.calcOneGrowthRateForAllData=true;
      empGrowthSettings.growthIntervalInYears = growthIntervalInYearsAll;

      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        FIN,
        BAL,
        Y,
        "totalStockholderEquity",
        equityGrowthModelAvg,
        empGrowthSettings);


      DataStructures::MetricGrowthDataSet epsGrowthModel, 
                                          epsGrowthModelAvg;

      empGrowthSettings.includeTimeUnitInAddress      = false;
      empGrowthSettings.calcOneGrowthRateForAllData   = false;  
      empGrowthSettings.growthIntervalInYears         = growthIntervalInYears;    

      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        EARN,
        ANNUAL,
        "",
        "epsActual",
        epsGrowthModel,
        empGrowthSettings);

      empGrowthSettings.calcOneGrowthRateForAllData=true;      
      empGrowthSettings.growthIntervalInYears      = growthIntervalInYearsAll;    
      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        EARN,
        ANNUAL,
        "",
        "epsActual",
        epsGrowthModelAvg,
        empGrowthSettings);


      DataStructures::MetricGrowthDataSet grossProfitGrowthModel,
                                              grossProfitGrowthModelAvg;

      empGrowthSettings.includeTimeUnitInAddress=true;
      empGrowthSettings.calcOneGrowthRateForAllData=false;      
      empGrowthSettings.growthIntervalInYears         = growthIntervalInYears;    

      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        FIN,
        IS,
        Y,
        "grossProfit",
        grossProfitGrowthModel,
        empGrowthSettings);

      empGrowthSettings.calcOneGrowthRateForAllData=true;      
      empGrowthSettings.growthIntervalInYears        = growthIntervalInYearsAll;    

      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        FIN,
        IS,
        Y,
        "grossProfit",
        grossProfitGrowthModelAvg,
        empGrowthSettings);

      DataStructures::MetricGrowthDataSet fcfGrowthModel, 
                                              fcfGrowthModelAvg;

      empGrowthSettings.includeTimeUnitInAddress=true;
      empGrowthSettings.calcOneGrowthRateForAllData=false;      
      empGrowthSettings.growthIntervalInYears         = growthIntervalInYears;    

      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        FIN,
        CF,
        Y,
        "freeCashFlow",
        fcfGrowthModel,
        empGrowthSettings);

      empGrowthSettings.calcOneGrowthRateForAllData=true;    
      empGrowthSettings.growthIntervalInYears      = growthIntervalInYearsAll;    

      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        FIN,
        CF,
        Y,
        "freeCashFlow",
        fcfGrowthModelAvg,
        empGrowthSettings);

      DataStructures::MetricGrowthDataSet revenueGrowthModel, 
                                              revenueGrowthModelAvg;

      empGrowthSettings.includeTimeUnitInAddress=true;
      empGrowthSettings.calcOneGrowthRateForAllData=false;      
      empGrowthSettings.growthIntervalInYears         = growthIntervalInYears;    

      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        FIN,
        IS,
        Y,
        "totalRevenue",
        revenueGrowthModel,
        empGrowthSettings);

      empGrowthSettings.calcOneGrowthRateForAllData=true;    
      empGrowthSettings.growthIntervalInYears      = growthIntervalInYearsAll;    

      NumericalFunctions::extractFundamentalDataMetricGrowthRates(
        fundamentalData,
        FIN,
        IS,
        Y,
        "totalRevenue",
        revenueGrowthModelAvg,
        empGrowthSettings);

      //MarketCapitalizationSummaryData
      DataStructures::FinancialRatios financialRatios;
      
        NumericalFunctions::extractFinancialRatios(
                            fundamentalData,
                            historicalData,
                            analysisDates,
                            timePeriod,
                            timePeriodOS,
                            maxDayErrorTTM,
                            maxDayErrorOutstandingShareData,
                            quarterlyTTMAnalysis,
                            financialRatios);
      

      //=======================================================================
      //
      // Create and set the first values in annualMilestones
      //
      //=======================================================================
      
      AnnualMilestoneDataSet annualMilestones;

      std::string ipoDate;
      JsonFunctions::getJsonString(
          fundamentalData["General"]["IPODate"],ipoDate);

      double ipoDateNumerical = 
        DateFunctions::convertToFractionalYear(ipoDate);
      double firstRecordDate = 
        DateFunctions::convertToFractionalYear(analysisDates.common.front());
      double lastRecordDate = 
        DateFunctions::convertToFractionalYear(analysisDates.common.back());

      double recentDate = 
        (firstRecordDate > lastRecordDate )? firstRecordDate:lastRecordDate;

      annualMilestones.yearsSinceIPO = recentDate-ipoDateNumerical;
      annualMilestones.yearsOnRecord = std::abs(firstRecordDate-lastRecordDate);



      //=======================================================================
      //
      //  Calculate the metrics for every data entry in the file
      //
      //======================================================================= 

      nlohmann::ordered_json metricAnalysisJson;

      indexDate         = -1;
      bool validDateSet = true;

      while( (indexDate+1) < indexLastCommonDate && validDateSet){

        ++indexDate;
        std::string date = analysisDates.common[indexDate]; 
        double dateDouble = DateFunctions::convertToFractionalYear(date);

        int dateYear = static_cast<int>( std::floor(dateDouble) );

                
        //The set of dates used for the TTM analysis
        //std::vector < std::string > dateSet;
        //std::vector < double > dateSetWeight;
        DateFunctions::DateSetTTM dateSet;
      

        validDateSet = 
          DateFunctions::extractTTM(indexDate,
                                    analysisDates.common,
                                    "%Y-%m-%d",
                                    dateSet,
                                    maxDayErrorTTM,
                                    quarterlyTTMAnalysis); 
          
        if(!validDateSet){
          break;
        }    
         
             
        //======================================================================
        //Update the list of previous time periods
        //======================================================================        

        //Check if we have enough data to get the previous time period
        int indexPrevious = indexDate+static_cast<int>(dateSet.dates.size());        

        //Fetch the previous TTM
        previousTimePeriod = analysisDates.common[indexPrevious];
        previousDateSet.clear();

        validDateSet = DateFunctions::extractTTM(indexPrevious,
                                                  analysisDates.common,
                                                  "%Y-%m-%d",
                                                  previousDateSet,
                                                  maxDayErrorTTM,
                                                  quarterlyTTMAnalysis); 
        if(!validDateSet){
          break;
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

          //std::vector< std::string > pastDateSet;
          //std::vector< double > pastDateSetWeight;
          DateFunctions::DateSetTTM pastDateSet;

          if(validPreviousDateSet){
            if(quarterlyTTMAnalysis 
              || (indexPastPeriods < analysisDates.common.size())){
              validPreviousDateSet = 
                DateFunctions::extractTTM(indexPastPeriods,
                                          analysisDates.common,
                                          "%Y-%m-%d",
                                          pastDateSet,
                                          maxDayErrorTTM,
                                          quarterlyTTMAnalysis);
            }else{
              if(indexPastPeriods >= analysisDates.common.size()){
                validPreviousDateSet=false;
              }
            }           
          }
          if(validPreviousDateSet){
            trailingPastPeriods.push_back(pastDateSet);
            indexPastPeriods += pastDateSet.dates.size();
          }

        }

       
        
        //======================================================================
        //Update the risk premium using the risk table, if it exists
        //======================================================================
        
        if(validRiskTable){
          //Calculate the country-specific equity risk premium
          if(riskTableFound && !std::isnan(riskTable.CRP)){
            equityRiskPremium = riskTable.CRP + erpUSADefault;
          }

          //It would be nice to replace this with a historical table of
          //inflation data by country.
          if(riskTableFound && !std::isnan(riskTable.inflation_2019_2023)
                            && !std::isnan(riskTable.inflation_2024_2028)){

            if(dateYear >= 2024){
              inflation = riskTable.inflation_2024_2028;
            }else{
              inflation = riskTable.inflation_2019_2023;
            }
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
        

        //Calculate the country specific defaultSpread
        if(riskTableFound && !std::isnan(riskTable.riskFreeRate)){
          riskFreeRate = riskTable.riskFreeRate;        
        }

        //======================================================================
        //Evaluate the cost of debt
        //======================================================================
        double interestCover = 
          FinancialAnalysisFunctions::calcInterestCover(
                                        fundamentalData,
                                        dateSet,
                                        defaultInterestCover,
                                        timePeriod.c_str(),
                                        appendTermRecord,
                                        setNansToMissingValue,
                                        termNames,
                                        termValues);

        double defaultSpread = FinancialAnalysisFunctions::
            calcDefaultSpread(fundamentalData,
                              dateSet,
                              timePeriod.c_str(),
                              interestCover,
                              jsonDefaultSpread,
                              appendTermRecord,
                              setNansToMissingValue,
                              termNames,
                              termValues);


        //Calculate the country specific defaultSpread
        if(riskTableFound && !std::isnan(riskTable.defaultSpread)){
          defaultSpread += riskTable.defaultSpread;        
        }

        std::string parentName = "afterTaxCostOfDebt_";
        
        //======================================================================
        //Get the tax rate
        //======================================================================

        double taxRate = 
          getTaxRate( analysisDates.common[indexDate],
                      riskTable,
                      countryISO2,
                      corpWorldTaxTable,
                      meanTaxRate,
                      defaultTaxRate,
                      acceptableBackwardsYearErrorForTaxRate,
                      usingTaxTable,
                      riskTableFound);        
   
        //======================================================================
        //Evaluate the cost of debt
        //======================================================================

        double afterTaxCostOfDebt = (riskFreeRate+defaultSpread)*(1.0-taxRate);

        //Adjust for inflation
        //Using Table 31 from Damodaran, "Country risk: determinants, measures
        //and implications - The 2024 Edition".

        //afterTaxCostOfDebt = 
        //(1.0 + afterTaxCostOfDebt)*(
        //    (1.0+inflation)/(1.0+defaultInflationRate))
        //    -1.0;



        termNames.push_back("afterTaxCostOfDebt_riskFreeRate");
        termNames.push_back("afterTaxCostOfDebt_defaultSpread");
        termNames.push_back("afterTaxCostOfDebt_taxRate");  
        termNames.push_back("afterTaxCostOfDebt");
        
        termValues.push_back(riskFreeRate);
        termValues.push_back(defaultSpread);
        termValues.push_back(taxRate);
        termValues.push_back(afterTaxCostOfDebt);

        //======================================================================
        //Evaluate short, long, and total debt
        //======================================================================
        DataStructures::DebtInfo debtInfo, previousDebtInfo;
        std::string debtParentName = "debt_";
        std::string previousDebtParentName = "previousDebt_";

        FinancialAnalysisFunctions::getDebtInfo(fundamentalData,
                                                timePeriod.c_str(),
                                                dateSet.dates[0].c_str(),
                                                debtInfo,
                                                appendTermRecord,
                                                debtParentName,
                                                termNames,
                                                termValues,
                                                setNansToMissingValue);
        
        FinancialAnalysisFunctions::getDebtInfo(fundamentalData,
                                                timePeriod.c_str(),
                                                previousDateSet.dates[0].c_str(),
                                                previousDebtInfo,
                                                appendTermRecord,
                                                previousDebtParentName,
                                                termNames,
                                                termValues,
                                                setNansToMissingValue);
        
        //======================================================================
        //Evaluate the current total short and long term debt
        //======================================================================
        //double longTermDebt = 
        //  JsonFunctions::getJsonFloat(
        //    fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
        //                   ["longTermDebt"]);


        //======================================================================        
        //Evaluate the current market capitalization
        //======================================================================
        double outstandingShares = 
          FinancialAnalysisFunctions::getOutstandingSharesClosestToDate(
              fundamentalData, 
              date,
              timePeriodOS.c_str());
        /*
        double outstandingShares = std::nan("1");
        int smallestDateDifference=std::numeric_limits<int>::max();        
        std::string closestDate("");
        for(auto& el : fundamentalData[OS][timePeriodOS.c_str()]){
          std::string dateOS("");
          JsonFunctions::getJsonString(el["dateFormatted"],dateOS);         
          int dateDifference = 
            DateFunctions::calcDifferenceInDaysBetweenTwoDates(
              date,"%Y-%m-%d",dateOS,"%Y-%m-%d");
          if(std::abs(dateDifference)<smallestDateDifference){
            closestDate = dateOS;
            smallestDateDifference=std::abs(dateDifference);
            outstandingShares = JsonFunctions::getJsonFloat(el["shares"]);
          }
        }
        */

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
          (1.0-taxRate)*(debtInfo.longTermDebtEstimate/marketCapitalization));


        double annualCostOfEquityAsAPercentage = 
          riskFreeRate + equityRiskPremium*beta;

        //Adjust for inflation
        //Using Table 31 from Damodaran, "Country risk: determinants, measures
        //and implications - The 2024 Edition".
        double costOfEquityAsAPercentageNoInflation = 
                  annualCostOfEquityAsAPercentage; 

        annualCostOfEquityAsAPercentage = 
          (1.0+annualCostOfEquityAsAPercentage)
          *((1.0+inflation)/(1.0+defaultInflationRate)) - 1.0;

         double costOfEquityAsAPercentage=annualCostOfEquityAsAPercentage;
        //if(quarterlyTTMAnalysis){
        //  costOfEquityAsAPercentage = costOfEquityAsAPercentage/4.0;
        //}        

        termNames.push_back("costOfEquityAsAPercentage_riskFreeRate");
        termNames.push_back("costOfEquityAsAPercentage_equityRiskPremium");
        termNames.push_back("costOfEquityAsAPercentage_betaUnlevered");
        termNames.push_back("costOfEquityAsAPercentage_taxRate");
        termNames.push_back("costOfEquityAsAPercentage_inflationReference");
        termNames.push_back("costOfEquityAsAPercentage_inflation");          
        termNames.push_back("costOfEquityAsAPercentage_longTermDebt");
        termNames.push_back("costOfEquityAsAPercentage_adjustedClose");
        termNames.push_back("costOfEquityAsAPercentage_outstandingShares");
        termNames.push_back("costOfEquityAsAPercentage_marketCapitalization");
        termNames.push_back("costOfEquityAsAPercentage_beta");
        termNames.push_back("costOfEquityAsAPercentage_noInflation");
        termNames.push_back("costOfEquityAsAPercentage");

        termValues.push_back(riskFreeRate);
        termValues.push_back(equityRiskPremium);
        termValues.push_back(betaUnlevered);
        termValues.push_back(taxRate);
        termValues.push_back(defaultInflationRate);
        termValues.push_back(inflation);
        termValues.push_back(debtInfo.longTermDebtEstimate);
        termValues.push_back(adjustedClosePrice);
        termValues.push_back(outstandingShares);
        termValues.push_back(marketCapitalization);
        termValues.push_back(beta);
        termValues.push_back(costOfEquityAsAPercentageNoInflation);
        termValues.push_back(costOfEquityAsAPercentage);



        //======================================================================        
        //Evaluate a weighted cost of capital
        //======================================================================        

        double costOfCapital = 
          (costOfEquityAsAPercentage*marketCapitalization
          +afterTaxCostOfDebt*debtInfo.longTermDebtEstimate)
          /(marketCapitalization+debtInfo.longTermDebtEstimate);


        termNames.push_back("costOfCapital_longTermDebt");
        termNames.push_back("costOfCapital_outstandingShares");
        termNames.push_back("costOfCapital_adjustedClose");
        termNames.push_back("costOfCapital_marketCapitalization");
        termNames.push_back("costOfCapital_costOfEquityAsAPercentage");
        termNames.push_back("costOfCapital_afterTaxCostOfDebt");
        termNames.push_back("costOfCapital");

        termValues.push_back(debtInfo.longTermDebtEstimate);
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
        // Write some of the financial ratios
        //======================================================================
        double dateRecent = 
          DateFunctions::convertToFractionalYear(dateSet.dates[0]);
        bool validFinancialRatios = financialRatios.datesNumerical.size() > 0;

      
        if(validFinancialRatios){
          int idxFR = DateFunctions::getIndexClosestToDate(dateRecent,
                                      financialRatios.datesNumerical);

          termNames.push_back("financialRatios_dividendYield");
          termNames.push_back("financialRatios_operationalLeverage");
          termNames.push_back("financialRatios_pe");
          termValues.push_back(financialRatios.dividendYield[idxFR]);
          termValues.push_back(financialRatios.operationalLeverage[idxFR]);
          termValues.push_back(financialRatios.pe[idxFR]);  
                                            
        }
        //======================================================================
        //Evaluate the metrics
        //  At the moment residual cash flow and the company's valuation are
        //  most of interest. The remaining metrics are useful, however, and so,
        //  have been included.
        //======================================================================        
        std::string emptyParentName("");

        double totalStockHolderEquity =  
          FinancialAnalysisFunctions::sumFundamentalDataOverDates(
            fundamentalData,FIN,BAL,timePeriod.c_str(),dateSet,
            "totalStockholderEquity", setNansToMissingValue);

        double roicOp = FinancialAnalysisFunctions::
          calcReturnOnInvestedOperatingCapital(
              fundamentalData,
              dateSet,
              timePeriod.c_str(),
              taxRate,
              appendTermRecord, 
              emptyParentName,
              setNansToMissingValue,
              termNames, 
              termValues);
        double roicOpLessCostOfCapital = roicOp - costOfCapitalMature;  
        termNames.push_back("returnOnInvestedOperatingCapitalLessCostOfCapital");
        termValues.push_back(roicOpLessCostOfCapital); 




        double roce = FinancialAnalysisFunctions::
          calcReturnOnCapitalDeployed(  fundamentalData,
                                        dateSet,
                                        timePeriod.c_str(), 
                                        appendTermRecord, 
                                        emptyParentName,
                                        setNansToMissingValue,
                                        termNames, 
                                        termValues);

        double grossMargin = FinancialAnalysisFunctions::
          calcGrossMargin(  fundamentalData,
                            dateSet,
                            timePeriod.c_str(),
                            appendTermRecord,
                            setNansToMissingValue,
                            termNames,
                            termValues);

        double operatingMargin = FinancialAnalysisFunctions::
          calcOperatingMargin(  fundamentalData,
                                dateSet,
                                timePeriod.c_str(), 
                                appendTermRecord,
                                setNansToMissingValue,
                                termNames,
                                termValues);          

        double cashConversion = FinancialAnalysisFunctions::
          calcCashConversionRatio(  fundamentalData,
                                    dateSet,
                                    timePeriod.c_str(), 
                                    taxRate,
                                    appendTermRecord,
                                    setNansToMissingValue,
                                    termNames,
                                    termValues);

        double debtToCapital = FinancialAnalysisFunctions::
          calcDebtToCapitalizationRatio(  fundamentalData,
                                          dateSet,
                                          timePeriod.c_str(),
                                          debtInfo,
                                          appendTermRecord,
                                          setNansToMissingValue,
                                          termNames,
                                          termValues);

        double ownersEarnings = FinancialAnalysisFunctions::
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

          residualCashFlow = FinancialAnalysisFunctions::
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
        //
        //Residual cash flow to enterprise value
        //
        double enterpriseValue = FinancialAnalysisFunctions::
            calcEnterpriseValue(fundamentalData, 
                                marketCapitalization, 
                                dateSet,
                                timePeriod.c_str(),
                                debtInfo,
                                appendTermRecord,                                
                                emptyParentName,
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
          termNames.push_back("residualCashFlowToEnterpriseValue");
          termValues.push_back(residualCashFlowToEnterpriseValue);
        }        

        double freeCashFlowToEquity=std::nan("1");
        if(previousTimePeriod.length()>0){
          freeCashFlowToEquity = FinancialAnalysisFunctions::
            calcFreeCashFlowToEquity(fundamentalData, 
                                     dateSet,
                                     previousDateSet,
                                     timePeriod.c_str(),
                                     debtInfo,
                                     previousDebtInfo,
                                     appendTermRecord,
                                     setNansToMissingValue,
                                     termNames,
                                     termValues);
        }

        double freeCashFlowToFirm=std::nan("1");
        freeCashFlowToFirm = FinancialAnalysisFunctions::
          calcFreeCashFlowToFirm(fundamentalData, 
                                 dateSet, 
                                 previousDateSet, 
                                 timePeriod.c_str(),
                                 taxRate,
                                 appendTermRecord,
                                 setNansToMissingValue,
                                 termNames, 
                                 termValues);


        double retentionRatio = FinancialAnalysisFunctions::
            calcRetentionRatio(
                                 fundamentalData, 
                                 dateSet, 
                                 timePeriod.c_str(),
                                 appendTermRecord,
                                 emptyParentName,
                                 setNansToMissingValue,
                                 termNames, 
                                 termValues);
        
        double returnOnEquity = FinancialAnalysisFunctions::
            calcReturnOnEquity(
                                fundamentalData, 
                                dateSet, 
                                timePeriod.c_str(),
                                appendTermRecord,
                                emptyParentName,
                                setNansToMissingValue,
                                termNames, 
                                termValues);

        double netIncomeGrowth = retentionRatio*returnOnEquity;                                

        if(appendTermRecord){
          termNames.push_back("netIncomeGrowth");
          termValues.push_back(netIncomeGrowth);
        }

        //
        // Discounted Cashflow Valuation (using finanical inputs)
        //

        
        double returnOnInvestedCapitalFinanical 
                = FinancialAnalysisFunctions::
                          calcReturnOnInvestedFinancialCapital(
                            fundamentalData,
                            dateSet,
                            timePeriod.c_str(),
                            taxRate,
                            debtInfo,
                            appendTermRecord, 
                            emptyParentName,
                            setNansToMissingValue,
                            termNames, 
                            termValues);

        double roicFinLessCostOfCapital = 
                returnOnInvestedCapitalFinanical 
                - costOfCapitalMature;  
        termNames.push_back("returnOnInvestedFinancialCapitalLessCostOfCapital");
        termValues.push_back(roicFinLessCostOfCapital);                                    

        double reinvestmentRate = 
                FinancialAnalysisFunctions::
                      calcReinvestmentRate( fundamentalData,
                                            dateSet,
                                            previousDateSet,
                                            timePeriod.c_str(),
                                            taxRate,
                                            appendTermRecord,
                                            emptyParentName,
                                            setNansToMissingValue,
                                            termNames,
                                            termValues);

        double afterTaxOperatingIncomeGrowth=
                reinvestmentRate
                *returnOnInvestedCapitalFinanical;

        termNames.push_back("afterTaxOperatingIncomeGrowth");
        termValues.push_back(afterTaxOperatingIncomeGrowth); 

        //William Price's shareholder yield
        parentName = "shareHolderYield_";
        double shareHolderYield =  
                FinancialAnalysisFunctions::
                  calcShareholderYield( fundamentalData, 
                                        historicalData,
                                        dateSet,
                                        previousDateSet,
                                        timePeriod.c_str(), 
                                        timePeriodOS.c_str(),
                                        debtInfo,
                                        previousDebtInfo,
                                        costOfCapital,
                                        appendTermRecord,                                      
                                        parentName,
                                        setNansToMissingValue,
                                        termNames,
                                        termValues);

        parentName="priceToValue_";

    
        //Valuation (discounted cash flow)
        double presentValue = FinancialAnalysisFunctions::
            calcPriceToValueUsingDiscountedCashflowModel(  
              fundamentalData,
              dateSet,
              timePeriod.c_str(),
              debtInfo,
              riskFreeRate,
              costOfCapital,
              costOfCapitalMature,
              taxRate,
              afterTaxOperatingIncomeGrowth,
              reinvestmentRate,
              returnOnInvestedCapitalFinanical,
              marketCapitalization,
              numberOfYearsOfGrowthForDcmValuation,
              appendTermRecord,
              setNansToMissingValue,
              parentName,
              termNames,
              termValues);

        //
        // Empirical - recent average 
        //

        if(empiricalGrowthData.dates.size() > 0){
          size_t indexGrowth = 
          NumericalFunctions::getIndexOfEmpiricalGrowthDataSet(
                                dateDouble,
                                maxDateErrorInYearsInEmpiricalData,
                                empiricalGrowthData);

          if(appendTermRecord 
            && indexGrowth < empiricalGrowthData.datesNumerical.size()){
            std::string nameToPrepend("atoiEmpirical_");

            NumericalFunctions::appendEmpiricalAfterTaxOperatingIncomeGrowthDataSet(
                  indexGrowth,      
                  empiricalGrowthData,
                  dateDouble,
                  costOfCapitalMature,
                  nameToPrepend,
                  termNames,
                  termValues);
          }


          if(indexGrowth < empiricalGrowthData.datesNumerical.size()){
            parentName="priceToValueEmpirical_";

            double roicEmp = 
              empiricalGrowthData.afterTaxOperatingIncomeGrowth[indexGrowth]
              /empiricalGrowthData.reinvestmentRate[indexGrowth];

            //Valuation (discounted cash flow) using empirical growth
            double presentValueEmpirical = 
            FinancialAnalysisFunctions::
                calcPriceToValueUsingDiscountedCashflowModel(  
                  fundamentalData,
                  dateSet,
                  timePeriod.c_str(),
                  debtInfo,
                  riskFreeRate,
                  costOfCapital,
                  costOfCapitalMature,
                  taxRate,
                  empiricalGrowthData.afterTaxOperatingIncomeGrowth[indexGrowth],
                  empiricalGrowthData.reinvestmentRate[indexGrowth],
                  roicEmp,
                  marketCapitalization,
                  numberOfYearsOfGrowthForDcmValuation,
                  appendTermRecord,
                  setNansToMissingValue,
                  parentName,
                  termNames,
                  termValues);
          }
        }


        //
        // Empirical - average all time years
        //
        if(empiricalGrowthDataAll.dates.size() == 1){   

          if(appendTermRecord 
              && empiricalGrowthDataAll.datesNumerical.size()==1){
            std::string nameToPrepend("atoiEmpiricalAvg_");

            NumericalFunctions::appendEmpiricalAfterTaxOperatingIncomeGrowthDataSet(
                                  0,      
                                  empiricalGrowthDataAll,
                                  dateDouble,
                                  costOfCapitalMature,
                                  nameToPrepend,
                                  termNames,
                                  termValues);
          }

          if(empiricalGrowthDataAll.datesNumerical.size()==1){
            double roicEmpAvg = 
              empiricalGrowthDataAll.afterTaxOperatingIncomeGrowth[0]
              /empiricalGrowthDataAll.reinvestmentRate[0];

            parentName="priceToValueEmpiricalAvg_";

            //Valuation (discounted cash flow) using empirical growth
            double presentValueEmpiricalAvg = 
              FinancialAnalysisFunctions::
                calcPriceToValueUsingDiscountedCashflowModel(  
                  fundamentalData,
                  dateSet,
                  timePeriod.c_str(),
                  debtInfo,
                  riskFreeRate,
                  costOfCapital,
                  costOfCapitalMature,
                  taxRate,
                  empiricalGrowthDataAll.afterTaxOperatingIncomeGrowth[0],
                  empiricalGrowthDataAll.reinvestmentRate[0],
                  roicEmpAvg,                
                  marketCapitalization,   
                  numberOfYearsOfGrowthForDcmValuation,
                  appendTermRecord,
                  setNansToMissingValue,
                  parentName,
                  termNames,
                  termValues);
          }
        }

        //
        // Price-to-Value using EPS and EPS growth
        //
        parentName="priceToValueEpsGrowth_";
        NumericalFunctions::
          calcPriceToValueUsingEarningsPerShareGrowth(
            dateSet,
            equityGrowthModel,
            financialRatios,
            peMarketVariationUpperBound,
            discountRate,
            numberOfYearsOfGrowthForDcmValuation,
            appendTermRecord,
            setNansToMissingValue,
            parentName,
            termNames,
            termValues);  


        //
        // Price-to-Value using free-cash-flow per share and
        // free-cash-flow yield 
        //


        //
        // Equity growth
        //
        NumericalFunctions::appendMetricGrowthDataSet(
              dateDouble,              
              equityGrowthModel,
              std::string("equityEmpiricalModel_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);

        NumericalFunctions::appendMetricGrowthDataSetRecentDate(
              equityGrowthModelAvg,
              std::string("equityEmpiricalModelAvg_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);

        //
        // Earnings per share growth
        //
        NumericalFunctions::appendMetricGrowthDataSet(
              dateDouble,              
              epsGrowthModel,
              std::string("epsEmpiricalModel_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);

        NumericalFunctions::appendMetricGrowthDataSetRecentDate(
              epsGrowthModelAvg,
              std::string("epsEmpiricalModelAvg_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);              

        //
        // Gross profit growth
        //
        NumericalFunctions::appendMetricGrowthDataSet(
              dateDouble,              
              grossProfitGrowthModel,
              std::string("grossProfitEmpiricalModel_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);

        NumericalFunctions::appendMetricGrowthDataSetRecentDate(
              grossProfitGrowthModelAvg,
              std::string("grossProfitEmpiricalModelAvg_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);
        //
        // Free cash flow
        //
        NumericalFunctions::appendMetricGrowthDataSet(
              dateDouble,              
              fcfGrowthModel,
              std::string("fcfEmpiricalModel_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);

        NumericalFunctions::appendMetricGrowthDataSetRecentDate(
              fcfGrowthModelAvg,
              std::string("fcfEmpiricalModelAvg_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);
        //
        // Sales
        //
        NumericalFunctions::appendMetricGrowthDataSet(
              dateDouble,              
              revenueGrowthModel,
              std::string("revenueEmpiricalModel_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);

        NumericalFunctions::appendMetricGrowthDataSetRecentDate(
              revenueGrowthModelAvg,
              std::string("revenueEmpiricalModelAvg_"),
              maxDateErrorInYearsInEmpiricalData,
              termNames,
              termValues);

        //revenueGrowthModel
        //revenueGrowthModelAvg              
        //
        //
        //
        nlohmann::ordered_json analysisEntry=nlohmann::ordered_json::object();
        analysisEntry.push_back({"date", date});        
        for( unsigned int i=0; i < termNames.size();++i){
          analysisEntry.push_back({termNames[i],
                                   termValues[i]});
        }

        //
        //annualMilestones
        //
        if(analysisDates.isAnnualReport[indexDate]){

          double excessReturnOnInvestedCapital = 
            returnOnInvestedCapitalFinanical-costOfCapital;

          if((excessReturnOnInvestedCapital)>0){
            ++annualMilestones.yearsOfPositiveValueCreation;
          }

          double dividendsPaid = 
            JsonFunctions::getJsonFloat(
              fundamentalData[FIN][CF][Y][date.c_str()]["dividendsPaid"],false);
          if(std::isnan(dividendsPaid)){
            dividendsPaid = 0;
          }
          if(dividendsPaid > 0){
            ++annualMilestones.yearsWithADividend;
          }
          if(dividendsPaid > annualMilestones.lastDividend){
            ++annualMilestones.yearsWithADividendIncrease;
          }
          annualMilestones.lastDividend=dividendsPaid;
        }



        

        metricAnalysisJson[date]= analysisEntry;        
        ++entryCount;
      }


      nlohmann::ordered_json stringDataReport;
      stringDataReport["home_country"] = homeRiskTable.CountryISO3;
      stringDataReport["firm_country"] = riskTable.CountryISO3;

      analysis["country_data"] = stringDataReport;


      nlohmann::ordered_json annualMilestoneReport;
      annualMilestoneReport["years_since_IPO"] 
        = annualMilestones.yearsSinceIPO;
      annualMilestoneReport["years_reported"] 
        = annualMilestones.yearsOnRecord;
      annualMilestoneReport["years_value_created"] 
        = annualMilestones.yearsOfPositiveValueCreation;
      annualMilestoneReport["years_with_dividend"] 
        = annualMilestones.yearsWithADividend;
      annualMilestoneReport["years_with_dividend_increase"] 
        = annualMilestones.yearsWithADividendIncrease;

      analysis["annual_milestones"] = annualMilestoneReport;


             
      //if(empiricalGrowthData.datesNumerical.size()>0){
      //  double dateRange= empiricalGrowthData.datesNumerical.front()
      //                  -empiricalGrowthData.datesNumerical.back(); 
      //  if(dateRange < 0){
      //    indexRecentDate = empiricalGrowthData.datesNumerical.size()-1;
      //  }
      //  
      //  dateRange= empiricalGrowthDataAll.datesNumerical.front()
      //            -empiricalGrowthDataAll.datesNumerical.back(); 
      //  if(dateRange < 0){
      //    indexRecentDateAll = empiricalGrowthDataAll.datesNumerical.size()-1;
      //  }
      //}

      //
      // growth of equity
      //   equityGrowthModel   
      //   equityGrowthModelAvg
      //

      nlohmann::ordered_json equityGrowthModelJson;
      NumericalFunctions::appendMetricGrowthModelRecent(
        equityGrowthModelJson,
        equityGrowthModel,"");

      nlohmann::ordered_json equityGrowthModelAvgJson;
      if(equityGrowthModelAvg.datesNumerical.size() > 0){        
        NumericalFunctions::appendEmpiricalGrowthModelRecent(
            equityGrowthModelAvgJson,
            equityGrowthModelAvg.model[0],"");
      }

      //
      // growth of earnings per share
      //    epsGrowthModel
      //    epsGrowthModelAvg

      nlohmann::ordered_json epsGrowthModelJson;
      NumericalFunctions::appendMetricGrowthModelRecent(
        epsGrowthModelJson,
        epsGrowthModel,"");

      nlohmann::ordered_json epsGrowthModelAvgJson;
      if(epsGrowthModelAvg.datesNumerical.size() > 0){
        NumericalFunctions::appendEmpiricalGrowthModelRecent(
            epsGrowthModelAvgJson,
            epsGrowthModelAvg.model[0],"");
      }

      //
      // growth of gross profit
      //    grossProfitGrowthModel
      //    grossProfitGrowthModelAvg

      nlohmann::ordered_json grossProfitGrowthModelJson;
      NumericalFunctions::appendMetricGrowthModelRecent(
        grossProfitGrowthModelJson,
        grossProfitGrowthModel,"");

      nlohmann::ordered_json grossProfitGrowthModelAvgJson;
      if(grossProfitGrowthModelAvg.datesNumerical.size() > 0){
        NumericalFunctions::appendEmpiricalGrowthModelRecent(
            grossProfitGrowthModelAvgJson,
            grossProfitGrowthModelAvg.model[0],"");
      }


      //
      // growth of free cash flow
      //    fcfGrowthModel 
      //    fcfGrowthModelAvg

      nlohmann::ordered_json fcfGrowthModelJson;
      NumericalFunctions::appendMetricGrowthModelRecent(
        fcfGrowthModelJson,
        fcfGrowthModel,"");

      nlohmann::ordered_json fcfGrowthModelAvgJson;
      if(fcfGrowthModelAvg.datesNumerical.size() > 0){
        NumericalFunctions::appendEmpiricalGrowthModelRecent(
            fcfGrowthModelAvgJson,
            fcfGrowthModelAvg.model[0],"");
      }

      //
      // growth of revenue
      //    revenueGrowthModel 
      //    revenueGrowthModelAvg

      nlohmann::ordered_json revenueGrowthModelJson;
      NumericalFunctions::appendMetricGrowthModelRecent(
        revenueGrowthModelJson,
        revenueGrowthModel,"");

      nlohmann::ordered_json revenueGrowthModelAvgJson;
      if(revenueGrowthModelAvg.datesNumerical.size() > 0){
        NumericalFunctions::appendEmpiricalGrowthModelRecent(
            revenueGrowthModelAvgJson,
            revenueGrowthModelAvg.model[0],"");
      }      

      //
      // Avg empirical model
      //

      nlohmann::ordered_json atoiGrowthModelAverageJson;
      if(empiricalGrowthDataAll.datesNumerical.size() > 0){
        NumericalFunctions::appendEmpiricalGrowthModelRecent(
            atoiGrowthModelAverageJson,
            empiricalGrowthDataAll,"");
      }

      //
      // Recent empirical data
      //

      nlohmann::ordered_json atoiGrowthModelRecentJson;      
      if(empiricalGrowthData.datesNumerical.size()>0){
        NumericalFunctions::appendEmpiricalGrowthModelRecent(
            atoiGrowthModelRecentJson,
            empiricalGrowthData,"");
      }

      //
      // Pricing model
      //
            
      //Downsample all outgoing data.
      //Each year gets 12 data points + the last entry gets added
      nlohmann::ordered_json priceGrowthModelJson;      
      if(priceModel.x.size()>0){
        DataStructures::EmpiricalGrowthModel priceModelDS;
        int previousMonth=0;
        double previousDate=0.;

        //Copy over all the model meta-data
        priceModelDS.modelType                   = priceModel.modelType                  ;  
        priceModelDS.duration                    = priceModel.duration                   ;   
        priceModelDS.annualGrowthRateOfTrendline = priceModel.annualGrowthRateOfTrendline;   
        priceModelDS.r2                          = priceModel.r2                         ;   
        priceModelDS.r2Trendline                 = priceModel.r2Trendline                ;  
        priceModelDS.r2Cyclic                    = priceModel.r2Cyclic                   ;  
        priceModelDS.validFitting                = priceModel.validFitting               ;     
        priceModelDS.outlierCount                = priceModel.outlierCount               ; 
        priceModelDS.parameters                  = priceModel.parameters                 ;   


        for(size_t i=0; i<priceModel.x.size();++i){
          int month = std::floor( (            priceModel.x[i]
                                  -std::floor(priceModel.x[i]))*12.0);
          if(month != previousMonth){
            priceModelDS.x.push_back(
                priceModel.x[i]);
            priceModelDS.y.push_back(
                priceModel.y[i]);
            priceModelDS.yTrendline.push_back(
                priceModel.yTrendline[i]);
            priceModelDS.yCyclic.push_back(
                priceModel.yCyclic[i]);
            priceModelDS.yCyclicData.push_back(
                priceModel.yCyclicData[i]);
            priceModelDS.yCyclicNorm.push_back(
                priceModel.yCyclicNorm[i]);
            priceModelDS.yCyclicNormData.push_back(
                priceModel.yCyclicNormData[i]);
            priceModelDS.yCyclicNormDataPercentiles.push_back(
                priceModel.yCyclicNormDataPercentiles[i]);
            previousDate = priceModel.x[i];
          }  
          previousMonth=month;                               
        }

        if(priceModel.x.back() !=  previousDate){
            priceModelDS.x.push_back(
                priceModel.x.back());
            priceModelDS.y.push_back(
                priceModel.y.back());
            priceModelDS.yTrendline.push_back(
                priceModel.yTrendline.back());
            priceModelDS.yCyclic.push_back(
                priceModel.yCyclic.back());    
            priceModelDS.yCyclicData.push_back(
                priceModel.yCyclicData.back());
            priceModelDS.yCyclicNorm.push_back(
                priceModel.yCyclicNorm.back());
            priceModelDS.yCyclicNormData.push_back(
                priceModel.yCyclicNormData.back());
            priceModelDS.yCyclicNormDataPercentiles.push_back(
                priceModel.yCyclicNormDataPercentiles.back());
        }


        NumericalFunctions::appendEmpiricalGrowthModelRecent(
            priceGrowthModelJson,
            priceModelDS,"");
      }
      //
      // Package all three into a single json object
      //

      analysis["metric_data"]                     = metricAnalysisJson;
      analysis["equity_growth_model_avg"]         = equityGrowthModelAvgJson;
      analysis["equity_growth_model_recent"]      = equityGrowthModelJson;
      analysis["eps_growth_model_avg"]            = epsGrowthModelAvgJson;
      analysis["eps_growth_model_recent"]         = epsGrowthModelJson;
      analysis["grossProfit_growth_model_avg"]    = grossProfitGrowthModelAvgJson;
      analysis["grossProfit_growth_model_recent"] = grossProfitGrowthModelJson;
      analysis["fcf_growth_model_avg"]            = fcfGrowthModelAvgJson;
      analysis["fcf_growth_model_recent"]         = fcfGrowthModelJson;
      analysis["revenue_growth_model_avg"]        = revenueGrowthModelAvgJson;
      analysis["revenue_growth_model_recent"]     = revenueGrowthModelJson;

      analysis["atoi_growth_model_avg"]     = atoiGrowthModelAverageJson;
      analysis["atoi_growth_model_recent"]  = atoiGrowthModelRecentJson;
      analysis["price_growth_model"]        = priceGrowthModelJson;


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
