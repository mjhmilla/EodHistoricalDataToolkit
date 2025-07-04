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


const double Percentiles[5]     ={0.05, 0.25, 0.5, 0.75, 0.95};


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
      std::vector< double > yCyclicNormDataPercentiles;
      EmpiricalGrowthModel():
        modelType(-1),
        duration(std::numeric_limits<double>::signaling_NaN()),
        annualGrowthRateOfTrendline(std::numeric_limits<double>::signaling_NaN()),
        r2(std::numeric_limits<double>::signaling_NaN()),
        r2Trendline(std::numeric_limits<double>::signaling_NaN()),
        r2Cyclic(std::numeric_limits<double>::signaling_NaN()),
        validFitting(false),
        outlierCount(0){};
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

    //============================================================================
    struct EmpiricalGrowthSettings{
      int maxDateErrorInDays;
      double growthIntervalInYears;
      double maxOutlierProportionInEmpiricalModel;
      double minCycleDurationInYears;
      double exponentialModelR2Preference;
      bool calcOneGrowthRateForAllData;
      bool includeTimeUnitInAddress;
      int typeOfEmpiricalModel;
    };

    //============================================================================
    struct MetricGrowthDataSet{
      std::vector< std::string > dates;
      std::vector< double > datesNumerical;
      std::vector< double > metricValue;
      std::vector< double > metricGrowthRate;
      std::vector< EmpiricalGrowthModel > model;
    };

    //==========================================================================
    enum PercentileIndices{
      P05=0,
      P25,
      P50,
      P75,
      P95,
      NUM_PERCENTILES
    };
    //==========================================================================
    struct SummaryStatistics{
      std::vector< double > percentiles;
      double min;
      double max;
      double current;
      std::string name;
      SummaryStatistics():
        min(0),
        max(0),
        current(0),
        name(""){
        };
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
    // Note: These summary statistics are interpolated so that this method
    //     will give a sensible response with 1 data point or many.
    static bool extractSummaryStatistics(const std::vector< double > &data, 
                      SummaryStatistics &summary){

      bool validSummaryStatistics = true;                      

      if(data.size() > 0){
      std::vector<double> dataCopy;
      for(size_t i=0; i<data.size();++i){
        dataCopy.push_back(data[i]);
      }

      summary.current = std::nan("-1");

      std::sort(dataCopy.begin(),dataCopy.end());

      summary.min = dataCopy[0];
      summary.max = dataCopy[dataCopy.size()-1];

      if(dataCopy.size() > 1){
        for(size_t i = 0; i < PercentileIndices::NUM_PERCENTILES; ++i){
        double idx = Percentiles[i]*(dataCopy.size()-1);
        int indexA = std::floor(idx);
        int indexB = std::ceil(idx);
        double weightB = idx - static_cast<double>(indexA);
        double weightA = 1.0-weightB;
        double valueA = dataCopy[indexA];
        double valueB = dataCopy[indexB];
        double value = valueA*weightA + valueB*weightB;
        summary.percentiles.push_back(value);
        }
      }else{
        for(size_t i = 0; i < PercentileIndices::NUM_PERCENTILES; ++i){
          summary.percentiles.push_back(dataCopy[0]);
        }
        validSummaryStatistics=false;
      }
      }else{
      validSummaryStatistics=false;
      }
      return validSummaryStatistics;
    };
    //==============================================================================
    static void mapToPercentiles(const std::vector<double> &x,  
                         std::vector<double> &xPercentiles){

      std::vector<size_t> indexSorted = calcSortedVectorIndicies(x,true);
      xPercentiles.resize(x.size());
      for(size_t i=0; i<x.size();++i){
        size_t j=indexSorted[i];
        xPercentiles[j] = static_cast<double>(i)
                        / static_cast<double>(x.size());        
      }

      
    };
    //==============================================================================
    static int getIndexOfMostRecentDate(
                    const EmpiricalGrowthDataSet &empiricalGrowthData){
      int index = -1;
      if(empiricalGrowthData.datesNumerical.size()>0){

        index = 0;
        double dateRange= empiricalGrowthData.datesNumerical.front()
                        -empiricalGrowthData.datesNumerical.back(); 
        if(dateRange < 0){
          index = empiricalGrowthData.datesNumerical.size()-1;
        }
      }
      return index;
    };

    //==============================================================================
    static int getIndexOfMostRecentDate(
                    const MetricGrowthDataSet &metricGrowthData){
      int index = -1;
      if(metricGrowthData.datesNumerical.size()>0){

        index = 0;
        double dateRange= metricGrowthData.datesNumerical.front()
                         -metricGrowthData.datesNumerical.back(); 
        if(dateRange < 0){
          index = metricGrowthData.datesNumerical.size()-1;
        }
      }
      return index;
    };


    //==============================================================================
    static size_t getIndexOfMetricGrowthDataSet(
                          double dateTarget, 
                          double maxDateTargetError,
                          const MetricGrowthDataSet &metricGrowthData)
    {

      bool found = false;
      size_t index = 0;
      while(!found && index < metricGrowthData.datesNumerical.size()){
        double dateError = dateTarget - metricGrowthData.datesNumerical[index];
        if(dateError >= 0 && dateError <= maxDateTargetError){
          found = true;
        }else{
          ++index;
        }
      }
      return index;
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


      if(x.size()==y.size() && x.size()>2){
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
      }else{
        modelUpd.validFitting=false;
      }


    };
    //==========================================================================
    static void fitCyclicalModelWithLinearBaseline(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  double minTimeResolutionInYears,
                  bool forceZeroSlope,
                  EmpiricalGrowthModel &modelUpd){

      if(x.size() == y.size() && x.size() > 2){
        EmpiricalGrowthModel linearModel;
        fitLinearGrowthModel(x,y,forceZeroSlope,linearModel);

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
        mapToPercentiles(modelUpd.yCyclicNormData,
                        modelUpd.yCyclicNormDataPercentiles);
      }else{
        modelUpd.validFitting=false;
      }
    };
    //==========================================================================
    static void fitLinearGrowthModel(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  bool forceZeroSlope,
                  EmpiricalGrowthModel &modelUpd){

      if(x.size() == y.size() && x.size() > 2){
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

        
        double y1Mdl, y0Mdl, dydwMdl;

        if(forceZeroSlope){
          y0Mdl = std::reduce(y.cbegin(),y.cend());
          y0Mdl = y0Mdl / static_cast<double>(y.size());
          y1Mdl = y0Mdl;
          dydwMdl = 0.;


        }else{
          auto [y0Fit, dydwFit, r2Fit] = 
            boost::math::statistics::
            simple_ordinary_least_squares_with_R_squared(w,y);        

          y0Mdl     = static_cast<double>(y0Fit);
          dydwMdl   = static_cast<double>(dydwFit);
          y1Mdl     = y0Mdl + dydwMdl*modelUpd.duration;

        }
            

        //Here I'm normalizing the slope by the starting and ending values
        //to get something like a growth rate
        double growth0      = (dydwMdl/y0Mdl);
        double growth1      = (dydwMdl/y1Mdl);


        std::vector< double > yMdl(x.size());   
        for(size_t i=0; i<w.size(); ++i){
          yMdl[i]= y0Mdl + dydwMdl*w[i];
        }

        double r2 = calcR2(yMdl,y);


        modelUpd.annualGrowthRateOfTrendline=growth1;
        modelUpd.parameters.push_back(w0);
        modelUpd.parameters.push_back(y0Mdl);
        modelUpd.parameters.push_back(dydwMdl);
        modelUpd.validFitting=true;
        modelUpd.outlierCount=0;
        modelUpd.r2=r2;
        modelUpd.x = x;
        modelUpd.y = yMdl;
      }else{
        modelUpd.validFitting=false;
      }
                              
    };
    //==========================================================================    
    static void fitCyclicalModelWithExponentialBaseline(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  double minTimeResolutionInYears,
                  double maxProportionOfNegativeAtoi,
                  EmpiricalGrowthModel &modelUpd){

      if(x.size() == y.size() && x.size() > 2){
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

        mapToPercentiles(modelUpd.yCyclicNormData,
                        modelUpd.yCyclicNormDataPercentiles);
      }else{
        modelUpd.validFitting=false;
      }
    };
    //==========================================================================
    static void fitExponentialGrowthModel(
                  const std::vector< double > &x,
                  const std::vector< double > &y,
                  double maxProportionOfNegativeAtoi,
                  EmpiricalGrowthModel &modelUpd){

      //Remove the bias on x
      if(x.size()==y.size() && x.size() > 2){
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
          if(y[i]<1.0){
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
      }else{
        modelUpd.validFitting=false;
      }

    };

    //==========================================================================
    static void extractFundamentalDataMetricGrowthRates(                  
                  const nlohmann::ordered_json &fundamentalData,
                  const char* reportChapter,
                  const char* reportSection,
                  const char* timePeriod,
                  const char* fieldName,
                  MetricGrowthDataSet &metricGrowthRateUpd,
                  const FinancialAnalysisFunctions::AnalysisDates &analysisDates,
                  int indexLastCommonDate,
                  const EmpiricalGrowthSettings &settings)
    {

      //int maxDayError,
      //double growthIntervalInYears,
      //double maxPropOfOutliersInExpModel,
      //double minCycleTimeInYears,
      //double minR2ImprovementOfCyclicalModel,
      //bool calcOneGrowthRateForAllData,
      //bool includeTimeUnitInAddress,
      //int empiricalModelType

      int indexDate       = -1;
      bool validDate    = true;
      bool forceZeroSlopeOnLinearModel =false;    



      //A value of 0.1 means a 10% preference for an exponential model
      //over a linear model. In this case the exponential model will still
      //be chosen even if its R2 value is 0.1 lower than the linear model.
      double preferenceForAnExponentialModel = 
        settings.exponentialModelR2Preference;  

      std::vector < double > dateNumV;    //Fractional year
      std::vector < std::string > dateV;  //string yer
      std::vector < double > valueV;       //after tax operating income 

      double maxYearError = settings.maxDateErrorInDays / DateFunctions::DAYS_PER_YEAR;

      std::vector < std::string > metricDatesV;
      std::vector < double > metricDatesNumV;

      if(settings.includeTimeUnitInAddress){
        for(auto &el : fundamentalData[reportChapter][reportSection][timePeriod]){
          std::string date;
          JsonFunctions::getJsonString(el["date"],date);
          double dateNum = DateFunctions::convertToFractionalYear(date);

          metricDatesV.push_back(date);
          metricDatesNumV.push_back(dateNum);
        }
      }else{
        for(auto &el : fundamentalData[reportChapter][reportSection]){
          std::string date;
          JsonFunctions::getJsonString(el["date"],date);
          double dateNum = DateFunctions::convertToFractionalYear(date);

          metricDatesV.push_back(date);
          metricDatesNumV.push_back(dateNum);
        }
      }



      //Extract only the valid data in the desired format: Q, Y, or TTM
      while( (indexDate+1) < indexLastCommonDate){
        ++indexDate;
        validDate=true;


        std::string dateCommon = analysisDates.common[indexDate];
        double dateCommonNum = 
          DateFunctions::convertToFractionalYear(dateCommon);
        
        //Check to see if this date is in the list of metric dates
        std::string date("");
        double dateNum=0.;
        bool found=false;
        size_t i=0;
        while(i<metricDatesNumV.size() && !found){
          if(std::abs(metricDatesNumV[i]-dateCommonNum) <= maxYearError){
            date = metricDatesV[i];
            dateNum=metricDatesNumV[i];
            found=true;
          }
          ++i;
        }
        if(!found){
          validDate=false;
        }

        //
        //Eliminate dates in dateSetTTM that are not in the metric date set
        //


        if(validDate){         
          bool setNansToMissingValue=true;
          double value = std::nan("1");
          if(settings.includeTimeUnitInAddress){
            value = 
              JsonFunctions::getJsonFloat(fundamentalData[reportChapter]
                  [reportSection][timePeriod][date][fieldName],
                  setNansToMissingValue);
          }else{
            value = 
              JsonFunctions::getJsonFloat(fundamentalData[reportChapter]
                  [reportSection][date][fieldName],
                  setNansToMissingValue);

          }

          if(JsonFunctions::isJsonFloatValid(value)){
            bool dateEntryValid = JsonFunctions::isJsonFloatValid(dateNum);
            if(dateEntryValid){          
              dateV.push_back(date);
              dateNumV.push_back(dateNum);
              valueV.push_back(value);
            }
          }                                                       
        }

      }

      //
      // Extract the growth values for an interval of length 
      // growthIntervalInYears
      //
      indexDate = -1;
      validDate=true;
      int indexDateMax = static_cast<int>(dateV.size());

      while( (indexDate+1) < indexLastCommonDate 
              && indexDate < indexDateMax 
              && dateV.size() >= 2
              && validDate
              && ((settings.calcOneGrowthRateForAllData && indexDate == -1) 
                    || !settings.calcOneGrowthRateForAllData)){

        ++indexDate;

        std::vector< double > dateSubV; //date sub vector
        std::vector< double > valueSubV; // after-tax operating income sub vector

        //
        // Extract the sub interval
        //
        dateSubV.push_back(dateNumV[indexDate]);
        valueSubV.push_back(valueV[indexDate]);

        int indexDateStart=indexDate+1;
        bool foundStartDate=false;

        while(  !foundStartDate 
              && indexDateStart < indexLastCommonDate 
              && indexDateStart < indexDateMax){
            
            double timeSpan      = dateSubV[0] - dateNumV[indexDateStart]; 
            double timeSpanError = timeSpan-settings.growthIntervalInYears;

            if( timeSpanError < maxYearError ){
              dateSubV.push_back(dateNumV[indexDateStart]);
              valueSubV.push_back(valueV[indexDateStart]);
            }else{
              foundStartDate = true;
            }       
            ++indexDateStart;
        }

        //
        // Fit each of the models to the data and choose the most appropriate
        // model using the following criteria
        //
        //  Linear model, if R2 linear > R2 exponential
        //  Linear + cyclical model, if R2 linear+cyclical >= linear + 0.10
        //  Exponential model, if R2 exponential > R2 linear
        //  Exponential + cyclical model, if R2 exp.+cyclical >= exp. + 0.10
        //

        if(    dateSubV.size() >= 2 
           && (dateSubV.size()==valueSubV.size())
           && (foundStartDate || settings.calcOneGrowthRateForAllData)){
        
          //Order dateSubV so that it proceeds from the earliest date to the 
          //latest date
          if(dateSubV.front() > dateSubV.back()){
            std::reverse(dateSubV.begin(),dateSubV.end());
            std::reverse(valueSubV.begin(),valueSubV.end());
          }

          EmpiricalGrowthModel empModel;
          bool validFitting = false;

          if(settings.typeOfEmpiricalModel==-1){
            EmpiricalGrowthModel exponentialModel, linearModel;

            fitExponentialGrowthModel(dateSubV,valueSubV,
                        settings.maxOutlierProportionInEmpiricalModel,
                        exponentialModel);

            fitLinearGrowthModel(dateSubV,valueSubV,forceZeroSlopeOnLinearModel,
                                linearModel);

            validFitting = 
              (exponentialModel.validFitting || linearModel.validFitting);

            int modelType = -1;
            int linearModelType = 
              static_cast<int>(EmpiricalGrowthModelTypes::LinearModel);
            int exponentialModelType = 
              static_cast<int>(EmpiricalGrowthModelTypes::ExponentialModel);
            

            //A linear model is used unless the exponential model
            //is both valid and has a higher R2
            if(exponentialModel.validFitting==true){
              double exponentialModelR2Upd = 
                exponentialModel.r2
                + preferenceForAnExponentialModel;
              if(exponentialModelR2Upd > linearModel.r2 
                  || !linearModel.validFitting){
                modelType=static_cast<int>(
                    EmpiricalGrowthModelTypes::ExponentialModel);
              }
            }else if(linearModel.validFitting){
              modelType = 
                static_cast<int>(EmpiricalGrowthModelTypes::LinearModel);
            }

            switch(modelType){
              case static_cast<int>(EmpiricalGrowthModelTypes::ExponentialModel):{
                fitCyclicalModelWithExponentialBaseline(
                  dateSubV,
                  valueSubV,
                  settings.minCycleDurationInYears,
                  settings.maxOutlierProportionInEmpiricalModel,
                  empModel);                
              } break;
              case static_cast<int>(EmpiricalGrowthModelTypes::LinearModel):{
                fitCyclicalModelWithLinearBaseline(
                  dateSubV,
                  valueSubV,
                  settings.minCycleDurationInYears,
                  forceZeroSlopeOnLinearModel,
                  empModel);
              } break;
            };

          }else{
            switch(settings.typeOfEmpiricalModel){
              case 0:
              {
                fitExponentialGrowthModel(dateSubV,valueSubV,
                        settings.maxOutlierProportionInEmpiricalModel,empModel);
                validFitting=empModel.validFitting;
              }break;
              case 1:
              {                
                fitCyclicalModelWithExponentialBaseline(
                  dateSubV,
                  valueSubV,
                  settings.minCycleDurationInYears,
                  settings.maxOutlierProportionInEmpiricalModel,
                  empModel);
                validFitting=empModel.validFitting;
              }break;
              case 2:
              {
                fitLinearGrowthModel(dateSubV,valueSubV,
                    forceZeroSlopeOnLinearModel, empModel);
                validFitting=empModel.validFitting;
              }break;
              case 3:
              {
                fitCyclicalModelWithLinearBaseline(
                  dateSubV,
                  valueSubV,
                  settings.minCycleDurationInYears,
                  forceZeroSlopeOnLinearModel,
                  empModel);
                  validFitting=empModel.validFitting;
              }break;
              default:
                std::cout <<"Error: settings.typeOfEmpiricalModel must be [0,1,2,3]"
                          <<std::endl;
                std::abort();
            };
          }

          if(validFitting){

            //
            // Store the model results
            //
            if(valueSubV.size() > 0 && dateSubV.size() > 0){

              metricGrowthRateUpd.dates.push_back(dateV[indexDate]);
              metricGrowthRateUpd.datesNumerical.push_back(dateNumV[indexDate]);
              metricGrowthRateUpd.metricGrowthRate.push_back(
                empModel.annualGrowthRateOfTrendline);
              metricGrowthRateUpd.metricValue.push_back(valueSubV.back());
              metricGrowthRateUpd.model.push_back(empModel);
            } 
          } 
        }
      }
    };

    //==========================================================================
    static void extractEmpiricalAfterTaxOperatingIncomeGrowthRates(
          EmpiricalGrowthDataSet &empiricalGrowthDataUpd,
          const nlohmann::ordered_json &fundamentalData,
          const std::vector< double > &taxRateRecord,
          const FinancialAnalysisFunctions::AnalysisDates &analysisDates,
          std::string &timePeriod,
          int indexLastCommonDate,  
          bool quarterlyTTMAnalysis,
          bool approximateReinvestmentRate,
          const EmpiricalGrowthSettings &settings)
    {

      //    int maxDayErrorTTM,
      //    double growthIntervalInYears,
      //    double maxPropOfOutliersInExpModel,
      //    double minCycleTimeInYears,
      //    double minR2ImprovementOfCyclicalModel,
      //    bool calcOneGrowthRateForAllData,
      //    int empiricalModelType,
      //    bool approximateReinvestmentRate

      int indexDate       = -1;
      bool validDateSet    = true;
      bool forceZeroSlopeOnLinearModel =false;    

      //A value of 0.1 means a 10% preference for an exponential model
      //over a linear model. In this case the exponential model will still
      //be chosen even if its R2 value is 0.1 lower than the linear model.
      double preferenceForAnExponentialModel = 
        settings.exponentialModelR2Preference;  

      std::vector < double > dateNumV;    //Fractional year
      std::vector < std::string > dateV;  //string yer
      std::vector < double > atoiV;       //after tax operating income 
      std::vector < double > rrV;         //reinvestment rate

      std::string previousTimePeriod("");
      std::vector< std::string > previousDateSet;
      std::vector< double > previousDateSetWeight;

      double maxYearErrorTTM = settings.maxDateErrorInDays / DateFunctions::DAYS_PER_YEAR;

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
                                                    settings.maxDateErrorInDays); 
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
                                                    settings.maxDateErrorInDays); 
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

            bool ignoreDepreciation=true;
            double capitalExpenditures =
              FinancialAnalysisFunctions::
                calcNetCapitalExpenditures( fundamentalData, 
                                            dateSetTTM,
                                            previousDateSet,
                                            timePeriod.c_str(),
                                            appendTermRecord,
                                            parentName,
                                            setNansToMissingValue,
                                            ignoreDepreciation,
                                            termNames,
                                            termValues);            

            ignoreDepreciation=false;
            double netCapitalExpenditures = 
            FinancialAnalysisFunctions::
              calcNetCapitalExpenditures( fundamentalData, 
                                          dateSetTTM,
                                          previousDateSet,
                                          timePeriod.c_str(),
                                          appendTermRecord,
                                          parentName,
                                          setNansToMissingValue,
                                          ignoreDepreciation,
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
            if(approximateReinvestmentRate){
              rrEntry = (capitalExpenditures)
                        /atoiEntry;
            }

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
              && ((settings.calcOneGrowthRateForAllData && indexDate == -1) 
                    || !settings.calcOneGrowthRateForAllData)){

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
            double timeSpanError = timeSpan-settings.growthIntervalInYears;

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
           && (foundStartDate || settings.calcOneGrowthRateForAllData)){

          //Order dateSubV so that it proceeds from the earliest date to the 
          //latest date
          if(dateSubV.front() > dateSubV.back()){
            std::reverse(dateSubV.begin(),dateSubV.end());
            std::reverse(atoiSubV.begin(),atoiSubV.end());
          }



          EmpiricalGrowthModel empModel;
          bool validFitting = false;
          
          if(settings.typeOfEmpiricalModel==-1){
            EmpiricalGrowthModel exponentialModel, linearModel;

            fitExponentialGrowthModel(dateSubV,atoiSubV,
                        settings.maxOutlierProportionInEmpiricalModel,
                        exponentialModel);

            fitLinearGrowthModel(dateSubV,atoiSubV,forceZeroSlopeOnLinearModel,
                                linearModel);

            validFitting = 
              (exponentialModel.validFitting || linearModel.validFitting);

            int modelType = -1;
            int linearModelType = 
              static_cast<int>(EmpiricalGrowthModelTypes::LinearModel);
            int exponentialModelType = 
              static_cast<int>(EmpiricalGrowthModelTypes::ExponentialModel);
            

            //A linear model is used unless the exponential model
            //is both valid and has a higher R2
            if(exponentialModel.validFitting==true){
              double exponentialModelR2Upd = 
                exponentialModel.r2
                + preferenceForAnExponentialModel;
              if(exponentialModelR2Upd > linearModel.r2 
                  || !linearModel.validFitting){
                modelType=static_cast<int>(
                    EmpiricalGrowthModelTypes::ExponentialModel);
              }
            }else if(linearModel.validFitting){
              modelType = 
                static_cast<int>(EmpiricalGrowthModelTypes::LinearModel);
            }

            switch(modelType){
              case static_cast<int>(EmpiricalGrowthModelTypes::ExponentialModel):{
                fitCyclicalModelWithExponentialBaseline(
                  dateSubV,
                  atoiSubV,
                  settings.minCycleDurationInYears,
                  settings.maxOutlierProportionInEmpiricalModel,
                  empModel);                
              } break;
              case static_cast<int>(EmpiricalGrowthModelTypes::LinearModel):{
                fitCyclicalModelWithLinearBaseline(
                  dateSubV,
                  atoiSubV,
                  settings.minCycleDurationInYears,
                  forceZeroSlopeOnLinearModel,
                  empModel);
              } break;
            };

          }else{
            switch(settings.typeOfEmpiricalModel){
              case 0:
              {
                fitExponentialGrowthModel(dateSubV,atoiSubV,
                        settings.maxOutlierProportionInEmpiricalModel,empModel);
                validFitting=empModel.validFitting;
              }break;
              case 1:
              {                
                fitCyclicalModelWithExponentialBaseline(
                  dateSubV,
                  atoiSubV,
                  settings.minCycleDurationInYears,
                  settings.maxOutlierProportionInEmpiricalModel,
                  empModel);
                validFitting=empModel.validFitting;
              }break;
              case 2:
              {
                fitLinearGrowthModel(dateSubV,atoiSubV,
                    forceZeroSlopeOnLinearModel, empModel);
                validFitting=empModel.validFitting;
              }break;
              case 3:
              {
                fitCyclicalModelWithLinearBaseline(
                  dateSubV,
                  atoiSubV,
                  settings.minCycleDurationInYears,
                  forceZeroSlopeOnLinearModel,
                  empModel);
                  validFitting=empModel.validFitting;
              }break;
              default:
                std::cout <<"Error: settings.typeOfEmpiricalModel must be [0,1,2,3]"
                          <<std::endl;
                std::abort();
            };

          }

          

          if(validFitting){
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
      }     

    };
    //==========================================================================
    static void appendEmpiricalGrowthModelRecent(
        nlohmann::ordered_json &jsonStruct,
        const EmpiricalGrowthModel &growthModel,
        const std::string nameToPrepend)
    {
      if(growthModel.validFitting){
          std::string fieldName = nameToPrepend+"modelType";
          jsonStruct[fieldName] = growthModel.modelType;

          fieldName = nameToPrepend+"duration";
          jsonStruct[fieldName] = growthModel.duration;

          fieldName = nameToPrepend+"annualGrowthRateOfTrendline";
          jsonStruct[fieldName] = growthModel.annualGrowthRateOfTrendline;

          fieldName = nameToPrepend+"r2";
          jsonStruct[fieldName] = growthModel.r2;

          fieldName = nameToPrepend+"r2Trendline";
          jsonStruct[fieldName] = growthModel.r2Cyclic;

          fieldName = nameToPrepend+"r2Cyclic";
          jsonStruct[fieldName] = growthModel.r2Trendline;

          fieldName = nameToPrepend+"validFitting";
          jsonStruct[fieldName] = growthModel.validFitting;

          fieldName = nameToPrepend+"outlierCount";
          jsonStruct[fieldName] = growthModel.outlierCount;

          int indexRecent=0;
          int indexLast = static_cast<int>(growthModel.x.size())-1;

          if(growthModel.x[indexLast] > growthModel.x[0]){
            indexRecent = indexLast;
          }

          fieldName = nameToPrepend+"yCyclicNormDataPercentilesRecent";
          jsonStruct[fieldName] = 
            growthModel.yCyclicNormDataPercentiles[indexRecent];

          fieldName = nameToPrepend+"parameters";  
          jsonStruct[fieldName] = growthModel.parameters;

          fieldName = nameToPrepend+"x";
          jsonStruct[fieldName] = growthModel.x;
          
          fieldName = nameToPrepend+"y";
          jsonStruct[fieldName] = growthModel.y;

          fieldName = nameToPrepend+"yTrendline";
          jsonStruct[fieldName] = growthModel.yTrendline;

          fieldName = nameToPrepend+"yCyclic";
          jsonStruct[fieldName] = growthModel.yCyclic;

          fieldName = nameToPrepend+"yCyclicData";
          jsonStruct[fieldName] = growthModel.yCyclicData;

          fieldName = nameToPrepend+"yCyclicNorm";
          jsonStruct[fieldName] = growthModel.yCyclicNorm;

          fieldName = nameToPrepend+"yCyclicNormData";
          jsonStruct[fieldName] = growthModel.yCyclicNormData;

          fieldName = nameToPrepend+"yCyclicNormDataPercentiles";
          jsonStruct[fieldName] = growthModel.yCyclicNormDataPercentiles;
        }
    };
    //==========================================================================
    static void appendMetricGrowthModelRecent(
        nlohmann::ordered_json &jsonStruct,
        const MetricGrowthDataSet &metricDataSet,
        const std::string nameToPrepend)
    {

      if(metricDataSet.datesNumerical.size()>0){
        int index = NumericalFunctions::getIndexOfMostRecentDate(metricDataSet);      

        if(metricDataSet.model[index].validFitting){

          /*
          size_t indexRecentEntry = 0;
          if(metricDataSet.model[index].x.front() 
            < metricDataSet.model[index].x.back()){
              indexRecentEntry = 
                metricDataSet.model[index].x.size()-1;
          }
          */

        appendEmpiricalGrowthModelRecent(
          jsonStruct,
          metricDataSet.model[index],
          nameToPrepend);
        //metricDataSet.model[indexRecentEntry],
        }
      }

    };

    //==========================================================================
    static void appendEmpiricalGrowthModelRecent(
        nlohmann::ordered_json &jsonStruct,
        const EmpiricalGrowthDataSet &empiricalDataSet,
        const std::string nameToPrepend)
    {

      if(empiricalDataSet.datesNumerical.size()>0){
        int index = NumericalFunctions::getIndexOfMostRecentDate(empiricalDataSet);      

        if(empiricalDataSet.model[index].validFitting){

          /*
          size_t indexRecentEntry = 0;
          size_t indexLastEntry  = empiricalDataSet.datesNumerical.size()-1;
          if(empiricalDataSet.datesNumerical[indexRecentEntry] 
            < empiricalDataSet.datesNumerical[indexLastEntry]){
              indexRecentEntry = indexLastEntry;
          }
          */

        appendEmpiricalGrowthModelRecent(
          jsonStruct,
          empiricalDataSet.model[index],
          nameToPrepend);
          //empiricalDataSet.model[indexRecentEntry],
          
        }
      }

    };


    //==========================================================================
    static void appendMetricGrowthDataSetRecentDate(
        const MetricGrowthDataSet &metricGrowthData,        
        const std::string nameToPrepend,
        double maxDateErrorInYearsInEmpiricalData,
        std::vector< std::string > &termNames,
        std::vector< double > &termValues)
    {
      
      if(metricGrowthData.datesNumerical.size()>0){

        double date = metricGrowthData.datesNumerical[0];

        if( metricGrowthData.datesNumerical.back() > date){
          date = metricGrowthData.datesNumerical.back();
        }

        appendMetricGrowthDataSet(
                date,
                metricGrowthData,
                nameToPrepend,
                maxDateErrorInYearsInEmpiricalData,
                termNames,
                termValues);
              
      }
    };

    //==========================================================================
    static void appendMetricGrowthDataSet(
        double dateInYears,   
        const MetricGrowthDataSet &metricGrowthData,        
        const std::string nameToPrepend,
        double maxDateErrorInYearsInEmpiricalData,
        std::vector< std::string > &termNames,
        std::vector< double > &termValues)
    {

      if(metricGrowthData.datesNumerical.size()>0){

        size_t index = 
        NumericalFunctions::getIndexOfMetricGrowthDataSet(
                              dateInYears,
                              maxDateErrorInYearsInEmpiricalData,
                              metricGrowthData);

        double dateError = metricGrowthData.datesNumerical[index]
                          -dateInYears;
        if(std::abs(dateError) < maxDateErrorInYearsInEmpiricalData){

          termNames.push_back(nameToPrepend+"date");
          termValues.push_back(metricGrowthData.datesNumerical[index]);

          termNames.push_back(nameToPrepend+"value");
          termValues.push_back(metricGrowthData.metricValue[index]);

          termNames.push_back(nameToPrepend+"growth");
          termValues.push_back(metricGrowthData.metricGrowthRate[index]);

          termNames.push_back(nameToPrepend+"r2");
          termValues.push_back(metricGrowthData.model[index].r2);

          termNames.push_back(nameToPrepend+"r2Trendline");
          termValues.push_back(metricGrowthData.model[index].r2Trendline);

          termNames.push_back(nameToPrepend+"r2Cyclic");
          termValues.push_back(metricGrowthData.model[index].r2Cyclic);

          termNames.push_back(nameToPrepend+"ModelType");
          termValues.push_back(metricGrowthData.model[index].modelType);
        }
      }
    };

    //==========================================================================
    static void appendEmpiricalAfterTaxOperatingIncomeGrowthDataSet(
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