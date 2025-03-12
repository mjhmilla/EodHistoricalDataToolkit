//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef NUMERICAL_TOOLKIT
#define NUMERICAL_TOOLKIT

#include <cmath>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <boost/math/statistics/linear_regression.hpp>



class NumericalToolkit {

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
      std::vector< double > parameters;
      double growthRate;
      double r2;
      bool validFitting;
      int outlierCount;
    };    
    //============================================================================
    struct EmpiricalGrowthDataSet{
      std::vector< std::string > dates;
      std::vector< double > datesNumerical;
      std::vector< double > durationNumerical;      
      //std::vector< double > intervalStartDate;
      std::vector< double > afterTaxOperatingIncomeGrowth;
      std::vector< double > reinvestmentRate;
      std::vector< double > reinvestmentRateSD;
      std::vector< double > returnOnInvestedCapital;
      std::vector< EmpiricalGrowthModel > empiricalModel;
      // std::vector< double > intervalStartingAfterTaxOperatingIncome;
      // std::vector< double > growthModelR2;
      // std::vector< int > typeOfGrowthModel;
      
      //std::vector< int > outlierCount;
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
    //==========================================================================

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
      int invalidEntryCount = 0;
      for(size_t i=0; i< y.size();++i){
        if(y[i]<1){
          w[i]=1.0;
          ++invalidEntryCount;
        }else{
          w[i]=y[i];
        }
      }
      
      double invalidEntryProportion = 
          static_cast<double>(invalidEntryCount)
        / static_cast<double>(y.size());

      if(invalidEntryProportion > maxProportionOfNegativeAtoi){
        exponentialModelUpd.validFitting=false;
      }

      if(exponentialModelUpd.validFitting){

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

};



#endif