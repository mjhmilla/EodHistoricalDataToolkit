//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef NUMERICAL_FUNCTIONS
#define NUMERICAL_FUNCTIONS

#include <cmath>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <boost/math/statistics/linear_regression.hpp>

#include "FinancialAnalysisFunctions.h"


class NumericalFunctions {

  public:

    enum EmpiricalGrowthModelTypes{
      ExponentialModel=0,
      ExponentialCyclicalModel,
      LinearModel,
      LinearCyclicalModel,
      NUM_EMPIRICAL_GROWTH_MODELS
    };
    //============================================================================
    struct EmpiricalGrowthDataSetSample{
      std::vector< double > years;
      std::vector< double > afterTaxOperatingIncome;
    };
    //============================================================================
    struct EmpiricalGrowthModel{
      int modelType;
      double duration;
      double growthRate;
      double r2;
      bool validFitting;
      int outlierCount;
      std::vector< double > parameters;      
    };    
    //============================================================================
    struct EmpiricalGrowthDataSet{
      std::vector< std::string > dates;
      std::vector< double > datesNumerical;
      std::vector< double > afterTaxOperatingIncomeGrowth;
      std::vector< double > reinvestmentRate;
      std::vector< double > reinvestmentRateSD;
      std::vector< double > returnOnInvestedCapital;
      std::vector< EmpiricalGrowthModel > model;

    };

    //==============================================================================
    static size_t getIndexOfEmpiricalGrowthDataSet(
                          double dateTarget, 
                          double maxDateTargetError,
                          const EmpiricalGrowthDataSet &empiricalGrowthData)
    {

      bool found = false;
      size_t index = 0;
      while(!found && index < empiricalGrowthData.datesNumerical.size()){
        double dateError = dateTarget - empiricalGrowthData.datesNumerical[index];
        if(dateError >= 0 && dateError <= maxDateTargetError){
          found = true;
        }else{
          ++index;
        }
      }
      return index;
    };

    //==========================================================================
    static void fitExponentialGrowthModel(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  double maxProportionOfNegativeAtoi,
                  EmpiricalGrowthModel &exponentialModelUpd){

      //Remove the bias on x
      double w0 = *std::min_element(x.begin(),x.end());
      std::vector< double > w;
      w.resize(x.size());
      for(size_t i=0; i<x.size();++i){
        w[i] = x[i]-w0;
      }      

      //Go through and count the y entries < 1
      std::vector< double > z;
      z.resize(y.size());
      
      exponentialModelUpd.validFitting=true;
      exponentialModelUpd.outlierCount = 0;
      for(size_t i=0; i< y.size();++i){
        if(y[i]<1){
          w[i]=1.0;
          ++exponentialModelUpd.outlierCount;
        }else{
          w[i]=y[i];
        }
      }
      
      double invalidEntryProportion = 
          static_cast<double>(exponentialModelUpd.outlierCount)
        / static_cast<double>(y.size());

      if(invalidEntryProportion > maxProportionOfNegativeAtoi){
        exponentialModelUpd.validFitting=false;
      }

      if(exponentialModelUpd.validFitting){

        exponentialModelUpd.duration = 
          std::abs(x.front()-x.back());

        exponentialModelUpd.modelType = 
          static_cast<int>(
            EmpiricalGrowthModelTypes::ExponentialModel);

        std::vector<double> logZ(y.size());
        for(size_t i=0; i< z.size();++i){
          logZ[i] = std::log(z[i]);
        } 

        auto [e0, e1, r2] = 
          boost::math::statistics::
            simple_ordinary_least_squares_with_R_squared(w,logZ);

        double growth       = std::exp( e1 )-1.0;
        double initalValue  = std::exp( e0 + e1*x.back());
        double y0Exp        = std::exp( e0 + e1*x.back());
        double y1Exp        = std::exp( e0 + e1*x.front());             

        exponentialModelUpd.growthRate=growth;
        exponentialModelUpd.parameters.push_back(w0);
        exponentialModelUpd.parameters.push_back(initalValue);
        exponentialModelUpd.parameters.push_back(growth);
        exponentialModelUpd.r2=r2;
      }

    };

