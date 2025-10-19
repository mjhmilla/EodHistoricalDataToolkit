//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef DATE_FUNCTIONS
#define DATE_FUNCTIONS

#include <cmath>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <chrono>

#include "date.h"



class DateFunctions {

  public:
    static constexpr double DAYS_PER_YEAR = 365.25;

  //==============================================================================
    static double convertToFractionalYear(const std::string &dateStr){
      double date = std::nan("1");

      if(dateStr.size() > 0){
        date::year_month_day dateYmd;
        date::sys_days dateDay, dateDayFirstOfYear, dateDayLastOfYear;

        std::istringstream dateStrStreamA(dateStr);
        dateStrStreamA.exceptions(std::ios::failbit);
        dateStrStreamA >> date::parse("%Y-%m-%d",dateDay);

        std::istringstream dateStrStreamB(dateStr);
        dateStrStreamB.exceptions(std::ios::failbit);
        dateStrStreamB >> date::parse("%Y-%m-%d",dateYmd);

        int year = int(dateYmd.year());
        std::string dateStrFirstOfYear(dateStr.substr(0,4));
        dateStrFirstOfYear.append("-01-01");

        std::istringstream dateStrStreamC(dateStrFirstOfYear);
        dateStrStreamC.exceptions(std::ios::failbit);      
        dateStrStreamC >> date::parse("%Y-%m-%d",dateDayFirstOfYear);

        std::string dateStrLastOfYear(dateStr.substr(0,4));
        dateStrLastOfYear.append("-12-31");

        std::istringstream dateStrStreamD(dateStrLastOfYear);
        dateStrStreamD.exceptions(std::ios::failbit);      
        dateStrStreamD >> date::parse("%Y-%m-%d",dateDayLastOfYear);

        //Adding 1 here because Jan 1 otherwise doesn't count as a day.
        
        double daysInYear = 
          double((dateDayLastOfYear-dateDayFirstOfYear).count())
          + 1.0;

        //Here 31-12-2025 will be 364 days from 01-01-2025 and the 
        //fraction will be 364/365=0.9973 of a year. This is consistent 
        //with https://www.epochcounter.com
          date = double(year) 
            + double( (dateDay-dateDayFirstOfYear).count() )/daysInYear;
      }

      return date;
    };
    //==========================================================================
    static double getTodaysDate(){

      auto date = date::floor<date::days>(std::chrono::system_clock::now());  
      auto ymd = date::year_month_day{date};
            
      std::stringstream ss;
      ss << ymd;
      std::string dateStr(ss.str());
      return convertToFractionalYear(dateStr);

    }
    //==========================================================================
    static void getTodaysDate(std::string &todaysDateUpd){

      auto date = date::floor<date::days>(std::chrono::system_clock::now());  
      auto ymd = date::year_month_day{date};
            
      std::stringstream ss;
      ss << ymd;
      todaysDateUpd = ss.str();

    }
    //==========================================================================
    static int getIndexClosestToDate(
                  const std::string &date, 
                  const std::vector< std::string > &dateSet){

      double dateNum = convertToFractionalYear(date);
      double dateErrorBest = std::fabs(dateNum
                                     -convertToFractionalYear(dateSet[0]));
      int indexBest=0;
      
      for(int i=1; i<dateSet.size();++i){
        double dateError = std::fabs(dateNum-convertToFractionalYear(dateSet[i]));
        if(dateError<dateErrorBest){
          indexBest=i;    
        }
      }
      return indexBest;
    };
    //==========================================================================
    static int getIndexClosestToDate(
                  double dateNumerical, 
                  const std::vector< double > &dateSetNumerical){

      double dateErrorBest = std::fabs(dateNumerical-dateSetNumerical[0]);

      int indexBest=0;
      
      for(int i=1; i<dateSetNumerical.size();++i){
        double dateError = std::fabs(dateNumerical-dateSetNumerical[i]);
        if(dateError<dateErrorBest){
          indexBest=i;
          dateErrorBest=dateError;   
        }
      }
      return indexBest;
    };
    //==========================================================================
    static int calcDifferenceInDaysBetweenTwoDates(const std::string &dateA,
                                            const char* dateAFormat,
                                            const std::string &dateB,
                                            const char* dateBFormat){

      std::istringstream dateStream(dateA);
      dateStream.exceptions(std::ios::failbit);
      date::sys_days daysA;
      dateStream >> date::parse(dateAFormat,daysA);

      dateStream.clear();
      dateStream.str(dateB);
      date::sys_days daysB;
      dateStream >> date::parse(dateBFormat,daysB);

      int daysDifference = (daysA-daysB).count();

      return daysDifference;

    };

    //============================================================================
    static bool extractTTM( int indexA,
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
        count            += daysInterval;
              
        if(daysInterval < 0){
          std::cerr << "Error: dates should be in reverse chronological order"
                    << std::endl;
          std::abort();                    
        }

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

};

#endif