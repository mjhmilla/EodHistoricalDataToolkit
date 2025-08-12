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


};

#endif