    //==========================================================================
    static void extractEmpiricalGrowthRates(
          EmpiricalGrowthDataSet &empiricalGrowthDataUpd,
          const nlohmann::ordered_json &fundamentalData,
          const std::vector< double > &taxRateRecord,
          const FinancialAnalysisFunctions::AnalysisDates &analysisDates,
          std::string &timePeriod,
          int indexLastCommonDate,
          bool quarterlyTTMAnalysis,
          int maxDayErrorTTM,
          double growthIntervalInYears,
          double maxProportionOfNegativeOpIncome,
          bool calcOneGrowthRateForAllData)
    {

      int indexDate       = -1;
      bool validDateSet    = true;

      std::vector < double > dateNumV;    //Fractional year
      std::vector < std::string > dateV;  //string yer
      std::vector < double > atoiV;       //after tax operating income 
      std::vector < double > rrV;         //reinvestment rate

      std::string previousTimePeriod("");
      std::vector< std::string > previousDateSet;
      std::vector< double > previousDateSetWeight;

      double maxYearErrorTTM = maxDayErrorTTM / DateFunctions::DAYS_PER_YEAR;

      //
      // For all time periods with sufficient data evaluate: after-tax operating
      // income and the reinvestment rate
      //
      while( (indexDate+1) < indexLastCommonDate && validDateSet){
        ++indexDate;

        //The set of dates used for the TTM analysis
        std::vector < std::string > dateSetTTM;
        std::vector < double > dateSetTTMWeight;

        if(quarterlyTTMAnalysis){
          validDateSet = 
            FinancialAnalysisFunctions::extractTTM( indexDate,
                                                    analysisDates.common,
                                                    "%Y-%m-%d",
                                                    dateSetTTM,                                    
                                                    dateSetTTMWeight,
                                                    maxDayErrorTTM); 
          if(!validDateSet){
            break;
          }     
        }else{
          dateSetTTM.push_back(analysisDates.common[indexDate]);
        }                     

        //Check if we have enough data to get the previous time period
        int indexPrevious = indexDate+static_cast<int>(dateSetTTM.size());        

        //
        //Fetch the previous TTM
        //
        previousTimePeriod = analysisDates.common[indexPrevious];
        previousDateSet.resize(0);

        if(quarterlyTTMAnalysis){
          validDateSet = 
            FinancialAnalysisFunctions::extractTTM( indexPrevious,
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

        if(validDateSet){
          double operatingIncome = 
            FinancialAnalysisFunctions::
              sumFundamentalDataOverDates(fundamentalData,FIN,IS,
                                          timePeriod.c_str(),dateSetTTM,
                                          "operatingIncome",true);
          
          if(JsonFunctions::isJsonFloatValid(operatingIncome)){
          
            double dateEntry = 
              DateFunctions::convertToFractionalYear(
                            analysisDates.common[indexDate]); 

            double taxRate = taxRateRecord[indexDate];
            double atoiEntry = operatingIncome*(1-taxRate);


            bool appendTermRecord=false;
            bool setNansToMissingValue=true;
            std::vector< std::string > termNames;
            std::vector< double > termValues;

            std::string parentName("");

            double netCapitalExpenditures = 
            FinancialAnalysisFunctions::
              calcNetCapitalExpenditures( fundamentalData, 
                                          dateSetTTM,
                                          previousDateSet,
                                          timePeriod.c_str(),
                                          appendTermRecord,
                                          parentName,
                                          setNansToMissingValue,
                                          termNames,
                                          termValues);

            double changeInNonCashWorkingCapital = 
              FinancialAnalysisFunctions::
                calcChangeInNonCashWorkingCapital(  fundamentalData, 
                                                    dateSetTTM,
                                                    previousDateSet,
                                                    timePeriod.c_str(),
                                                    appendTermRecord,
                                                    parentName,
                                                    setNansToMissingValue,
                                                    termNames,
                                                    termValues); 
            double rrEntry = (netCapitalExpenditures
                        +changeInNonCashWorkingCapital)
                        /atoiEntry;

            bool rrEntryValid   = JsonFunctions::isJsonFloatValid(rrEntry);
            bool dateEntryValid = JsonFunctions::isJsonFloatValid(dateEntry);
            bool atoiEntryValid = JsonFunctions::isJsonFloatValid(atoiEntry);

            if(rrEntryValid && dateEntryValid && atoiEntryValid && atoiEntry > 0.){
              dateV.push_back(analysisDates.common[indexDate]);
              dateNumV.push_back(dateEntry);
              atoiV.push_back(atoiEntry);
              rrV.push_back(rrEntry);              
            }
          }
        }
      }
      
      //
      //Extract the growth values for an interval with 
      //growthIntervalInYears in it
      //
      indexDate = -1;
      validDateSet=true;
      int indexDateMax = static_cast<int>(rrV.size());

      while( (indexDate+1) < indexLastCommonDate 
              && indexDate < indexDateMax 
              && rrV.size() >= 2
              && validDateSet
              && ((calcOneGrowthRateForAllData && indexDate == -1) 
                    || !calcOneGrowthRateForAllData)){

        ++indexDate;

        std::vector< double > dateSubV; //date sub vector
        std::vector< double > atoiSubV; // after-tax operating income sub vector
        std::vector< double > rrSubV; //rate of return sub vector

        //
        //Extract the sub interval
        //
        dateSubV.push_back(dateNumV[indexDate]);
        atoiSubV.push_back(atoiV[indexDate]);
        rrSubV.push_back(rrV[indexDate]);

        int indexDateStart=indexDate+1;
        bool foundStartDate=false;
   

        while(  !foundStartDate 
              && indexDateStart < indexLastCommonDate 
              && indexDateStart < indexDateMax){
            
            double timeSpan      = dateSubV[0] - dateNumV[indexDateStart]; 
            double timeSpanError = timeSpan-growthIntervalInYears;

            if( timeSpanError < maxYearErrorTTM ){
              dateSubV.push_back(dateNumV[indexDateStart]);
              atoiSubV.push_back(atoiV[indexDateStart]);
              rrSubV.push_back(rrV[indexDateStart]); 
            }else{
              foundStartDate = true;
            }       
            ++indexDateStart;

        }


        //
        //Fit each of the models to the data and choose the most appropriate
        //model using this criteria
        //
        //  Linear model, if R2 linear > R2 exponential
        //  Linear + cyclical model, if R2 linear+cyclical >= linear + 0.10
        //  Exponential model, if R2 exponential > R2 linear
        //  Exponential + cyclical model, if R2 exp.+cyclical >= exp. + 0.10
        //
        //
        if(    dateSubV.size() >= 2 
           && (dateSubV.size()==atoiSubV.size())
           && (foundStartDate || calcOneGrowthRateForAllData)){


          NumericalFunctions::EmpiricalGrowthModel expModel, bestModel;


          NumericalFunctions::fitExponentialGrowthModel(dateSubV,atoiSubV,
                                    maxProportionOfNegativeOpIncome,expModel);
          bestModel=expModel;
          double initialValue = bestModel.parameters[1];

          //
          //Evaluate the average rate of reinvestment
          //
          double count=0.;
          double rrAvg = 0.;

          for(size_t i=0; i < rrSubV.size();++i){
            if(JsonFunctions::isJsonFloatValid(rrSubV[i])){
              rrAvg += rrSubV[i];
              count += 1.0;
            }
          }

          rrAvg = rrAvg/count;

          double rrSd = 0;
          for(size_t i=0; i<rrSubV.size();++i){
            double rrError = (rrSubV[i]-rrAvg);
            rrSd += rrError*rrError;
          }
          rrSd = std::sqrt(rrSd / count);

          double roic = bestModel.growthRate/rrAvg;

          //
          // Store the model results
          //
          if(count > 0 && !std::isnan(rrAvg) && !std::isinf(rrAvg)){

            empiricalGrowthDataUpd.afterTaxOperatingIncomeGrowth.push_back(
                bestModel.growthRate);

            empiricalGrowthDataUpd.reinvestmentRate.push_back(rrAvg);
            empiricalGrowthDataUpd.reinvestmentRateSD.push_back(rrSd);
            empiricalGrowthDataUpd.returnOnInvestedCapital.push_back(roic);

            empiricalGrowthDataUpd.dates.push_back(
                dateV[indexDate]);
            empiricalGrowthDataUpd.datesNumerical.push_back(
                dateNumV[indexDate]);

            empiricalGrowthDataUpd.model.push_back(bestModel);     

            //empiricalGrowthDataUpd.intervalStartDate.push_back(dateMin);

            //empiricalGrowthDataUpd.durationNumerical.push_back(
            //    duration);
            //empiricalGrowthDataUpd.typeOfGrowthModel.push_back(
            //    bestModel.modelType);    
            //empiricalGrowthDataUpd.growthModelR2.push_back(
            //    bestModel.R2);
            //empiricalGrowthDataUpd.outlierCount.push_back(
            //    invalidEntryCount);
            
            //empiricalGrowthDataUpd.
            //intervalStartingAfterTaxOperatingIncome.push_back(initialValue);
          }  
        }   
      }     

    };
    //==========================================================================
    static void evaluateGrowthModel(
      const std::string &endDate,
      const NumericalFunctions::EmpiricalGrowthDataSet &empiricalGrowthData,
      NumericalFunctions::EmpiricalGrowthDataSetSample &empiricalGrowthSampleUpd)
    {
      bool found = false;
      size_t index = 0;
      while(!found && index < empiricalGrowthData.dates.size()){
        if(endDate.compare(empiricalGrowthData.dates[index])==0){
          found =true;          
        }else{
          ++index;
        }
      }

      if(found){
        if(empiricalGrowthData.model[index].modelType ==
          static_cast<int>(NumericalFunctions::EmpiricalGrowthModelTypes::ExponentialModel)){
          int n = std::round(empiricalGrowthData.model[index].duration);
                    
          empiricalGrowthSampleUpd.years.resize(n+1);
          empiricalGrowthSampleUpd.afterTaxOperatingIncome.resize(n+1);
          
          double y0 = 
            empiricalGrowthData.model[index].parameters[1];
          double t0 = 
            empiricalGrowthData.model[index].parameters[0];
          double r = 
            empiricalGrowthData.model[index].parameters[2];

          for(int i=0; i<=n;++i){
            empiricalGrowthSampleUpd.years[i] 
              = t0 + static_cast<double>(i);
            empiricalGrowthSampleUpd.afterTaxOperatingIncome[i] 
              = y0*std::pow(1.0+r,static_cast<double>(i));
          }

        }else{
          std::cerr << "Error: this function can only "
                    <<  "evaluate ExponentialGrowthModels"
                    << std::endl;
          std::abort();
        }
      }
    };

    //==========================================================================
    static void appendEmpiricalGrowthRateData(
        size_t index,      
        const NumericalFunctions::EmpiricalGrowthDataSet &empiricalGrowthData,
        double dateInYears,
        double costOfCapitalMature,
        const std::string nameToPrepend,
        std::vector< std::string > &termNames,
        std::vector< double > &termValues)
    {



      termNames.push_back(nameToPrepend+"AfterTaxOperatingIncomeGrowth");
      termValues.push_back(empiricalGrowthData.afterTaxOperatingIncomeGrowth[index]);


      termNames.push_back(nameToPrepend+"ModelR2");
      termValues.push_back(empiricalGrowthData.model[index].r2); 

      termNames.push_back(nameToPrepend+"ModelType");
      termValues.push_back(empiricalGrowthData.model[index].modelType); 

      termNames.push_back(nameToPrepend+"ReinvestmentRateMean");
      termValues.push_back(empiricalGrowthData.reinvestmentRate[index]);

      termNames.push_back(nameToPrepend+"ReinvestmentRateStandardDeviation");
      termValues.push_back(empiricalGrowthData.reinvestmentRateSD[index]);

      termNames.push_back(nameToPrepend+"ReturnOnInvestedCapital");
      termValues.push_back(empiricalGrowthData.returnOnInvestedCapital[index]);

      termNames.push_back(nameToPrepend+"ReturnOnInvestedCapitalLessCostOfCapital");
      double roicEmpLCC = empiricalGrowthData.returnOnInvestedCapital[index]
                          -costOfCapitalMature;          
      termValues.push_back(roicEmpLCC);

      termNames.push_back(nameToPrepend+"DataDuration");
      termValues.push_back(empiricalGrowthData.model[index].duration); 

      termNames.push_back(nameToPrepend+"ModelDateError");
      double dateError = 
        dateInYears - empiricalGrowthData.datesNumerical[index];
      termValues.push_back(dateError); 

      //termNames.push_back(nameToPrepend+"OutlierCount");
      //termValues.push_back(empiricalGrowthData.outlierCount[index]); 

    };   

};



#endif