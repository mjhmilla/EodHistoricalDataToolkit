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

#include "DataStructures.h"
#include "DateFunctions.h"
#include "FinancialAnalysisFunctions.h"



class NumericalFunctions {

  public:

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
                      DataStructures::SummaryStatistics &summary){

      bool validSummaryStatistics = true;                      

      if(data.size() > 0){
        std::vector<double> dataCopy(data);
        //for(size_t i=0; i<data.size();++i){
        //  dataCopy.push_back(data[i]);
        //}

        summary.current = std::nan("-1");

        std::sort(dataCopy.begin(),dataCopy.end());

        summary.min = dataCopy[0];
        summary.max = dataCopy[dataCopy.size()-1];
        summary.median=0;
        summary.mean = 0;

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

          double idx = 0.5*(dataCopy.size()-1);
          int indexA = std::floor(idx);
          int indexB = std::ceil(idx);
          double weightB = idx - static_cast<double>(indexA);
          double weightA = 1.0-weightB;
          double valueA = dataCopy[indexA];
          double valueB = dataCopy[indexB];
          summary.median = valueA*weightA + valueB*weightB;           

          for(size_t i=0; i<dataCopy.size();++i){
            summary.mean+=dataCopy[i];
          }
          summary.mean = summary.mean / static_cast<double>(dataCopy.size());

        }else{
          for(size_t i = 0; i < PercentileIndices::NUM_PERCENTILES; ++i){
            summary.percentiles.push_back(dataCopy[0]);
          }
          summary.mean = dataCopy[0];
          summary.median=dataCopy[0];
          validSummaryStatistics=false;
        }
      }else{
        validSummaryStatistics=false;
      }
      return validSummaryStatistics;
    };
    //==========================================================================
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
    //==========================================================================
    static int getIndexOfMostRecentDate(
           const DataStructures::EmpiricalGrowthDataSet &empiricalGrowthData){

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

    //==========================================================================
    static int getIndexOfMostRecentDate(
                const DataStructures::MetricGrowthDataSet &metricGrowthData){
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


    //==========================================================================
    static size_t getIndexOfMetricGrowthDataSet(
              double dateTarget, 
              double maxDateTargetError,
              const DataStructures::MetricGrowthDataSet &metricGrowthData)
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


    //==========================================================================
    static size_t getIndexOfEmpiricalGrowthDataSet(
              double dateTarget, 
              double maxDateTargetError,
              const DataStructures::EmpiricalGrowthDataSet &empiricalGrowthData)
    {

      bool found = false;
      size_t index = 0;
      while(!found && index < empiricalGrowthData.datesNumerical.size()){
        double dateError = dateTarget 
                          - empiricalGrowthData.datesNumerical[index];
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
                  DataStructures::EmpiricalGrowthModel &modelUpd){


      if(x.size()==y.size() && x.size()>2){
        std::vector< double > xN(x.size());
        std::vector< double > yM(y.size());

        //copy over y
        for(size_t i=0; i<y.size();++i){
          yM[i]=y[i];
        }

        double xSpan = std::abs(x.back()-x.front());
        if(xSpan < 0){
          std::cout << "Error: data must be ordered "
                    << "from oldest to most recent"
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
                  DataStructures::EmpiricalGrowthModel &modelUpd){

      if(x.size() == y.size() && x.size() > 2){
        DataStructures::EmpiricalGrowthModel linearModel;
        fitLinearGrowthModel(x,y,forceZeroSlope,linearModel);

        //Subtract off the base line then call            
        std::vector<double> yC(y.size());
        for(size_t i=0; i<yC.size();++i){
          yC[i] = y[i] - linearModel.y[i];
        }

        DataStructures::EmpiricalGrowthModel cyclicalModel;
        fitCyclicalModel(x,yC,minTimeResolutionInYears,cyclicalModel);

        //Form and evaluate the linear+cyclical model
        std::vector<double> yM(y.size());
        for(size_t i=0; i<yM.size();++i){
          yM[i] = linearModel.y[i] + cyclicalModel.y[i];
        }

        double r2LC = calcR2(yM,y);

        modelUpd.duration     = std::abs(x.back()-x.front());
        modelUpd.annualGrowthRateOfTrendline   
          = linearModel.annualGrowthRateOfTrendline;
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
                  DataStructures::EmpiricalGrowthModel &modelUpd){

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
            

        std::vector< double > yMdl(x.size());   
        for(size_t i=0; i<w.size(); ++i){
          yMdl[i]= y0Mdl + dydwMdl*w[i];
        }
        
        double r2 = calcR2(yMdl,y);

        //Evaluate an equivalent growth rate using the last year of data
        double yMdlExp1 = yMdl[yMdl.size()-1];
        size_t i=yMdl.size()-2;

        double durationExp = w[w.size()-1]-w[i];
        while(durationExp < 1.0 && i > 0){
          --i;
          durationExp = w[w.size()-1]-w[i];
        }
        double yMdlExp0 = yMdl[i];
        

        //double expGrowthEq  = 
        //  std::exp(std::log(yMdlExp1/yMdlExp0)/durationExp)-1.0;

        // This linear formula agrees with the exponential growth formula
        // for sensible values, but in addition, will function as 
        // expected for negative values, and negative growth rates.
        double growth = (((yMdlExp1-yMdlExp0)/durationExp)
                        /( std::max(1e-6, std::abs(yMdlExp0))));


        modelUpd.annualGrowthRateOfTrendline=growth;
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
                  DataStructures::EmpiricalGrowthModel &modelUpd){

      if(x.size() == y.size() && x.size() > 2){
        DataStructures::EmpiricalGrowthModel exponentialModel;
        fitExponentialGrowthModel(x,y,maxProportionOfNegativeAtoi,
                                  exponentialModel);

        //Subtract off the base line then call            
        std::vector<double> yC(y.size());
        for(size_t i=0; i<yC.size();++i){
          yC[i] = y[i] - exponentialModel.y[i];
        }

        DataStructures::EmpiricalGrowthModel cyclicalModel;
        fitCyclicalModel(x,yC,minTimeResolutionInYears,cyclicalModel);

        //Form and evaluate the linear+cyclical model
        std::vector<double> yM(y.size());
        for(size_t i=0; i<yM.size();++i){
          yM[i] = exponentialModel.y[i] + cyclicalModel.y[i];
        }

        double r2LC = calcR2(yM,y);


        modelUpd.duration   = std::abs(x.back()-x.front());
        modelUpd.annualGrowthRateOfTrendline 
          = exponentialModel.annualGrowthRateOfTrendline;
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
                  DataStructures::EmpiricalGrowthModel &modelUpd){

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

          //Fit with all of the data
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

          /*
          std::cout << std::endl << "w" << std::endl;
          for(size_t i=0; i<w.size();++i){
            std::cout << w[i] << std::endl;
          }

          std::cout << std::endl << "y" << std::endl;
          for(size_t i=0; i<y.size();++i){
            std::cout << y[i] << std::endl;
          }

          std::cout << std::endl << "yMdl" << std::endl;
          for(size_t i=0; i<yMdl.size();++i){
            std::cout << yMdl[i] << std::endl;
          }
          */


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
    static void extractDividendInfo(  
              const nlohmann::ordered_json &fundamentalData,
              const nlohmann::ordered_json &historicalData,
              const DataStructures::AnalysisDates &analysisDates,
              const char *timePeriod,
              const char *timePeriodOS,
              int yearsToAverageFCFLessDividends,
              DataStructures::DividendInfo &dividendInfoUpd)
    {

      if(std::strcmp(timePeriod,Y)!=0 || std::strcmp(timePeriodOS,A)!=0){
        std::cout << "Error: extractDividendInfo can only be applied to yearly "
                     "data. Set the fields analysisDates, timePeriod, "
                     "timePeriodOS appropriately"
                  << std::endl;
        std::abort();                  
      }
      if(yearsToAverageFCFLessDividends < 1){
        std::cout << "Error: yearsToAverageFCFLessDividends must be >= 1. "
                  << "William Priest uses 3 years, so this should be a good"
                  << "place to start."
                  << std::endl;
      }

      bool setNansToMissingValue    = false;
      bool ignoreNans               = true;
      bool includeTimeUnitInAddress = true;

      int count = 0;
      int countDividendIncrease=0;
      int countDividendCancelled=0;
      int countFcfPositive = 0;
      int countFcfLessDividendsPositive=0;

      double meanDividendYield = 0;
      double meanFreeCashFlowYield = 0;
      double meanFreeCashFlowLessDividendsYield = 0;
      double meanDividendPayoutRatio = 0;
      double meanDividendFreeCashFlowRatio = 0;

      dividendInfoUpd.clear();

      double previousDividend = 0;

      size_t indexDateMax = analysisDates.common.size()
                           -static_cast<size_t>(yearsToAverageFCFLessDividends);

      for(size_t indexDate = 0; indexDate < indexDateMax; 
                              ++indexDate){

        std::string date              
          = analysisDates.common[indexDate];        
        double dateNumerical          
          = DateFunctions::convertToFractionalYear(date);

        std::string datePrevious      
          = analysisDates.common[indexDate+1];        
        double dateNumericalPrevious  
          = DateFunctions::convertToFractionalYear(datePrevious);


        int indexHistoricalData = analysisDates.indicesHistorical[indexDate];


        // Go and get dividendsPaid, the stock price, freeCashFlow
        double dividendsPaid = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][CF][timePeriod][date]["dividendsPaid"],
          setNansToMissingValue);

        if(std::isnan(dividendsPaid)){
          dividendsPaid=0.;
        }

        double freeCashFlow = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][CF][timePeriod][date]["freeCashFlow"],
          setNansToMissingValue);

        double netIncome = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][CF][timePeriod][date]["netIncome"],
          setNansToMissingValue);

        
        double dividendPayoutRatio = dividendsPaid/netIncome;
        double dividendFreeCashFlowRatio = dividendsPaid/freeCashFlow;
        //
        // Evaluate all yield values for this year
        //
        double stockPrice = 
          JsonFunctions::getJsonFloat(
            historicalData[indexHistoricalData]["adjusted_close"],
            setNansToMissingValue);

        double outstandingShares = 
          FinancialAnalysisFunctions::getOutstandingSharesClosestToDate(
              fundamentalData,
              date,
              timePeriodOS);
                  
        double dividendYield = 
                  (dividendsPaid/outstandingShares)/stockPrice;
        double freeCashFlowYield = 
                  (freeCashFlow/outstandingShares)/stockPrice;
        double freeCashFlowLessDividendsYield = 
                  ((freeCashFlow-dividendsPaid)/outstandingShares)/stockPrice;

        if(!std::isnan(dividendYield)){
          meanDividendYield                   += dividendYield;
        }
        if(!std::isnan(freeCashFlowYield)){
          meanFreeCashFlowYield               += freeCashFlowYield;
        }
        if(!std::isnan(freeCashFlowLessDividendsYield)){
          meanFreeCashFlowLessDividendsYield  += freeCashFlowLessDividendsYield;
        }
        if(!std::isnan(dividendPayoutRatio)){
          meanDividendPayoutRatio             += dividendPayoutRatio;
        }
        if(!std::isnan(dividendFreeCashFlowRatio)){
          meanDividendFreeCashFlowRatio       += dividendFreeCashFlowRatio;
        }

        //
        // Get the trailing average of dividends paid and free cash flow
        //
        double dividendsTrailing                  = 0.;
        double freeCashFlowTrailing               = 0.;
        double netIncomeTrailing                  = 0.;

        for(size_t j=0; 
            j < static_cast<size_t>(yearsToAverageFCFLessDividends); ++j){

          std::string dateEntry = analysisDates.common[indexDate+j];               

          double dividendsPaidEntry = JsonFunctions::getJsonFloat(
            fundamentalData[FIN][CF][timePeriod][dateEntry]["dividendsPaid"],
            setNansToMissingValue);

          if(std::isnan(dividendsPaidEntry)){
            dividendsPaidEntry=0.;
          }              

          dividendsTrailing += dividendsPaidEntry;

          double netIncomeEntry = JsonFunctions::getJsonFloat(
            fundamentalData[FIN][CF][timePeriod][dateEntry]["netIncome"],
            setNansToMissingValue);

          netIncomeTrailing += netIncomeEntry;

          double freeCashFlowEntry = JsonFunctions::getJsonFloat(
            fundamentalData[FIN][CF][timePeriod][dateEntry]["freeCashFlow"],
            setNansToMissingValue);

          if(std::isnan(freeCashFlowEntry)){
            freeCashFlowEntry=0.;
          }              

          freeCashFlowTrailing += freeCashFlowEntry;

        }

        double freeCashFlowLessDividendsTrailing = freeCashFlowTrailing
                                                  -dividendsTrailing;

        double dividendPayoutRatioTrailing = dividendsTrailing 
                                           / netIncomeTrailing;   
                                           
        double dividendFreeCashFlowRatioTrailing = dividendsTrailing
                                              / freeCashFlowTrailing;                                           
        //
        // Evaluate the trailing average of fcf, dividends, and fcf-dividends
        //                                                  
        double freeCashFlowAvg = freeCashFlowTrailing
                        /static_cast<double>(yearsToAverageFCFLessDividends);

        double dividendsAvg = dividendsTrailing
                        /static_cast<double>(yearsToAverageFCFLessDividends);

        double freeCashFlowLessDividendsAvg = freeCashFlowAvg-dividendsAvg;


        //
        // Check the previous dividend to see if there was an increase
        //
        double dividendsPaidPrevious = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][CF][timePeriod][datePrevious]["dividendsPaid"],
          setNansToMissingValue);

        if(std::isnan(dividendsPaidPrevious)){
          dividendsPaidPrevious=0.;
        }


        //
        // Evaluate all of the trailing average yield values
        //

        double freeCashFlowYieldTrailingAverageEntry = 
            (freeCashFlowAvg/outstandingShares)/stockPrice;

        double freeCashFlowLessDividendsYieldTrailingAverageEntry = 
          (freeCashFlowLessDividendsAvg/outstandingShares)/stockPrice;

        //
        // Update the vector fields of dividendInfoUpd
        //
        dividendInfoUpd.dates.push_back(date);
        dividendInfoUpd.datesNumerical.push_back(dateNumerical);
        dividendInfoUpd.dividendsPaid.push_back(dividendsPaid);

        dividendInfoUpd.stockPrice.push_back(stockPrice);
        dividendInfoUpd.dividendYield.push_back(dividendYield);
        dividendInfoUpd.dividendPayoutRatio.push_back(dividendPayoutRatio);
        dividendInfoUpd.dividendFreeCashFlowRatio.push_back(dividendFreeCashFlowRatio);

        dividendInfoUpd.freeCashFlowTrailing.push_back(freeCashFlowTrailing);
        dividendInfoUpd.dividendsTrailing.push_back(dividendsTrailing);
        dividendInfoUpd.freeCashFlowLessDividendsTrailing.push_back(
                                      freeCashFlowLessDividendsTrailing);

        dividendInfoUpd.dividendPayoutRatioTrailingAverage.push_back(
                                      dividendPayoutRatioTrailing);
        dividendInfoUpd.dividendFreeCashFlowRatioTrailingAverage.push_back(
                                      dividendFreeCashFlowRatioTrailing);

        dividendInfoUpd.freeCashFlowYieldTrailingAverage.push_back(
                                      freeCashFlowYieldTrailingAverageEntry);
        dividendInfoUpd.freeCashFlowLessDividendsYieldTrailingAverage.push_back(
                            freeCashFlowLessDividendsYieldTrailingAverageEntry);

        //
        // Update the counts
        //
        if(dividendsPaid > 0.){
          ++dividendInfoUpd.yearsWithADividend;
        }else{
          ++countDividendCancelled;
        }
        if(dividendsPaid > dividendsPaidPrevious && dividendsPaidPrevious > 0){
          ++countDividendIncrease;
        }
        if(freeCashFlowYieldTrailingAverageEntry > 0.){
          ++countFcfPositive;
        }
        if(freeCashFlowLessDividendsYieldTrailingAverageEntry > 0.){
          ++countFcfLessDividendsPositive;
        }
        ++count;
      }

      dividendInfoUpd.fractionOfYearsWithDividendIncreases = 
        static_cast<double>(countDividendIncrease)
        /static_cast<double>(count);

      dividendInfoUpd.fractionOfYearsWithDividends = 
        static_cast<double>(dividendInfoUpd.yearsWithADividend)
        /static_cast<double>(count);

      dividendInfoUpd.fractionOfYearsWithCancelledDividends = 
        static_cast<double>(countDividendCancelled)
        /static_cast<double>(count);

      dividendInfoUpd.fractionOfYearsWithPositiveFreeCashFlowTrailing = 
        static_cast<double>(countFcfPositive)
        /static_cast<double>(count);

      dividendInfoUpd.fractionOfYearsWithPositiveFreeCashFlowLessDividendsTrailing = 
        static_cast<double>(countFcfLessDividendsPositive)
        /static_cast<double>(count);


      dividendInfoUpd.meanDividendYield = meanDividendYield 
                                            /static_cast<double>(count); 

      dividendInfoUpd.meanFreeCashFlowYield = meanFreeCashFlowYield 
                                            /static_cast<double>(count); 

      dividendInfoUpd.meanFreeCashFlowLessDividendsYield = 
                                        meanFreeCashFlowLessDividendsYield 
                                        /static_cast<double>(count); 

      dividendInfoUpd.meanDividendPayoutRatio = meanDividendPayoutRatio 
                                            /static_cast<double>(count); 

      dividendInfoUpd.meanDividendFreeCashFlowRatio 
                  = meanDividendFreeCashFlowRatio 
                    /static_cast<double>(count); 

    };
    //==========================================================================
    static void extractFinancialRatios(
              const nlohmann::ordered_json &fundamentalData,
              const nlohmann::ordered_json &historicalData,
              const DataStructures::AnalysisDates &analysisDates,
              const std::string &timePeriod,
              const std::string &timePeriodOS,
              int maximumDayErrorTTM,
              int maximumDayErrorOutstandingShareData,
              bool quarterlyTTMAnalysis,              
              DataStructures::FinancialRatios &financialRatiosUpd)
    {
      bool setNansToMissingValue = false;

      std::vector< std::string > dateSetEarnings;

      for(auto& el : fundamentalData[EARN][HIST]){
        std::string dateEarnings("");
        JsonFunctions::getJsonString(el["date"],dateEarnings);
        dateSetEarnings.push_back(dateEarnings);
      }      

      for(size_t indexDate = 0; indexDate <analysisDates.common.size(); 
                              ++indexDate){

        std::string date     = analysisDates.common[indexDate];
        double datesNumerical = DateFunctions::convertToFractionalYear(date);

        // 
        // Get the date of the previous year
        //
        std::string datePreviousYear("");
        int indexDatePrevYear=0;
        int smallestDateDifference=std::numeric_limits<int>::max();
        int daysPerYearInt= static_cast<int>(DateFunctions::DAYS_PER_YEAR);

        for(int i=indexDate; i<analysisDates.common.size(); ++i){

          int dateDifference = 
            DateFunctions::calcDifferenceInDaysBetweenTwoDates(
              date,"%Y-%m-%d",analysisDates.common[i],"%Y-%m-%d");

          if(std::abs(dateDifference-daysPerYearInt)<smallestDateDifference 
              && dateDifference >= 0){
            smallestDateDifference=std::abs(dateDifference-daysPerYearInt);
            datePreviousYear = analysisDates.common[i];
            indexDatePrevYear = i;
          }
        }

        //
        // Go and get the TTM and previous TTM date sets
        //
        DateFunctions::DateSetTTM dateSetTTM;      
        bool dateSetTTMValid = 
          DateFunctions::extractTTM(
                      indexDate,
                      analysisDates.common,
                      "%Y-%m-%d",
                      dateSetTTM,
                      maximumDayErrorTTM,
                      quarterlyTTMAnalysis);   

        if( !(dateSetTTMValid) ){
          break;
        }       

        DateFunctions::DateSetTTM dateSetPrevTTM;

        bool dateSetPrevTTMValid = 
          DateFunctions::extractTTM(
                        indexDatePrevYear,
                        analysisDates.common,
                        "%Y-%m-%d",
                        dateSetPrevTTM,
                        maximumDayErrorTTM,
                        quarterlyTTMAnalysis);    
                        

        if( !dateSetPrevTTMValid){
          break;
        }  

        //
        //Go get the number of outstanding shares reported from the closest date
        //
        double outstandingShares = std::nan("1");
        smallestDateDifference=std::numeric_limits<int>::max();        
        std::string closestDate("");

        for(auto& el : fundamentalData[OS][timePeriodOS.c_str()]){
          std::string dateOS("");
          JsonFunctions::getJsonString(el["dateFormatted"],dateOS);         
          int dateDifference = 
            DateFunctions::calcDifferenceInDaysBetweenTwoDates(
              date,"%Y-%m-%d",dateOS,"%Y-%m-%d");
          if(   std::abs(dateDifference)<smallestDateDifference 
             && dateDifference >= 0){
            closestDate = dateOS;
            smallestDateDifference=std::abs(dateDifference);
            outstandingShares = JsonFunctions::getJsonFloat(el["shares"]);
          }
        }

        if(smallestDateDifference < maximumDayErrorOutstandingShareData){
          //
          //Get the financial ratios for this date
          //
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
 

          //
          //Evaluate P/E
          //         
          
          // 1. Extract the list of dates needed to identify the TTM
          //
          //
          //

          closestDate.clear();
          smallestDateDifference=std::numeric_limits<int>::max();

          int indexEarningsClosestDate=0;

          for(int i=0; i<dateSetEarnings.size(); ++i){
            int dateDifference = 
              DateFunctions::calcDifferenceInDaysBetweenTwoDates(
                date,"%Y-%m-%d",dateSetEarnings[i],"%Y-%m-%d");
            if(std::abs(dateDifference)<smallestDateDifference){
              smallestDateDifference=std::abs(dateDifference);
              closestDate = dateSetEarnings[i];
              indexEarningsClosestDate = i;
            }
          }

          int indexEarningsPrevYear=0;
          smallestDateDifference=std::numeric_limits<int>::max();

          for(int i=0; i<dateSetEarnings.size(); ++i){
            int dateDifference = 
              DateFunctions::calcDifferenceInDaysBetweenTwoDates(
                date,"%Y-%m-%d",dateSetEarnings[i],"%Y-%m-%d");
            if(std::abs(dateDifference-365)<smallestDateDifference 
              && dateDifference >= 0){
              smallestDateDifference=std::abs(dateDifference-365);
              closestDate = dateSetEarnings[i];
              indexEarningsPrevYear = i;
            }
          }

          //
          // Go get the TTM date sets for earnings
          //
          DateFunctions::DateSetTTM dateSetTTMEarnings;


          bool dateSetTTMValidEarnings = 
            DateFunctions::extractTTM(
                        indexEarningsClosestDate,
                        dateSetEarnings,
                        "%Y-%m-%d",
                        dateSetTTMEarnings,
                        maximumDayErrorTTM,
                        quarterlyTTMAnalysis);   

          if( !(dateSetTTMValidEarnings) ){
            break;
          }       


          //2. Evaluate the epsActual and PE across the TTM using the reported
          //   values and the GAAP values

          bool includeTimeUnit  = false;
          bool ignoreNans       = false;

          double epsActualTTM = 
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
                fundamentalData,EARN,HIST,"",dateSetTTMEarnings,
                "epsActual",setNansToMissingValue,
                 includeTimeUnit,ignoreNans);

          double peTTM = adjustedClosePrice/epsActualTTM;

          includeTimeUnit = true;
          ignoreNans      = false;

          double netIncomeTTM= 
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,CF,timePeriod.c_str(),dateSetTTM,
              "netIncome",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          includeTimeUnit = true;
          ignoreNans      = true;

          double dividendsPaidTTM=
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,CF,timePeriod.c_str(),dateSetTTM,
              "dividendsPaid",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          double epsGaapTTM = (netIncomeTTM-dividendsPaidTTM)/outstandingShares; 
          double peGaapTTM = marketCapitalization/(netIncomeTTM-dividendsPaidTTM);          

          double dividendYield = dividendsPaidTTM / marketCapitalization;

          //3. Evaluate the operating leverage
          // https://www.investopedia.com/terms/d/degreeofoperatingleverage.asp

          includeTimeUnit = true;
          ignoreNans      = false;

          double revenueTTM= 
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,IS,timePeriod.c_str(),dateSetTTM,
              "totalRevenue",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          double operatingIncomeTTM= 
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,IS,timePeriod.c_str(),dateSetTTM,
              "operatingIncome",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          double fcfTTM= 
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,CF,timePeriod.c_str(),dateSetTTM,
              "freeCashFlow",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          double earningsTTM = (netIncomeTTM-dividendsPaidTTM);

          //
          // Evaluate all of the same quantities for the previous time period
          //

          includeTimeUnit = true;
          ignoreNans      = false;

          double netIncomePreviousTTM= 
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,CF,timePeriod.c_str(),dateSetPrevTTM,
              "netIncome",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          includeTimeUnit = true;
          ignoreNans      = true;

          double dividendsPaidPreviousTTM=
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,CF,timePeriod.c_str(),dateSetPrevTTM,
              "dividendsPaid",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          includeTimeUnit = true;
          ignoreNans      = false;

          double revenuePreviousTTM= 
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,IS,timePeriod.c_str(),dateSetPrevTTM,
              "totalRevenue",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          double operatingIncomePreviousTTM= 
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,IS,timePeriod.c_str(),dateSetPrevTTM,
              "operatingIncome",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          double fcfPreviousTTM = 
            FinancialAnalysisFunctions::sumFundamentalDataOverDates(
              fundamentalData,FIN,CF,timePeriod.c_str(),dateSetPrevTTM,
              "freeCashFlow",setNansToMissingValue,
              includeTimeUnit, ignoreNans);

          double earningsPreviousTTM = (netIncomePreviousTTM-dividendsPaidPreviousTTM);
                    
          //
          // Evaluate the change between the current and previous time period
          //

          double operatingIncomeChange  = (operatingIncomeTTM
                                          /operatingIncomePreviousTTM)-1.0;
          
          double fcfChange              = (fcfTTM/fcfPreviousTTM)-1.0;

          double earningsChange         = (earningsTTM/earningsPreviousTTM)-1.0;

          double revenueChange          = (revenueTTM/revenuePreviousTTM)-1.0;



          double operationalLeverage = operatingIncomeChange/revenueChange;

          double fcfLeverage = fcfChange/revenueChange;

          double earningsLeverage = earningsChange/revenueChange;

          financialRatiosUpd.dates.push_back(date);
          financialRatiosUpd.datesNumerical.push_back(datesNumerical);
          
          financialRatiosUpd.adjustedClosePrice.push_back(adjustedClosePrice);
          financialRatiosUpd.outstandingShares.push_back(outstandingShares);
          financialRatiosUpd.marketCapitalization.push_back(
                             marketCapitalization);

          financialRatiosUpd.dividendYield.push_back(dividendYield);                             
          financialRatiosUpd.eps.push_back(epsActualTTM);
          financialRatiosUpd.pe.push_back(peTTM);
          financialRatiosUpd.epsGaap.push_back(epsGaapTTM);
          financialRatiosUpd.peGaap.push_back(peGaapTTM);
          
          financialRatiosUpd.operationalLeverage.push_back(operationalLeverage);
          financialRatiosUpd.freeCashFlowLeverage.push_back(fcfLeverage);
          financialRatiosUpd.earningsLeverage.push_back(earningsLeverage);

        }
      }

    };

    //==========================================================================
    static void extractFundamentalDataMetricGrowthRates(                  
                  const nlohmann::ordered_json &fundamentalData,
                  const char* reportChapter,
                  const char* reportSection,
                  const char* timePeriod,
                  const char* fieldName,
                  DataStructures::MetricGrowthDataSet &metricGrowthRateUpd,
                  const DataStructures::EmpiricalGrowthSettings &settings)
    {

      bool forceZeroSlopeOnLinearModel =false;    



      //A value of 0.1 means a 10% preference for an exponential model
      //over a linear model. In this case the exponential model will still
      //be chosen even if its R2 value is 0.1 lower than the linear model.
      double preferenceForAnExponentialModel = 
        settings.exponentialModelR2Preference;  

      std::vector < double > dateNumV;    //Fractional year
      std::vector < std::string > dateV;  //string yer
      std::vector < double > valueV;       //after tax operating income 

      double maxYearError = settings.maxDateErrorInDays
                          / DateFunctions::DAYS_PER_YEAR;

      std::vector < std::string > metricDatesV;
      std::vector < double > metricDatesNumV;

      if(settings.includeTimeUnitInAddress){
        for(auto &el:fundamentalData[reportChapter][reportSection][timePeriod]){
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



      for(size_t i =0; i < metricDatesV.size(); ++i){
        //++indexDate;
        std::string date = metricDatesV[i];
        double dateNum = metricDatesNumV[i];
        

        //if(validDate){         
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
        //}

      }

      //
      // Extract the growth values for an interval of length 
      // growthIntervalInYears
      //
      int indexDate = -1;
      //validDate=true;
      int indexDateMax = static_cast<int>(dateV.size());

      //(indexDate+1) < indexLastCommonDate 

      while(     indexDate < indexDateMax 
              && dateV.size() >= 2
              && ((settings.calcOneGrowthRateForAllData && indexDate == -1) 
                    || !settings.calcOneGrowthRateForAllData)){

        ++indexDate;


        std::vector< double > dateSubV; //date sub vector
        std::vector< double > valueSubV; //after-tax operating income sub vector

        //
        // Extract the sub interval
        //
        dateSubV.push_back(dateNumV[indexDate]);
        valueSubV.push_back(valueV[indexDate]);

        int indexDateStart=indexDate+1;
        bool foundStartDate=false;

        double growthIntervalInYears =settings.growthIntervalInYears;
        if(settings.calcOneGrowthRateForAllData){
          growthIntervalInYears = abs(dateNumV[0]-dateNumV[dateNumV.size()-1]);
        }

        while(  !foundStartDate 
              && indexDateStart < indexDateMax){
            


            double timeSpan      = dateSubV[0] - dateNumV[indexDateStart]; 
            double timeSpanError = timeSpan-growthIntervalInYears;

            if( ((timeSpanError < maxYearError) 
                  && !settings.calcOneGrowthRateForAllData) 
              || (settings.calcOneGrowthRateForAllData 
                  && timeSpan <= settings.growthIntervalInYears) ){
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

          DataStructures::EmpiricalGrowthModel empModel;
          bool validFitting = false;

          if(settings.typeOfEmpiricalModel==-1){
            DataStructures::EmpiricalGrowthModel exponentialModel, linearModel;

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
              
              modelType=static_cast<int>(
                          EmpiricalGrowthModelTypes::ExponentialModel);              
              
              double exponentialModelR2Upd = 
                exponentialModel.r2
                + preferenceForAnExponentialModel;
              if(exponentialModelR2Upd > linearModel.r2 
                  || !linearModel.validFitting){
                modelType=static_cast<int>(
                    EmpiricalGrowthModelTypes::ExponentialModel);
              }else{
                modelType = 
                  static_cast<int>(EmpiricalGrowthModelTypes::LinearModel);  
              }
              
            }else if(linearModel.validFitting){
              modelType = 
                static_cast<int>(EmpiricalGrowthModelTypes::LinearModel);
            }

            switch(modelType){
              case static_cast<int>(
                  EmpiricalGrowthModelTypes::ExponentialModel):{

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
                std::cout <<"Error: settings.typeOfEmpiricalModel"
                          <<" must be [0,1,2,3]"
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
          DataStructures::EmpiricalGrowthDataSet &empiricalGrowthDataUpd,
          const nlohmann::ordered_json &fundamentalData,
          const std::vector< double > &taxRateRecord,
          const DataStructures::AnalysisDates &analysisDates,
          std::string &timePeriod,
          int indexLastCommonDate,  
          bool quarterlyTTMAnalysis,
          bool approximateReinvestmentRate,
          const DataStructures::EmpiricalGrowthSettings &settings)
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
      DateFunctions::DateSetTTM previousDateSet;
      

      double maxYearErrorTTM = settings.maxDateErrorInDays 
                             / DateFunctions::DAYS_PER_YEAR;

      //
      // For all time periods with sufficient data evaluate: after-tax operating
      // income and the reinvestment rate
      //
      while( (indexDate+1) < indexLastCommonDate && validDateSet){
        ++indexDate;

        //The set of dates used for the TTM analysis
        DateFunctions::DateSetTTM dateSetTTM;
        std::vector < double > dateSetTTMWeight;

        validDateSet = 
          DateFunctions::extractTTM( indexDate,
                                      analysisDates.common,
                                      "%Y-%m-%d",
                                      dateSetTTM,
                                      settings.maxDateErrorInDays,
                                      quarterlyTTMAnalysis); 
        if(!validDateSet){
          break;
        }     
        if(quarterlyTTMAnalysis){
          double dateFront = 
            DateFunctions::convertToFractionalYear(dateSetTTM.dates.front());
          double dateBack = 
            DateFunctions::convertToFractionalYear(dateSetTTM.dates.back());

          if(dateBack > dateFront){
            std::cerr << "Error: dateSetTTM is assumed by many functions to go"
                          "from the most recent date at the front to the oldest"
                          "date at the back. " << std::endl;
            std::abort();                         
          }
        }
                      

        //Check if we have enough data to get the previous time period
        int indexPrevious = indexDate+static_cast<int>(dateSetTTM.dates.size());        

        //
        //Fetch the previous TTM
        //

        previousDateSet.clear();

        validDateSet = 
          DateFunctions::extractTTM( indexPrevious,
                                      analysisDates.common,
                                      "%Y-%m-%d",
                                      previousDateSet,
                                      settings.maxDateErrorInDays,
                                      quarterlyTTMAnalysis); 
        if(!validDateSet){
          break;
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

            if(    rrEntryValid   && dateEntryValid 
                && atoiEntryValid && atoiEntry > 0.){
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



          DataStructures::EmpiricalGrowthModel empModel;
          bool validFitting = false;
          
          if(settings.typeOfEmpiricalModel==-1){
            DataStructures::EmpiricalGrowthModel exponentialModel, linearModel;

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

              case static_cast<int>(
                  EmpiricalGrowthModelTypes::ExponentialModel):{

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
                std::cout <<"Error: settings.typeOfEmpiricalModel"
                          <<" must be [0,1,2,3]"
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
    static void calcPriceToValueUsingEarningsPerShareGrowth(
                    const DateFunctions::DateSetTTM &dateSet,
                    const DataStructures::MetricGrowthDataSet &equityGrowthModel,
                    const DataStructures::FinancialRatios &financialRatios,
                    const std::vector<double> &peMarketVariationUpperBound,
                    double discountRate,
                    int numberOfYearsForTerminalValuation,
                    bool appendTermRecord,
                    bool setNansToMissingValue,
                    const std::string &parentName,
                    std::vector< std::string> &termNames,
                    std::vector< double > &termValues)                                      
    {

      double dateNumRecent=
        DateFunctions::convertToFractionalYear(dateSet.dates[0]);

      bool validFinancialRatios = financialRatios.datesNumerical.size() > 0;
      bool validEquityGrowthModel = equityGrowthModel.datesNumerical.size()>0;

      if(validFinancialRatios && validEquityGrowthModel){
        //Go get the corresponding financialRatio index
        int idxFR = DateFunctions::getIndexClosestToDate(
                        dateNumRecent,
                        financialRatios.datesNumerical);
      
        //Go get the corresponding MetricGrowthDataSet index
        int idxGM= DateFunctions::getIndexClosestToDate(
                                  dateNumRecent,
                                  equityGrowthModel.datesNumerical);      


        double eps0 = financialRatios.eps[idxFR];

        DataStructures::SummaryStatistics growthStats;
        extractSummaryStatistics(equityGrowthModel.metricGrowthRate,growthStats);

        DataStructures::SummaryStatistics dividendYieldStats;
        extractSummaryStatistics(financialRatios.dividendYield,dividendYieldStats);

        DataStructures::SummaryStatistics peStats;                           
        extractSummaryStatistics(financialRatios.pe,peStats);

        double growth=equityGrowthModel.metricGrowthRate[idxGM];
        double dividendYield = financialRatios.dividendYield[idxFR];

        if(dividendYield < 0){
          dividendYield = 0.0;
        }

        std::vector<double> cumPresentValue(4); //nominal, less growth, more growth
        std::vector<double> growthVariation(4);
        std::vector<double> dividendYieldVariation(4);
        std::vector<double> peVariation(4);
        std::vector< double > priceToValue(4);



        for(size_t i=0; i<cumPresentValue.size();++i){

          std::string nameMod("");
          switch(i){
            case 0:
              {
                growthVariation[i]        =growth;
                dividendYieldVariation[i] =dividendYield;
                peVariation[i]            =financialRatios.pe[idxFR];

                if(std::isnan(dividendYieldVariation[i])){
                  dividendYieldVariation[i] = 0.;
                }                
                if(appendTermRecord){
                    nameMod="";
                    termNames.push_back(parentName+"sharePrice");
                    termNames.push_back(parentName+"eps");
                    termNames.push_back(parentName+"equityGrowth");
                    termNames.push_back(parentName+"dividendYield");
                    termNames.push_back(parentName+"pe");
                    termNames.push_back(parentName+"discountRate");
                    termNames.push_back(parentName+"years");
                    termValues.push_back(financialRatios.adjustedClosePrice[idxFR]);
                    termValues.push_back(eps0);
                    termValues.push_back(growthVariation[i]);
                    termValues.push_back(dividendYieldVariation[i]);
                    termValues.push_back(peVariation[i]);
                    termValues.push_back(discountRate);
                    termValues.push_back(numberOfYearsForTerminalValuation);
                }
              } break;
            case 1:
              {
                growthVariation[i]        =growthStats.percentiles[P25];
                dividendYieldVariation[i] =dividendYieldStats.percentiles[P25];
                peVariation[i]            =peStats.percentiles[P25];
                if(peMarketVariationUpperBound.size()>=1){
                  if(peVariation[i]> peMarketVariationUpperBound[0]){
                    peVariation[i]=peMarketVariationUpperBound[0];
                  }
                }

                if(std::isnan(dividendYieldVariation[i])){
                  dividendYieldVariation[i] = 0.;
                }                

                if(appendTermRecord){
                    nameMod="_P25";
                    termNames.push_back(parentName+"sharePrice"+nameMod);
                    termNames.push_back(parentName+"eps"+nameMod);
                    termNames.push_back(parentName+"equityGrowth"+nameMod);
                    termNames.push_back(parentName+"dividendYield"+nameMod);
                    termNames.push_back(parentName+"pe"+nameMod);
                    termValues.push_back(financialRatios.adjustedClosePrice[idxFR]);
                    termValues.push_back(eps0);
                    termValues.push_back(growthVariation[i]);
                    termValues.push_back(dividendYieldVariation[i]);
                    termValues.push_back(peVariation[i]);

                }

              } break;
            case 2:
              {
                growthVariation[i]        =growthStats.percentiles[P50];
                dividendYieldVariation[i] =dividendYieldStats.percentiles[P50];
                peVariation[i]            =peStats.percentiles[P50];
                if(peMarketVariationUpperBound .size()>=1){
                  if(peVariation[i]> peMarketVariationUpperBound[1]){
                    peVariation[i]=peMarketVariationUpperBound[1];
                  }
                }

                if(std::isnan(dividendYieldVariation[i])){
                  dividendYieldVariation[i] = 0.;
                }                

                if(appendTermRecord){
                    nameMod="_P50";
                    termNames.push_back(parentName+"sharePrice"+nameMod);
                    termNames.push_back(parentName+"eps"+nameMod);
                    termNames.push_back(parentName+"equityGrowth"+nameMod);
                    termNames.push_back(parentName+"dividendYield"+nameMod);
                    termNames.push_back(parentName+"pe"+nameMod);
                    termValues.push_back(financialRatios.adjustedClosePrice[idxFR]);
                    termValues.push_back(eps0);
                    termValues.push_back(growthVariation[i]);
                    termValues.push_back(dividendYieldVariation[i]);
                    termValues.push_back(peVariation[i]);
                }

              } break;
            case 3:
              {
                growthVariation[i]        =growthStats.percentiles[P75];
                dividendYieldVariation[i] =dividendYieldStats.percentiles[P75];
                peVariation[i]            =peStats.percentiles[P75];
                if(peMarketVariationUpperBound.size()>=1){
                  if(peVariation[i]> peMarketVariationUpperBound[2]){
                    peVariation[i]=peMarketVariationUpperBound[2];
                  }
                }

                if(std::isnan(dividendYieldVariation[i])){
                  dividendYieldVariation[i] = 0.;
                }                


                if(appendTermRecord){
                    nameMod="_P75";
                    termNames.push_back(parentName+"sharePrice"+nameMod);
                    termNames.push_back(parentName+"eps"+nameMod);
                    termNames.push_back(parentName+"equityGrowth"+nameMod);
                    termNames.push_back(parentName+"dividendYield"+nameMod);
                    termNames.push_back(parentName+"pe"+nameMod);
                    termValues.push_back(financialRatios.adjustedClosePrice[idxFR]);
                    termValues.push_back(eps0);
                    termValues.push_back(growthVariation[i]);
                    termValues.push_back(dividendYieldVariation[i]);
                    termValues.push_back(peVariation[i]);
                }

              } break;
            default:
            {
              std::cerr << "Error: invalid case hit in "
                        << "calcPriceToValueUsingEarningsPerShareGrowth"
                        << std::endl;
              std::abort();                      
            }
          }

          //Compute the present value
          cumPresentValue[i] = 0.;
          for (int j=1; j <= numberOfYearsForTerminalValuation;++j){
            double eps = eps0*std::pow(1.0+growthVariation[i],j);
            double dividend = eps*dividendYieldVariation[i];
            double discountFactor= std::pow(1.0+discountRate,j);
            double presentValue = (dividend) / discountFactor;

            cumPresentValue[i] += presentValue;

            //The details are only outputted for the nominal case
            if(appendTermRecord && i == 0){
              std::stringstream ss;
              ss << j;
              termNames.push_back(parentName+"eps"+nameMod+"_"+ss.str());
              termNames.push_back(parentName+"dividend"+nameMod+"_"+ss.str());
              termNames.push_back(parentName+"discount_factor"+nameMod+"_"+ss.str());
              termNames.push_back(parentName+"dividend_present_value"+nameMod+"_"+ss.str());

              termValues.push_back(eps);
              termValues.push_back(dividend);
              termValues.push_back(discountFactor);
              termValues.push_back(presentValue);            
            }
            
          }

          if(appendTermRecord){
            termNames.push_back(parentName+"cumulative_dividend_present_value"+nameMod);
            termValues.push_back(cumPresentValue[i]);
          }

          double epsTerminal = eps0*std::pow(1.0+growthVariation[i],
                                  numberOfYearsForTerminalValuation);

          double terminalValue = (epsTerminal*peVariation[i]);


          double terminalDiscount = 
            std::pow(1.+discountRate,numberOfYearsForTerminalValuation);

          double terminalPresentValue = terminalValue / terminalDiscount;       

          if(appendTermRecord){
            termNames.push_back(parentName+"terminal_eps"+nameMod);
            termValues.push_back(epsTerminal);  
            termNames.push_back(parentName+"terminal_pe"+nameMod);
            termValues.push_back(peVariation[i]);  
            termNames.push_back(parentName+"terminal_value"+nameMod);
            termValues.push_back(terminalValue);  
            termNames.push_back(parentName+"terminal_discount"+nameMod);
            termValues.push_back(terminalDiscount);        
            termNames.push_back(parentName+"terminal_present_value"+nameMod);
            termValues.push_back(terminalPresentValue);
          }


          cumPresentValue[i] += terminalPresentValue;


          if(appendTermRecord){
            termNames.push_back(parentName+"total_present_value"+nameMod);
            termValues.push_back(cumPresentValue[i]);
          }


          priceToValue[i] = 
            financialRatios.adjustedClosePrice[idxFR]/cumPresentValue[i];

          //Compute the price to value ratio
          if(appendTermRecord){
            termNames.push_back(parentName+"price_to_value"+nameMod);
            termValues.push_back(priceToValue[i]);
          }

        }
      }
    };
        
    //==========================================================================
    static void appendEmpiricalGrowthModelRecent(
        nlohmann::ordered_json &jsonStruct,
        const DataStructures::EmpiricalGrowthModel &growthModel,
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
        const DataStructures::MetricGrowthDataSet &metricDataSet,
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
        const DataStructures::EmpiricalGrowthDataSet &empiricalDataSet,
        const std::string nameToPrepend)
    {

      if(empiricalDataSet.datesNumerical.size()>0){
        int index = NumericalFunctions::getIndexOfMostRecentDate(
                                        empiricalDataSet);      

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
        const DataStructures::MetricGrowthDataSet &metricGrowthData,        
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
        const DataStructures::MetricGrowthDataSet &metricGrowthData,        
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
        const DataStructures::EmpiricalGrowthDataSet &empiricalGrowthData,
        double dateInYears,
        double costOfCapitalMature,
        const std::string nameToPrepend,
        std::vector< std::string > &termNames,
        std::vector< double > &termValues)
    {



      termNames.push_back(nameToPrepend+"AfterTaxOperatingIncomeGrowth");
      termValues.push_back(
          empiricalGrowthData.afterTaxOperatingIncomeGrowth[index]);

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

      termNames.push_back(nameToPrepend
                          +"ReturnOnInvestedCapitalLessCostOfCapital");

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