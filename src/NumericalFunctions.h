//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef NUMERICAL_FUNCTIONS
#define NUMERICAL_FUNCTIONS

#include <cmath>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <numeric>
#include <boost/math/statistics/linear_regression.hpp>

#include "FinancialAnalysisFunctions.h"


class NumericalFunctions {

  public:
    //
    // If this enum is changed note that there are 
    // switch statements in this header file that 
    // assume the following order
    //
    // 0. ExponentialModel
    // 1. ExponentialCyclicalModel
    // 2. LinearModel
    // 3. LinearCyclicalModel
    // 4. CyclicalModel
    //
    enum EmpiricalGrowthModelTypes{
      ExponentialModel=0,
      ExponentialCyclicalModel,
      LinearModel,
      LinearCyclicalModel,
      CyclicalModel,
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
      double annualGrowthRateOfTrendline;
      double r2;
      double r2Trendline;
      double r2Cyclic;            
      bool validFitting;
      int outlierCount;
      std::vector< double > parameters;  
      std::vector< double > x;
      std::vector< double > y;
      std::vector< double > yTrendline;
      std::vector< double > yCyclic;
      std::vector< double > yCyclicData;
      std::vector< double > yCyclicNorm;
      std::vector< double > yCyclicNormData;

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


    //Evaluates R2 using the same method as boost's linear_regresssion
    //function
    // https://live.boost.org/doc/libs/1_83_0/libs/math/doc/html/math_toolkit/linear_regression.html
    static double calcR2(const std::vector< double > &y,
                         const std::vector< double > &yRef){

      
      if(y.size()!=yRef.size()){
        std::cout << "Error: calcR2 both vectors must have the same length" 
                  << std::endl;
        std::abort();
      }

      double yRefMean = std::reduce(yRef.begin(), yRef.end());
      yRefMean /= static_cast<double>(yRef.size());

      double numerator=0.;
      double denominator=0.;

      for(size_t i=0; i<yRef.size();++i){
        double t0 = yRef[i]-y[i];
        numerator += (t0*t0);

        double t1 = yRef[i]-yRefMean;
        denominator += t1*t1;
      }

      return (1.0 - (numerator/denominator));

    };
    //==========================================================================
    static void fitCyclicalModel(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  double minTimeResolutionInYears,
                  EmpiricalGrowthModel &modelUpd){

      if(x.size()!=y.size()){
        std::cout << "Error: x and y must the same size"
                  << std::endl;
        std::abort();
      }

      std::vector< double > xN(x.size());
      std::vector< double > yM(y.size());

      //copy over y
      for(size_t i=0; i<y.size();++i){
        yM[i]=y[i];
      }

      double xSpan = std::abs(x.back()-x.front());
      if(xSpan < 0){
        std::cout << "Error: data must be ordered from earlierst to most recent"
                  << std::endl;
        std::abort();
      }

      modelUpd.x.resize(x.size());
      modelUpd.y.resize(x.size());
      //Normalize time to 0-1
      for(size_t i=0; i<x.size();++i){
        xN[i] = (x[i]-x.front())/xSpan;
        modelUpd.x[i] = x[i];
        modelUpd.y[i] = 0.;
      }

      int n = static_cast<int>(std::round(xSpan/minTimeResolutionInYears));

      //index into parameters
      modelUpd.parameters.resize(2*n);
      int indexParameters=0;

      for(int i=0; i<n; ++i){
        double w = static_cast<double>(i+1.0);
        
        std::vector< double > yDotSin(x.size());
        std::vector< double > yDotCos(x.size());

        //Evaluate the dot product between 
        // y and 
        // sin(xN*2*pi*2) and 
        // cos(xN*2*pi*2)

        for(int j=0; j<xN.size();++j){
          double sinTerm = std::sin(xN[j]*2.0*M_PI*w);
          double cosTerm = std::cos(xN[j]*2.0*M_PI*w);
          yDotSin[j] = y[j]*sinTerm;
          yDotCos[j] = y[j]*cosTerm;
        }

        //Evaluate the integral of the dot products. Here I'm just going to 
        //use the trapezoidal method
        double intYDotSin=0;
        double intYDotCos=0;
        for(int j=1; j<xN.size();++j){
          double dx = x[j]-x[j-1];
          intYDotSin += 0.5*(yDotSin[j]+yDotSin[j-1])*dx;
          intYDotCos += 0.5*(yDotCos[j]+yDotCos[j-1])*dx;
        }
        double cYSin = (2.0/xSpan)*intYDotSin;
        double cYCos = (2.0/xSpan)*intYDotCos;
        
        modelUpd.parameters[indexParameters] = cYSin;
        ++indexParameters;
        modelUpd.parameters[indexParameters] = cYCos;
        ++indexParameters;

        //Update the residual and fitting vector
        for(int j=0; j<yM.size();++j){
          double sinTerm = std::sin(xN[j]*2.0*M_PI*w);
          double cosTerm = std::cos(xN[j]*2.0*M_PI*w);
          yM[j]         = yM[j] 
                          - cYSin*sinTerm - cYCos*cosTerm;
          modelUpd.y[j] = modelUpd.y[j]  
                          + cYSin*sinTerm + cYCos*cosTerm;
        }

      }

      double r2 = calcR2(y,modelUpd.y);

      modelUpd.duration=xSpan;
      modelUpd.outlierCount=0;
      modelUpd.annualGrowthRateOfTrendline=0;
      modelUpd.r2=r2;
      modelUpd.validFitting=true;
      modelUpd.modelType = 
        static_cast<int>(EmpiricalGrowthModelTypes::CyclicalModel);
      modelUpd.yCyclicData = y;



    };
    //==========================================================================
    static void fitCyclicalModelWithLinearBaseline(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  double minTimeResolutionInYears,
                  EmpiricalGrowthModel &modelUpd){

      EmpiricalGrowthModel linearModel;
      fitLinearGrowthModel(x,y,linearModel);

      //Subtract off the base line then call            
      std::vector<double> yC(y.size());
      for(size_t i=0; i<yC.size();++i){
        yC[i] = y[i] - linearModel.y[i];
      }

      EmpiricalGrowthModel cyclicalModel;
      fitCyclicalModel(x,yC,minTimeResolutionInYears,cyclicalModel);

      //Form and evaluate the linear+cyclical model
      std::vector<double> yM(y.size());
      for(size_t i=0; i<yM.size();++i){
        yM[i] = linearModel.y[i] + cyclicalModel.y[i];
      }

      double r2LC = calcR2(yM,y);

      modelUpd.duration     = std::abs(x.back()-x.front());
      modelUpd.annualGrowthRateOfTrendline   = linearModel.annualGrowthRateOfTrendline;
      modelUpd.modelType = 
        static_cast<int>(EmpiricalGrowthModelTypes::LinearCyclicalModel);
      modelUpd.parameters   = linearModel.parameters;
      for(size_t i=0; i<cyclicalModel.parameters.size();++i){
        modelUpd.parameters.push_back(cyclicalModel.parameters[i]);
      }
      modelUpd.r2           = r2LC;
      modelUpd.r2Trendline  = linearModel.r2;
      modelUpd.r2Cyclic     = cyclicalModel.r2;
      modelUpd.validFitting = true;
      modelUpd.outlierCount =   linearModel.outlierCount 
                             +cyclicalModel.outlierCount;
      modelUpd.x            = x;
      modelUpd.y            = yM;
      modelUpd.yTrendline   = linearModel.y;
      modelUpd.yCyclic      = cyclicalModel.y;
      modelUpd.yCyclicData  = cyclicalModel.yCyclicData;

      modelUpd.yCyclicNorm.resize(modelUpd.y.size());
      modelUpd.yCyclicNormData.resize(modelUpd.y.size());
      for(size_t i=0; i<modelUpd.y.size();++i){
        modelUpd.yCyclicNorm[i] = 
          modelUpd.yCyclic[i]/modelUpd.yTrendline[i]; 
        modelUpd.yCyclicNormData[i] = 
          modelUpd.yCyclicData[i]/modelUpd.yTrendline[i];        
      }
    };
    //==========================================================================
    static void fitLinearGrowthModel(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  EmpiricalGrowthModel &modelUpd){

      //Remove the bias on x
      double w0 = *std::min_element(x.begin(),x.end());
      double w1 = *std::max_element(x.begin(),x.end());      
      std::vector< double > w;
      w.resize(x.size());
      for(size_t i=0; i<x.size();++i){
        w[i] = x[i]-w0;
      }      

      modelUpd.duration = 
        std::abs(x.front()-x.back());

      modelUpd.modelType = 
        static_cast<int>(
          EmpiricalGrowthModelTypes::LinearModel);

      auto [y0Mdl, dydwMdl, r2] = 
        boost::math::statistics::
          simple_ordinary_least_squares_with_R_squared(w,y);


      double y1Mdl        = y0Mdl + dydwMdl*modelUpd.duration;    

      //Here I'm normalizing the slope by the starting and ending values
      //to get something like a growth rate
      double growth0      = (dydwMdl/y0Mdl);
      double growth1      = (dydwMdl/y1Mdl);

      std::vector< double > yMdl(x.size());   
      for(size_t i=0; i<w.size(); ++i){
        yMdl[i]= y0Mdl + dydwMdl*w[i];
      }

      //double r2Test = calcR2(yMdl,y);


      modelUpd.annualGrowthRateOfTrendline=growth1;
      modelUpd.parameters.push_back(w0);
      modelUpd.parameters.push_back(y0Mdl);
      modelUpd.parameters.push_back(dydwMdl);
      modelUpd.validFitting=true;
      modelUpd.outlierCount=0;
      modelUpd.r2=r2;
      modelUpd.x = x;
      modelUpd.y = yMdl;
                              
    };
    //==========================================================================    
    static void fitCyclicalModelWithExponentialBaseline(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  double minTimeResolutionInYears,
                  double maxProportionOfNegativeAtoi,
                  EmpiricalGrowthModel &modelUpd){

      EmpiricalGrowthModel exponentialModel;
      fitExponentialGrowthModel(x,y,maxProportionOfNegativeAtoi,
                                exponentialModel);

      //Subtract off the base line then call            
      std::vector<double> yC(y.size());
      for(size_t i=0; i<yC.size();++i){
        yC[i] = y[i] - exponentialModel.y[i];
      }

      EmpiricalGrowthModel cyclicalModel;
      fitCyclicalModel(x,yC,minTimeResolutionInYears,cyclicalModel);

      //Form and evaluate the linear+cyclical model
      std::vector<double> yM(y.size());
      for(size_t i=0; i<yM.size();++i){
        yM[i] = exponentialModel.y[i] + cyclicalModel.y[i];
      }

      double r2LC = calcR2(yM,y);


      modelUpd.duration   = std::abs(x.back()-x.front());
      modelUpd.annualGrowthRateOfTrendline = exponentialModel.annualGrowthRateOfTrendline;
      modelUpd.modelType  = 
        static_cast<int>(EmpiricalGrowthModelTypes::ExponentialCyclicalModel);
      modelUpd.outlierCount = exponentialModel.outlierCount
                             +cyclicalModel.outlierCount;
      modelUpd.parameters   = exponentialModel.parameters;
      for(size_t i=0; i<cyclicalModel.parameters.size();++i){
        modelUpd.parameters.push_back(cyclicalModel.parameters[i]);
      }
      modelUpd.r2           = r2LC;
      modelUpd.r2Trendline  = exponentialModel.r2;
      modelUpd.r2Cyclic     = cyclicalModel.r2;
      modelUpd.validFitting = true;
      modelUpd.x            = x;
      modelUpd.y            = yM;
      modelUpd.yTrendline   = exponentialModel.y;
      modelUpd.yCyclic      = cyclicalModel.y;
      modelUpd.yCyclicData  = cyclicalModel.yCyclicData;

      modelUpd.yCyclicNorm.resize(modelUpd.y.size());
      modelUpd.yCyclicNormData.resize(modelUpd.y.size());
      for(size_t i=0; i<modelUpd.y.size();++i){
        modelUpd.yCyclicNorm[i] = 
          modelUpd.yCyclic[i]/modelUpd.yTrendline[i]; 
        modelUpd.yCyclicNormData[i] = 
          modelUpd.yCyclicData[i]/modelUpd.yTrendline[i];        
      }


    };
    //==========================================================================
    static void fitExponentialGrowthModel(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  double maxProportionOfNegativeAtoi,
                  EmpiricalGrowthModel &modelUpd){

      //Remove the bias on x
      double w0 = *std::min_element(x.begin(),x.end());
      double w1 = *std::max_element(x.begin(),x.end());
      std::vector< double > w;

      w.resize(x.size());
      for(size_t i=0; i<x.size();++i){
        w[i] = x[i]-w0;
      }      

      //Go through and count the y entries < 1
      std::vector< double > z;
      z.resize(y.size());
      
      modelUpd.validFitting=true;
      modelUpd.outlierCount=0;
      for(size_t i=0; i< y.size();++i){
        if(y[i]<1){
          z[i]=1.0;
          ++modelUpd.outlierCount;
        }else{
          z[i]=y[i];
        }
      }
      
      double invalidEntryProportion = 
          static_cast<double>(modelUpd.outlierCount)
        / static_cast<double>(y.size());

      if(invalidEntryProportion > maxProportionOfNegativeAtoi){
        modelUpd.validFitting=false;
      }

      if(modelUpd.validFitting){

        modelUpd.duration = w1-w0;

        modelUpd.modelType = 
          static_cast<int>(
            EmpiricalGrowthModelTypes::ExponentialModel);

        std::vector<double> logZ(y.size());
        for(size_t i=0; i< z.size();++i){
          logZ[i] = std::log(z[i]);
        } 

        auto [z0Mdl, dzdwMdl, logZR2] = 
          boost::math::statistics::
            simple_ordinary_least_squares_with_R_squared(w,logZ);

        double growth       = std::exp( dzdwMdl )-1.0;
        double y0Mdl        = std::exp( z0Mdl );
        double y1Mdl        = std::exp( z0Mdl + dzdwMdl*modelUpd.duration);    

        std::vector< double > yMdl(x.size());   
        for(size_t i=0; i<w.size(); ++i){
          yMdl[i]=y0Mdl*pow(1.0+growth,w[i]);
        }

        double r2 = calcR2(yMdl,y);

        modelUpd.outlierCount=0;
        modelUpd.annualGrowthRateOfTrendline=growth;
        modelUpd.parameters.push_back(w0);
        modelUpd.parameters.push_back(y0Mdl);
        modelUpd.parameters.push_back(growth);


        modelUpd.r2=r2;
        modelUpd.x = x;
        modelUpd.y = yMdl;
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
          double maxPropOfOutliersInExpModel,
          double minCycleTimeInYears,
          double minR2ImprovementOfCyclicalModel,
          bool calcOneGrowthRateForAllData,
          int empiricalModelType)
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

          //Order dateSubV so that it proceeds from the earliest date to the 
          //latest date
          if(dateSubV.front() > dateSubV.back()){
            std::reverse(dateSubV.begin(),dateSubV.end());
            std::reverse(atoiSubV.begin(),atoiSubV.end());
          }


          EmpiricalGrowthModel empModel;
          
          if(empiricalModelType==-1){
            EmpiricalGrowthModel exponentialModel, linearModel;

            fitExponentialGrowthModel(dateSubV,atoiSubV,
                        maxPropOfOutliersInExpModel,
                        exponentialModel);

            fitLinearGrowthModel(dateSubV,atoiSubV,linearModel);

            int modelType = 
              static_cast<int>(EmpiricalGrowthModelTypes::LinearModel);
            int linearModelType = 
              static_cast<int>(EmpiricalGrowthModelTypes::LinearModel);
            int exponentialModelType = 
              static_cast<int>(EmpiricalGrowthModelTypes::ExponentialModel);

            if(linearModel.validFitting==false){
              std::cout << "Error: a linear fit should always be valid"
                        << std::endl;
              std::abort();
            }

            //A linear model is used unless the exponential model
            //is both valid and has a higher R2
            if(exponentialModel.validFitting==true){
              if(exponentialModel.r2 > linearModel.r2){
                modelType=static_cast<int>(
                    EmpiricalGrowthModelTypes::ExponentialModel);
              }
            }

            if(modelType == exponentialModelType){

              fitCyclicalModelWithExponentialBaseline(
                dateSubV,
                atoiSubV,
                minCycleTimeInYears,
                maxPropOfOutliersInExpModel,
                empModel);
                            
            }else{

              fitCyclicalModelWithLinearBaseline(
                dateSubV,
                atoiSubV,
                minCycleTimeInYears,
                empModel);

            }
          }else{
            switch(empiricalModelType){
              case 0:
              {
                fitExponentialGrowthModel(dateSubV,atoiSubV,
                        maxPropOfOutliersInExpModel,empModel);
              }break;
              case 1:
              {                
                fitCyclicalModelWithExponentialBaseline(
                  dateSubV,
                  atoiSubV,
                  minCycleTimeInYears,
                  maxPropOfOutliersInExpModel,
                  empModel);
              }break;
              case 2:
              {
                fitLinearGrowthModel(dateSubV,atoiSubV,empModel);
              }break;
              case 3:
              {
                fitCyclicalModelWithLinearBaseline(
                  dateSubV,
                  atoiSubV,
                  minCycleTimeInYears,
                  empModel);
              }break;
              default:
                std::cout <<"Error: empiricalModelType must be [0,1,2,3]"
                          <<std::endl;
                std::abort();
            };

          }

          

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

          double roic = empModel.annualGrowthRateOfTrendline/rrAvg;

          //
          // Store the model results
          //
          if(count > 0 && !std::isnan(rrAvg) && !std::isinf(rrAvg)){

            empiricalGrowthDataUpd.afterTaxOperatingIncomeGrowth.push_back(
                empModel.annualGrowthRateOfTrendline);

            empiricalGrowthDataUpd.reinvestmentRate.push_back(rrAvg);
            empiricalGrowthDataUpd.reinvestmentRateSD.push_back(rrSd);
            empiricalGrowthDataUpd.returnOnInvestedCapital.push_back(roic);

            empiricalGrowthDataUpd.dates.push_back(
                dateV[indexDate]);
            empiricalGrowthDataUpd.datesNumerical.push_back(
                dateNumV[indexDate]);

            empiricalGrowthDataUpd.model.push_back(empModel);     

          }  
        }   
      }     

    };


    //==========================================================================
    static void appendEmpiricalGrowthRateData(
        size_t index,      
        const EmpiricalGrowthDataSet &empiricalGrowthData,
        double dateInYears,
        double costOfCapitalMature,
        const std::string nameToPrepend,
        std::vector< std::string > &termNames,
        std::vector< double > &termValues)
    {



      termNames.push_back(nameToPrepend+"AfterTaxOperatingIncomeGrowth");
      termValues.push_back(empiricalGrowthData.afterTaxOperatingIncomeGrowth[index]);

      termNames.push_back(nameToPrepend+"r2");
      termValues.push_back(empiricalGrowthData.model[index].r2); 

      termNames.push_back(nameToPrepend+"r2Trendline");
      termValues.push_back(empiricalGrowthData.model[index].r2Trendline); 

      termNames.push_back(nameToPrepend+"r2Cyclic");
      termValues.push_back(empiricalGrowthData.model[index].r2Cyclic); 

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

      termNames.push_back(nameToPrepend+"Duration");
      termValues.push_back(empiricalGrowthData.model[index].duration); 

      termNames.push_back(nameToPrepend+"ModelDateError");
      double dateError = 
        dateInYears - empiricalGrowthData.datesNumerical[index];
      termValues.push_back(dateError); 

      termNames.push_back(nameToPrepend+"OutlierCount");
      termValues.push_back(empiricalGrowthData.model[index].outlierCount); 

    };   

};



#endif