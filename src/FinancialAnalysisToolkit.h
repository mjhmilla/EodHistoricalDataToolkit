//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef FINANCIAL_ANALYSIS_TOOLKIT
#define FINANCIAL_ANALYSIS_TOOLKIT

#include <cmath>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <sstream>

#include "date.h"
#include <nlohmann/json.hpp>
#include "JsonFunctions.h"


const char *GEN = "General";
const char *TECH= "Technicals";
const char *FIN = "Financials";
const char *BAL = "Balance_Sheet";
const char *CF  = "Cash_Flow";
const char *IS  = "Income_Statement";
const char *OS  = "outstandingShares";

const char *Y = "yearly";
const char *A = "annual"; //EOD uses annual in the outstandingShares list.
const char *Q = "quarterly";


class FinancialAnalysisToolkit {

  public:


    //==========================================================================
    struct TickerMetricData{
      std::vector< date::sys_days > dates;
      std::string ticker;
      std::vector< std::vector< double > > metrics;
    };

    //==========================================================================
    struct MetricTable{
      date::sys_days dateStart;
      date::sys_days dateEnd;
      std::vector< std::string > tickers;
      std::vector< std::vector< double > > metrics;
      std::vector< std::vector< size_t > > metricRank;
      //std::vector< size_t > metricRankSum;
      //std::vector< size_t > rank;
    };

    //==========================================================================
    static double sumFundamentalDataOverDates(
        const nlohmann::ordered_json &fundamentalData,
        const char* reportChapter,
        const char* reportSection,
        const char* timeUnit,
        const std::vector< std::string > &dates,
        const char* fieldName,        
        bool setNansToMissingValue){

      double value        = 0;
      double sumOfValues  = 0;
      bool valueMissing   = false;
      int numberOfEntries = 0;

      for(auto &ele : dates){

        value = JsonFunctions::getJsonFloat( 
          fundamentalData[reportChapter][reportSection][timeUnit][ele.c_str()]
            [fieldName],false);

        if(!std::isnan(value)){
          sumOfValues += value;
          ++numberOfEntries;
        }

        
      }

      //If TTM data is being processed
      if( numberOfEntries == 0 ){
        if(setNansToMissingValue){
          sumOfValues = JsonFunctions::MISSING_VALUE;
        }else{
          sumOfValues = std::nan("1");
        }
      }

      return sumOfValues;

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
    //==========================================================================
    static int calcIndexOfClosestDateInHistorcalData(
                                    const std::string &targetDate,
                                    const char* targetDateFormat,
                                    nlohmann::ordered_json &historicalData,
                                    const char* dateSetFormat,
                                    bool verbose){

      int indexA = 0;
      int indexB = historicalData.size()-1;

      int indexAError = calcDifferenceInDaysBetweenTwoDates(
                          targetDate, targetDateFormat, 
                          historicalData[indexA]["date"],dateSetFormat);
                          
      int indexBError = calcDifferenceInDaysBetweenTwoDates(
                          targetDate, targetDateFormat, 
                          historicalData[indexB]["date"],dateSetFormat);
      int index      = std::round((indexB+indexA)*0.5);
      int indexError = 0;
      int changeInError = historicalData.size()-1;

      while( std::abs(indexB-indexA)>1 
          && std::abs(indexAError)>0 
          && std::abs(indexBError)>0
          && changeInError > 0){

        int indexError = calcDifferenceInDaysBetweenTwoDates(
                          targetDate, targetDateFormat, 
                          historicalData[index]["date"],dateSetFormat);

        if( indexError*indexAError >= 0){
          indexA = index;
          changeInError = std::abs(indexAError-indexError);
          indexAError=indexError;
        }else if(indexError*indexBError > 0){
          indexB = index;
          changeInError = std::abs(indexBError-indexError);
          indexBError=indexError;
        }
        if(std::abs(indexB-indexA) > 1){
          index      = std::round((indexB+indexA)*0.5);
        }
      }

      if(std::abs(indexAError) <= std::abs(indexBError)){
        return indexA;
      }else{
        return indexB;
      }


    };
    //==========================================================================
    static void getIsinCountryCodes(const std::string& isin, 
                            nlohmann::ordered_json &exchangeList,
                            std::string &isinCountryISO2,
                            std::string &isinCountryISO3){

      isinCountryISO2="";
      isinCountryISO3="";
      bool found=false;
      auto iterExchange=exchangeList.begin();
      while(iterExchange != exchangeList.end() && !found){
        std::string countryISO2 =
          (*iterExchange)["CountryISO2"].get<std::string>();
        std::string countryISO3 =
          (*iterExchange)["CountryISO3"].get<std::string>();
        
        if(  isin.find(countryISO2)==0 
          || isin.find(countryISO3)==0){
          isinCountryISO2=countryISO2;
          isinCountryISO3=countryISO3;
          found=true;
        }
        ++iterExchange;
      }
    };

    //==========================================================================
    static bool doesIsinHaveMatchingExchange(
                  const std::string &isin,
                  nlohmann::ordered_json &exchangeList,
                  const std::string &exchangeSymbolListFolder){

      auto iterExchange=exchangeList.begin();
      bool exists=false;
      while(iterExchange != exchangeList.end() && !exists){

        std::string countryISO2 =
          (*iterExchange)["CountryISO2"].get<std::string>();
        std::string countryISO3 =
          (*iterExchange)["CountryISO3"].get<std::string>();

        if(countryISO2.length()>0 && countryISO3.length()>0){
          if(  isin.find(countryISO2)==0 || isin.find(countryISO3)==0){

            //Check to see if the corresponding file exists
            std::string exchangeCode = 
              (*iterExchange)["Code"].get<std::string>();  
            std::string exchangeSymbolListPath = exchangeSymbolListFolder;
            exchangeSymbolListPath.append(exchangeCode);
            exchangeSymbolListPath.append(".json");
            bool fileExists = 
              std::filesystem::exists(exchangeSymbolListPath.c_str());
            
            if(fileExists){
              exists=true;
            }                   
          }
        }
        ++iterExchange;
      }  
      return exists;
    };

    //==========================================================================
    static bool isExchangeAndIsinConsistent( std::string &exchange,
                                      std::string &isin,
                                      nlohmann::ordered_json &exchangeList){

      auto iterExchange=exchangeList.begin();
      bool consistent=false;
      while(iterExchange != exchangeList.end() && !consistent){

        std::string countryISO2 =
          (*iterExchange)["CountryISO2"].get<std::string>();
        std::string countryISO3 =
          (*iterExchange)["CountryISO3"].get<std::string>();

        if(  isin.find(countryISO2)==0 || isin.find(countryISO3)==0){
          std::string exchangeCode=(*iterExchange)["Code"].get<std::string>();

          if(exchangeCode.compare(exchange) == 0){
            consistent=true;
          }

        }
        ++iterExchange;
      }  
      return consistent;
    };

    //==========================================================================
    static void createEodJsonFileName(const std::string &ticker, 
                                      const std::string &exchangeCode,
                                      std::string &updEodFileName)
    {
      updEodFileName=ticker;
      updEodFileName.append(".");
      updEodFileName.append(exchangeCode);
      updEodFileName.append(".json");
    };


    //==========================================================================
    static double calcReturnOnCapitalDeployed( nlohmann::ordered_json &jsonData, 
                                    std::vector< std::string > &dateSet, 
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    std::string &parentCategoryName,
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      // Return On Capital Deployed
      //  Source: https://www.investopedia.com/terms/r/roce.asp
      double longTermDebt = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]["longTermDebt"], 
        setNansToMissingValue);     

      //Total shareholder equity is the money that would be left over if the 
      //company went out of business immediately. 
      //https://www.investopedia.com/ask/answers/033015/what-does-total-stockholders-equity-represent.asp
      double totalStockholderEquity = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]["totalStockholderEquity"], 
        setNansToMissingValue);

      double operatingIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,IS,timeUnit,dateSet,"operatingIncome",
          setNansToMissingValue);      

      //There are two definitions for capital depoloyed, and they should
      //be the same. These values are not the same for Apple, but are within
      //30% of one and other.  
      //1. capitalDeployed = (longTermDebt+totalStockholderEquity);
      //2. capitalDeployed = (totalAssets-currentTotalLiabilities);

      double returnOnCapitalDeployed = 
        operatingIncome / (longTermDebt+totalStockholderEquity);

      if(appendTermRecord){
        termNames.push_back(parentCategoryName+"returnOnCapitalDeployed_longTermDebt");
        termNames.push_back(parentCategoryName+"returnOnCapitalDeployed_totalStockholderEquity");
        termNames.push_back(parentCategoryName+"returnOnCapitalDeployed_operatingIncome");
        termNames.push_back(parentCategoryName+"returnOnCapitalDeployed");

        termValues.push_back(longTermDebt);
        termValues.push_back(totalStockholderEquity);
        termValues.push_back(operatingIncome);
        termValues.push_back(returnOnCapitalDeployed);
      }

      return returnOnCapitalDeployed;
    };

    //==========================================================================
    static double calcReturnOnInvestedCapital(nlohmann::ordered_json &jsonData, 
                                    std::vector< std::string> &dateSet,
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    std::string &parentCategoryName,
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      // Return On Invested Capital
      //  Source: 
      // https://www.investopedia.com/terms/r/returnoninvestmentcapital.asp
      double longTermDebt = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]
                      ["longTermDebt"], setNansToMissingValue);       

      double totalStockholderEquity = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]
                      ["totalStockholderEquity"], setNansToMissingValue);

      double netIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"netIncome",
          setNansToMissingValue);

      //Interesting fact: dividends paid can be negative. This would have
      //the effect of increasing the ROIC for a misleading reason.

      double dividendsPaid = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"dividendsPaid",
          true);      

      double returnOnInvestedCapital =  
        (netIncome-dividendsPaid) / (longTermDebt+totalStockholderEquity);

      //The ROIC is meaningless unless the denominator is defined.
      //I'm willing to accept dividends paid being missing from the EOD 
      //records because many companys do not issue dividends.
      if(    !JsonFunctions::isJsonFloatValid(longTermDebt) 
          || !JsonFunctions::isJsonFloatValid(totalStockholderEquity)
          || !JsonFunctions::isJsonFloatValid(netIncome)){
        if(setNansToMissingValue){
          returnOnInvestedCapital = JsonFunctions::MISSING_VALUE;
        }else{
          returnOnInvestedCapital = std::nan("1");
        }
      }

      if(appendTermRecord){
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital_longTermDebt");
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital_totalStockholderEquity");
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital_netIncome");
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital_dividendsPaid");
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital");

        termValues.push_back(longTermDebt);
        termValues.push_back(totalStockholderEquity);
        termValues.push_back(netIncome);
        termValues.push_back(dividendsPaid);
        termValues.push_back(returnOnInvestedCapital);
      }

      return returnOnInvestedCapital;
    };

    /**
     * https://www.investopedia.com/terms/r/returnonequity.asp
    */
    static double calcReturnOnEquity(nlohmann::ordered_json &jsonData, 
                                    std::vector< std::string > &dateSet,
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    std::string &parentCategoryName,
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      double totalStockholderEquity = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]
                      ["totalStockholderEquity"], setNansToMissingValue);
      
      double netIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"netIncome",
          setNansToMissingValue);

      double returnOnEquity = netIncome/totalStockholderEquity;

      if(    !JsonFunctions::isJsonFloatValid(netIncome) 
          || !JsonFunctions::isJsonFloatValid(totalStockholderEquity)){
        if(setNansToMissingValue){
          returnOnEquity = JsonFunctions::MISSING_VALUE;
        }else{
          returnOnEquity = std::nan("1");
        }
      }


      if(appendTermRecord){
        termNames.push_back(parentCategoryName+"returnOnEquity_netIncome");
        termNames.push_back(parentCategoryName+"returnOnEquity_totalStockholderEquity");
        termNames.push_back(parentCategoryName+"returnOnEquity");

        termValues.push_back(netIncome);
        termValues.push_back(totalStockholderEquity);
        termValues.push_back(returnOnEquity);

      }

      return returnOnEquity;
    };    

    //==========================================================================
    static double calcRetentionRatio(nlohmann::ordered_json &jsonData, 
                                    std::vector< std::string > &dateSet,
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    std::string &parentCategoryName,
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      // Return On Invested Capital
      //  Source: 
      // https://www.investopedia.com/terms/r/returnoninvestmentcapital.asp

      double netIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"netIncome",
          setNansToMissingValue);

      //Interesting fact: dividends paid can be negative. This would have
      //the effect of increasing the ROIC for a misleading reason.
      //double  dividendsPaid = 
      //  JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
      //                ["dividendsPaid"], setNansToMissingValue);
    
      double dividendsPaid = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"dividendsPaid",
          true);

      double retentionRatio =  
        (netIncome-dividendsPaid) / (netIncome);

      if( !JsonFunctions::isJsonFloatValid(netIncome)){
        if(setNansToMissingValue){
          retentionRatio = JsonFunctions::MISSING_VALUE;
        }else{
          retentionRatio = std::nan("1");
        }
      }

      if(appendTermRecord){
        termNames.push_back(parentCategoryName+"retentionRatio_netIncome");
        termNames.push_back(parentCategoryName+"retentionRatio_dividendsPaid");
        termNames.push_back(parentCategoryName+"retentionRatio");

        termValues.push_back(netIncome);
        termValues.push_back(dividendsPaid);
        termValues.push_back(retentionRatio);

      }

      return retentionRatio;
    };    


    //==========================================================================
    /**
     *Return On Assets
     *Source: https://www.investopedia.com/terms/r/returnonassets.asp 
     * 
    */      
    static double calcReturnOnAssets(nlohmann::ordered_json &jsonData, 
                                     std::vector< std::string > &dateSet,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     double setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      double netIncome =  FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"netIncome",
          setNansToMissingValue);

      double totalAssets = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]
                      ["totalAssets"],setNansToMissingValue);

      //There are two definitions for capital depoloyed, and they should
      //be the same. These values are not the same for Apple, but are within
      //30% of one and other.  
      //1. capitalDeployed = (longTermDebt+totalStockholderEquity);
      //2. capitalDeployed = (totalAssets-currentTotalLiabilities);

      double returnOnAssets= netIncome / totalAssets;

      if(    !JsonFunctions::isJsonFloatValid(netIncome) 
          || !JsonFunctions::isJsonFloatValid(totalAssets)){
        if(setNansToMissingValue){
          returnOnAssets = JsonFunctions::MISSING_VALUE;
        }else{
          returnOnAssets = std::nan("1");
        }
      }

      if(appendTermRecord){
        termNames.push_back("returnOnAssets_netIncome");
        termNames.push_back("returnOnAssets_totalAssets");
        termNames.push_back("returnOnAssets");

        termValues.push_back(netIncome);
        termValues.push_back(totalAssets);
        termValues.push_back(returnOnAssets);
      }

      return returnOnAssets;

    };

    //==========================================================================
    /**
     * Gross margin
     * https://www.investopedia.com/terms/g/grossmargin.asp
    */
    static double calcGrossMargin(nlohmann::ordered_json &jsonData, 
                                     std::vector< std::string > &dateSet,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     bool setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){
      
      double totalRevenue = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,IS,timeUnit,dateSet,
            "totalRevenue", setNansToMissingValue);

      double costOfRevenue = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,IS,timeUnit,dateSet,
            "costOfRevenue", setNansToMissingValue);


      double grossMargin= (totalRevenue-costOfRevenue)/totalRevenue;

      if(    !JsonFunctions::isJsonFloatValid(totalRevenue) 
          || !JsonFunctions::isJsonFloatValid(costOfRevenue)){
        if(setNansToMissingValue){
          grossMargin = JsonFunctions::MISSING_VALUE;
        }else{
          grossMargin = std::nan("1");
        }
      }

      if(appendTermRecord){
        termNames.push_back("grossMargin_totalRevenue");
        termNames.push_back("grossMargin_costOfRevenue");
        termNames.push_back("grossMargin");

        termValues.push_back(totalRevenue);
        termValues.push_back(costOfRevenue);
        termValues.push_back(grossMargin);
      }

      return grossMargin;
    };

    //==========================================================================
    /**
     * https://www.investopedia.com/terms/o/operatingmargin.asp
    */
    static double calcOperatingMargin(nlohmann::ordered_json &jsonData, 
                                     std::vector< std::string > &dateSet,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     bool setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      double operatingIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,IS,timeUnit,dateSet,
            "operatingIncome", setNansToMissingValue);

      double totalRevenue = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,IS,timeUnit,dateSet,
            "totalRevenue", setNansToMissingValue);


      double operatingMargin=operatingIncome/totalRevenue;

      if(    !JsonFunctions::isJsonFloatValid(operatingIncome) 
          || !JsonFunctions::isJsonFloatValid(totalRevenue)){
        if(setNansToMissingValue){
          operatingMargin = JsonFunctions::MISSING_VALUE;
        }else{
          operatingMargin = std::nan("1");
        }
      }      

      if(appendTermRecord){
        termNames.push_back("operatingMargin_operatingIncome");
        termNames.push_back("operatingMargin_totalRevenue");
        termNames.push_back("operatingMargin");

        termValues.push_back(operatingIncome);
        termValues.push_back(totalRevenue);
        termValues.push_back(operatingMargin);
      }

      return operatingMargin;
    };    

    //==========================================================================
    /**
     * https://corporatefinanceinstitute.com/resources/accounting/cash-conversion-ratio/
    */
    static double calcCashConversionRatio(nlohmann::ordered_json &jsonData, 
                                     std::vector< std::string> &dateSet,
                                     const char *timeUnit,
                                     double taxRate,
                                     bool appendTermRecord,
                                     bool setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){
      /*
        Free cash flow is not reported in the 10-Q that I'm working through,
        but it does appear in EOD's data. The formula suggested from 
        investopedia (see link below) produces a value that's very close to
        what appears in EOD. As far as I can tell this is a calculated quantity
        and EOD appears to be calculating it correctly

        + (totalCashFromOperatingActivities) : Cash flow
        + (interestExpense)                  : Income Statement
        - [Tax shield on interest expense]   : Income Statement (couldn't find this)
        - (capitalExpenditures)              : Cash flow
        = freeCashFlow

        From Alphabet 10-Q Q1 2023
        +23,509
        +    80
        -     0
        - 6,300
        = 17,280 ~= 17,220

        https://www.investopedia.com/terms/f/freecashflow.asp

        Alphabet 10-Q Q1 2023
        https://www.sec.gov/Archives/edgar/data/1652044/000165204423000045/goog-20230331.htm

        Wednesday 28 June 2023
        -----------
         Email to EOD
        -----------
        I have been double checking the fields and values reported for one 
        company (Alphabet March 2023 10-Q). The 10-Q form does not contain a 
        reporting of free-cash-flow, yet this is a field within the json data 
        structure and it has a value in it. I've calculated what this should 
        be using a standard formula and it's close but not exactly what is 
        reported. And so I have two questions:
        1. I would like to know how the 'freecashflow' field in the fundamental 
            data is being calculated.
        2. I would like to know in general:
        a. What fields have values that are taken directly from SEC documents?
        b. What fields are calculated?
        c. For each calculated field, please provide the formula that is being 
            used.
        -----------
         Response from EOD
        -----------
        totalCashFromOperatingActivities – capitalExpenditures = freeCashFlow
        (in this case, it’s +, since the figures for capex for GOOG are shown 
        as negative).
        There is no issue for 2023-03-31 for GOOG either. 
        23509000000.00 - 6289000000.00 = 17220000000.00.
        We don’t have a shareable list of fields that are either calculated or 
        obtained. Each case is reviewed individually.
        -----------
        Reflections
        -----------                
        This is a little concerning, as I have no idea whether their figures are
        trustworthy. I would like more transparency in the least. It's a pity
        that SimFin doesn't have better coverage, as their service is remarkably
        transparent. In any case, I'm tempted to avoid fields that don't 
        actually show up in a quaterly or yearly report.
      */

      std::string categoryName("cashFlowConversionRatio_");
      double freeCashFlow =  calcFreeCashFlow(  jsonData,
                                                dateSet,
                                                timeUnit, 
                                                taxRate, 
                                                appendTermRecord,
                                                categoryName, 
                                                setNansToMissingValue,
                                                termNames, 
                                                termValues);

      double netIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,IS,timeUnit,dateSet,
            "netIncome", setNansToMissingValue);


      double cashFlowConversionRatio = (freeCashFlow)/netIncome;

      if(    !JsonFunctions::isJsonFloatValid(netIncome) 
          || !JsonFunctions::isJsonFloatValid(freeCashFlow)){
        if(setNansToMissingValue){
          cashFlowConversionRatio = JsonFunctions::MISSING_VALUE;
        }else{
          cashFlowConversionRatio = std::nan("1");
        }
      }

      if(appendTermRecord){
        termNames.push_back(categoryName+"netIncome");
        termNames.push_back("cashFlowConversionRatio");

        termValues.push_back(netIncome);
        termValues.push_back(cashFlowConversionRatio);
      }

      return cashFlowConversionRatio;

    };

    //==========================================================================
    /**
       https://www.investopedia.com/terms/l/leverageratio.asp
    */
    static double calcDebtToCapitalizationRatio(
                                    nlohmann::ordered_json &jsonData, 
                                    std::vector< std::string> &dateSet,
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

      //Sometimes this is not reported. I would rather have the computation
      //go through as this field is often not large and will get highlighted
      //in the output                                     
      double shortTermDebt = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]["shortTermDebt"],
        true);   

      double longTermDebt =  JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]["longTermDebt"], 
        setNansToMissingValue);  

      double totalStockholderEquity =  JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]["totalStockholderEquity"], 
        setNansToMissingValue);

      double debtToCapitalizationRatio=(shortTermDebt+longTermDebt)
                    /(shortTermDebt+longTermDebt+totalStockholderEquity);


      if(   !JsonFunctions::isJsonFloatValid(longTermDebt)
         || !JsonFunctions::isJsonFloatValid(shortTermDebt)
         || !JsonFunctions::isJsonFloatValid(totalStockholderEquity)){
        
        if(setNansToMissingValue){
          debtToCapitalizationRatio=JsonFunctions::MISSING_VALUE;
        }else{
          debtToCapitalizationRatio = std::nan("1");
        }
      }    

      if(appendTermRecord){
        termNames.push_back("debtToCapitalizationRatio_shortTermDebt");
        termNames.push_back("debtToCapitalizationRatio_longTermDebt");
        termNames.push_back("debtToCapitalizationRatio_totalStockholderEquity");
        termNames.push_back("debtToCapitalizationRatio");

        termValues.push_back(shortTermDebt);
        termValues.push_back(longTermDebt);
        termValues.push_back(totalStockholderEquity);
        termValues.push_back(debtToCapitalizationRatio);
      }

      return debtToCapitalizationRatio;                    
    };

    //==========================================================================
    /**
     https://www.investopedia.com/terms/i/interestcoverageratio.asp
    */
    static double calcInterestCover(const nlohmann::ordered_json &jsonData, 
                                    const std::vector< std::string > &dateSet,
                                    double defaultInterestCover,
                                    const char *timeUnit,                                    
                                    bool appendTermRecord,                                    
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

                                      
      double operatingIncome = 
        sumFundamentalDataOverDates(jsonData,FIN,IS,timeUnit,dateSet,
                                  "operatingIncome",setNansToMissingValue);

      double interestExpense = 
        sumFundamentalDataOverDates(jsonData,FIN,IS,timeUnit,dateSet,
                                  "interestExpense",setNansToMissingValue);

      double interestCover=operatingIncome/interestExpense;

      if(   !JsonFunctions::isJsonFloatValid(operatingIncome)
         || !JsonFunctions::isJsonFloatValid(interestExpense)){
        
          //This will give the company a modest interest cover
          interestCover = defaultInterestCover;        
      }      

      if(appendTermRecord){
        termNames.push_back("interestCover_operatingIncome");
        termNames.push_back("interestCover_interestExpense");
        termNames.push_back("interestCover");

        termValues.push_back(operatingIncome);
        termValues.push_back(interestExpense);
        termValues.push_back(interestCover);
      }

      return interestCover;

    };    

    //==========================================================================
    static double calcDefaultSpread(nlohmann::ordered_json &jsonData, 
                                    std::vector< std::string>  &dateSet,
                                    const char *timeUnit,
                                    double meanInterestCover,
                                    nlohmann::ordered_json &jsonDefaultSpread,
                                    bool appendTermRecord,
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

        double interestCover = FinancialAnalysisToolkit::
          calcInterestCover(jsonData,
                            dateSet,
                            meanInterestCover,
                            timeUnit,
                            appendTermRecord,
                            setNansToMissingValue,
                            termNames,
                            termValues);

        if(!JsonFunctions::isJsonFloatValid(interestCover)){
          interestCover = meanInterestCover;
        }

        double defaultSpread = std::nan("1");
        if(setNansToMissingValue){
          defaultSpread = JsonFunctions::MISSING_VALUE;
        }

        bool found=false;
        unsigned int i=0;        
        int tableSize = jsonDefaultSpread["US"]["default_spread"].size();

        double interestCoverLowestValue = JsonFunctions::getJsonFloat(
              jsonDefaultSpread["US"]["default_spread"].at(0).at(0),
              setNansToMissingValue);
        double interestCoverHighestValue = JsonFunctions::getJsonFloat(
              jsonDefaultSpread["US"]["default_spread"].at(tableSize-1).at(1),
              setNansToMissingValue);

        if(interestCover < interestCoverLowestValue 
          && JsonFunctions::isJsonFloatValid(interestCoverLowestValue)){
          defaultSpread = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(0).at(2),
                setNansToMissingValue);
        
        }else if(interestCover > interestCoverHighestValue
                 && JsonFunctions::isJsonFloatValid(interestCoverHighestValue)){
          defaultSpread = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(tableSize-1).at(2),
                setNansToMissingValue);          
        
        }else{        
          while(found == false 
                && i < tableSize){
            
            double interestCoverLowerBound = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(i).at(0),
                setNansToMissingValue);
            double interestCoverUpperBound = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(i).at(1),
                setNansToMissingValue);
            double defaultSpreadIntervalValue = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(i).at(2),
                setNansToMissingValue);


            if(    JsonFunctions::isJsonFloatValid(interestCoverLowerBound)
                && JsonFunctions::isJsonFloatValid(interestCoverUpperBound)
                && JsonFunctions::isJsonFloatValid(defaultSpreadIntervalValue)){

              if(interestCover >= interestCoverLowerBound
              && interestCover <= interestCoverUpperBound){
                defaultSpread = defaultSpreadIntervalValue;
                found=true;
              }
            }
            ++i;
          } 
        }

        if(std::isnan(defaultSpread) || std::isinf(defaultSpread) 
           || !JsonFunctions::isJsonFloatValid(defaultSpread)){
          std::cerr <<
            "Error: the default spread has evaluated to nan or inf"
            " which is only possible if the default spread json file"
            " has errors in it. The default spread json file should"
            " have 3 columns: interest cover lower bound, interest "
            " cover upper bound, and default spread rate. Each row "
            " specifies an interval. The intervals should be "
            " continuous and cover from -10000 to 10000, likely with"
            " very big intervals at the beginning and end. By default"
            " data/defaultSpreadTable.json contains this information"
            " which comes from "
            " https://pages.stern.nyu.edu/~adamodar/New_Home_Page/datafile/ratings.html" 
            " on January 2023. The 1st and 2nd columns come from the"
            " table in the url, the 3rd column (rating) is ignored"
            " while the final column is put in decimal format "
            "(divide by 100). This table is appropriate for the U.S."
            " and perhaps Europe, but is probably not correct for"
            " the rest of the world." << std::endl;
          std::abort();
        }       

        termNames.push_back("defaultSpread_interestCover");
        termNames.push_back("defaultSpread");
        termValues.push_back(interestCover);
        termValues.push_back(defaultSpread);

        return defaultSpread;
    };


    //==========================================================================
    static double calcFreeCashFlow( nlohmann::ordered_json &jsonData, 
                                    std::vector< std::string> &dateSet,
                                    const char *timeUnit,
                                    double taxRate,
                                    bool appendTermRecord,
                                    std::string parentCategoryName,
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

      
      //Investopedia definition
      //https://www.investopedia.com/terms/f/freecashflow.asp

      double totalCashFromOperatingActivities = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,CF,timeUnit,dateSet,
            "totalCashFromOperatingActivities", setNansToMissingValue);                    

      //Sometimes this is not reported. I would rather this get computed
      double interestExpense = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,IS,timeUnit,dateSet,
            "interestExpense", true);  

      std::string resultName(parentCategoryName);
      resultName.append("freeCashFlow_");                    
                                  
      double taxShieldOnInterestExpense = interestExpense*taxRate;

      double capitalExpenditures = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,CF,timeUnit,dateSet,
            "capitalExpenditures", setNansToMissingValue);    

      double freeCashFlow = 
          totalCashFromOperatingActivities
          + interestExpense
          - taxShieldOnInterestExpense
          - capitalExpenditures;

      double freeCashFlowEOD = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,CF,timeUnit,dateSet,
            "freeCashFlow", setNansToMissingValue); 


      double freeCashFlowReturned = freeCashFlow;
      //I'm willing to accept missing values in interest expense and
      //the tax-shield-on-interest-expense as these values are typically 
      //small in comparison to the other quantities.
      if(   !JsonFunctions::isJsonFloatValid(totalCashFromOperatingActivities)
         || !JsonFunctions::isJsonFloatValid(interestExpense)
         || !JsonFunctions::isJsonFloatValid(taxShieldOnInterestExpense)
         || !JsonFunctions::isJsonFloatValid(capitalExpenditures)){
        
        //If I can't calculate free cash flow, take the value that is
        //reported by EOD, if it is available. 
        freeCashFlowReturned=freeCashFlowEOD; 

        if(setNansToMissingValue){
          freeCashFlow=JsonFunctions::MISSING_VALUE;
        }else{
          freeCashFlow = std::nan("1");
        }
                 
      }

      if(appendTermRecord){
        
        termNames.push_back(resultName + "totalCashFromOperatingActivities");
        termNames.push_back(resultName + "interestExpense");
        termNames.push_back(resultName + "taxShieldOnInterestExpense");
        termNames.push_back(resultName + "capitalExpenditures");
        termNames.push_back(resultName + "freeCashFlow");
        termNames.push_back(resultName + "freeCashFlowEOD");
        termNames.push_back(parentCategoryName+"freeCashFlow");

        termValues.push_back(totalCashFromOperatingActivities);
        termValues.push_back(interestExpense);
        termValues.push_back(taxShieldOnInterestExpense);
        termValues.push_back(capitalExpenditures);
        termValues.push_back(freeCashFlow);
        termValues.push_back(freeCashFlowEOD);
        termValues.push_back(freeCashFlowReturned);
 
      }

      return freeCashFlowReturned;

    };    

    //==========================================================================
    static double calcNetCapitalExpenditures(nlohmann::ordered_json &jsonData, 
                                     std::vector< std::string > &dateSet,
                                     std::vector< std::string > &previousDateSet,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     std::string &parentCategoryName,
                                     bool setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){
      /*
      Problem: the depreciation field in EOD's json data is often null
      Solution: Since Damodran's formula for net capital expenditures is

      net capital expenditures =  capital expenditure - depreciation  [a]

      where capital expendature is the change in plant, property and equipment
      from the previous year we can instead use:

      capital expenditure = PPE - PPE_previousYear

      References
        https://www.investopedia.com/terms/c/capitalexpenditure.asp

        Damodaran, A.(2011). The Little Book of Valuation. Wiley.
      */


      double capitalExpenditures = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"capitalExpenditures",
          setNansToMissingValue);

      double plantPropertyEquipment=0;
      double plantPropertyEquipmentPrevious=0;
      double changeInPlantPropertyEquipment = 0;

      if(!JsonFunctions::isJsonFloatValid(capitalExpenditures)){

        std::vector< std::string > dateSetAnalysis;
        for(size_t i =0; i < dateSet.size(); ++i){
          dateSetAnalysis.push_back(dateSet[i]);
        }
        dateSetAnalysis.push_back(previousDateSet[0]);

        capitalExpenditures=0.;

        for(int indexPrevious=1; 
          indexPrevious < dateSetAnalysis.size();++indexPrevious){

          int index = indexPrevious-1;

          //Use an alternative method to calculate capital expenditures
          plantPropertyEquipment = 
            JsonFunctions::getJsonFloat(
              jsonData[FIN][BAL][timeUnit][dateSetAnalysis[index].c_str()]
              ["propertyPlantEquipment"], setNansToMissingValue);


          plantPropertyEquipmentPrevious = 
            JsonFunctions::getJsonFloat(
              jsonData[FIN][BAL][timeUnit][dateSetAnalysis[indexPrevious].c_str()]
              ["propertyPlantEquipment"], setNansToMissingValue);

          //If one of the PPE's is populated copy it over to the PPE that is
          //nan: this is a better approximation of the PPE than 0.
          if(   JsonFunctions::isJsonFloatValid(plantPropertyEquipment) 
            && JsonFunctions::isJsonFloatValid(plantPropertyEquipmentPrevious)){
            changeInPlantPropertyEquipment +=
                    ( plantPropertyEquipment
                     -plantPropertyEquipmentPrevious);                                         
          } 
          capitalExpenditures  += (changeInPlantPropertyEquipment); 

        }              
      }


      double depreciation = 
          FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,CF,timeUnit,dateSet,"depreciation",
            setNansToMissingValue);

      double netCapitalExpenditures = capitalExpenditures - depreciation;  

      if(     !JsonFunctions::isJsonFloatValid(capitalExpenditures) 
          ||  !JsonFunctions::isJsonFloatValid(depreciation)       ){
        if(setNansToMissingValue){
          netCapitalExpenditures = JsonFunctions::MISSING_VALUE; 
        }else{
          netCapitalExpenditures = std::nan("1");
        }        
      }

      if(appendTermRecord){
        termNames.push_back(parentCategoryName 
          + "netCapitalExpenditures_changeInPlantPropertyEquipment");
        termNames.push_back(parentCategoryName 
          + "netCapitalExpenditures_capitalExpenditures");
        termNames.push_back(parentCategoryName 
          + "netCapitalExpenditures_depreciation");
        termNames.push_back(parentCategoryName 
          + "netCapitalExpenditures");

        termValues.push_back(changeInPlantPropertyEquipment);     
        termValues.push_back(capitalExpenditures);
        termValues.push_back(depreciation);     
        termValues.push_back(netCapitalExpenditures);
      }

      return netCapitalExpenditures;
    };

    //==========================================================================
    static double calcChangeInNonCashWorkingCapital(
                                    nlohmann::ordered_json &jsonData, 
                                    std::vector< std::string > &dateSet,
                                    std::vector< std::string > &previousDateSet,
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    std::string &parentCategoryName,
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      /*
        Problem: Damodran's FCFE needs change in non-cash working capital, but
                 this quantity is not really reported by EOD. 

        Solution: calculate the change in non-cash working capital directly 
                  using the suggestions in Ch. 3 of Damodran

        change non-cash working capital = (inventory-inventoryPrevious)
                                        + (netReceivables-netReceivablesPrevious)
                                        - (accountsPayable-accountsPayablePrevious)

      */

      bool isInputValid=true;

      std::vector< std::string > dateSetAnalysis;
      for(size_t i =0; i < dateSet.size(); ++i){
        dateSetAnalysis.push_back(dateSet[i]);
      }
      dateSetAnalysis.push_back(previousDateSet[0]);
      

      double changeInNonCashWorkingCapital = 0.;
      double changeInInventory          = 0.;
      double changeInReceivables        = 0.;
      double changeInAccountsPayable    = 0.;
      bool changeInInventoryAdded       = false;
      bool changeInReceivablesAdded     = false;
      bool changeInAccountsPayableAdded = false;

      for(int indexPrevious=1; 
          indexPrevious < dateSetAnalysis.size();++indexPrevious){

        int index = indexPrevious-1;

        double inventory =  
          JsonFunctions::getJsonFloat(      
            jsonData[FIN][BAL][timeUnit][dateSetAnalysis[index].c_str()]["inventory"],
            setNansToMissingValue);

        double inventoryPrevious = 
          JsonFunctions::getJsonFloat(  
            jsonData[FIN][BAL][timeUnit][dateSetAnalysis[indexPrevious].c_str()]["inventory"],
            setNansToMissingValue);

        double netReceivables =
          JsonFunctions::getJsonFloat(  
            jsonData[FIN][BAL][timeUnit][dateSetAnalysis[index].c_str()]["netReceivables"],
            setNansToMissingValue);

        double netReceivablesPrevious = 
          JsonFunctions::getJsonFloat(  
            jsonData[FIN][BAL][timeUnit][dateSetAnalysis[indexPrevious].c_str()]["netReceivables"],
            setNansToMissingValue);

        double accountsPayable =
          JsonFunctions::getJsonFloat(  
            jsonData[FIN][BAL][timeUnit][dateSetAnalysis[index].c_str()]["accountsPayable"],
            setNansToMissingValue);

        double accountsPayablePrevious = 
          JsonFunctions::getJsonFloat(  
            jsonData[FIN][BAL][timeUnit][dateSetAnalysis[indexPrevious].c_str()]["accountsPayable"],
            setNansToMissingValue);


        //There are a number of companies that produce software or a service
        //and so never have any inventory to report.
        if(    JsonFunctions::isJsonFloatValid(  inventory)
            && JsonFunctions::isJsonFloatValid(  inventoryPrevious)){
          changeInInventory += (inventory-inventoryPrevious);
          changeInNonCashWorkingCapital += (inventory-inventoryPrevious);
          changeInInventoryAdded       = true;
        }

        if(    JsonFunctions::isJsonFloatValid(  netReceivables         )
            && JsonFunctions::isJsonFloatValid(  netReceivablesPrevious )){
          changeInReceivables += (netReceivables-netReceivablesPrevious);
          changeInNonCashWorkingCapital += (netReceivables-netReceivablesPrevious);
          changeInReceivablesAdded     = true;
        }

        if(    JsonFunctions::isJsonFloatValid(  accountsPayable        )
            && JsonFunctions::isJsonFloatValid(  accountsPayablePrevious)){
          changeInAccountsPayable += -1.0*(accountsPayable-accountsPayablePrevious);       
          changeInNonCashWorkingCapital += -1.0*(accountsPayable-accountsPayablePrevious);
          changeInAccountsPayableAdded = true;
        }

      }
          
      if(appendTermRecord){

        termNames.push_back(parentCategoryName 
        + "changeInNonCashWorkingCapital_changeInInventory");

        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_changeInNetReceivables");

        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_changeInAccountsPayable");

        termValues.push_back(changeInInventory);
        termValues.push_back(changeInReceivables);
        termValues.push_back(changeInAccountsPayable);

        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital");

        termValues.push_back(changeInNonCashWorkingCapital);   
      }  

      return changeInNonCashWorkingCapital;
    };

    /**
     Free cash flow to equity is a quantity that estimates how much money
     could have been returned to investors from a company. This quantity is
     an input needed to value a company following the methodology described
     in Ch. 3 of Damodaran.

     Unfortunately Damodaran's precise formula cannot directly used with EODs 
     data as not all of the necessary fields have been populated. However, this
     is pretty close

    ----------
     From the book                           
     FCFE =  net Income                     | netIncome 
           + depreciation                   | + depreciation
           - cap. expenditures              | - capitalExpenditures
           - change in non-cash capital     | - (otherNonCashItems-otherNonCashItemsPrevious)
           - (principal repaid - new debt)  | + (netDebt-netDebtPrevious)

      Damodaran, A.(2011). The Little Book of Valuation. Wiley.
    */
    static double calcFreeCashFlowToEquity(nlohmann::ordered_json &jsonData, 
                                     std::vector< std::string > &dateSet,
                                     std::vector< std::string > &previousDateSet,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     bool setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      
      std::string parentName = "freeCashFlowToEquity_";
      
      double netIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"netIncome",
          setNansToMissingValue);

      double depreciation = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"depreciation",
          setNansToMissingValue);

      double netCapitalExpenditures = 
        calcNetCapitalExpenditures( jsonData, 
                                    dateSet,
                                    previousDateSet,
                                    timeUnit,
                                    appendTermRecord,
                                    parentName,
                                    setNansToMissingValue,
                                    termNames,
                                    termValues);

      double changeInNonCashWorkingCapital = 
        calcChangeInNonCashWorkingCapital( jsonData, 
                                    dateSet,
                                    previousDateSet,
                                    timeUnit,
                                    appendTermRecord,
                                    parentName,
                                    setNansToMissingValue,
                                    termNames,
                                    termValues);                                    
      /*
        Problem: the change in cash due to debt is not something that EOD 
                 reports, though in the case of 3M 2007 10-K this value is
                 reported. Instead, I'll estimate this quantity by evaluating
                 the change of the sum of short-term and long-term debt. I'm
                 not using net debt, in this case, because  

                 net debt = (short-term debt + long-term debt) 
                          - total current assets

                 includes total current assets which I do not want.
      */

      double shortLongTermDebtTotal = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]["shortLongTermDebtTotal"],
        setNansToMissingValue);

      double shortLongTermDebtTotalPrevious = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][previousDateSet[0].c_str()]
                ["shortLongTermDebtTotal"],setNansToMissingValue);

      double netDebtIssued = (shortLongTermDebtTotal
                             -shortLongTermDebtTotalPrevious);

      //Incase shortLongTermDebtTotal is not available, then there is 
      //we can estimate this quantity using just the cash from long-term debt

      double longTermDebt = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]["longTermDebt"],
        setNansToMissingValue);

      double longTermDebtPrevious = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][previousDateSet[0].c_str()]
                ["longTermDebt"],setNansToMissingValue);

      double netDebtIssuedAlternative = (longTermDebt
                                        -longTermDebtPrevious);


      if(   JsonFunctions::isJsonFloatValid(shortLongTermDebtTotal) 
         && JsonFunctions::isJsonFloatValid(shortLongTermDebtTotalPrevious)){       
          //Zero the alternative calculation (not needed)
          netDebtIssuedAlternative  = 0.;          
          longTermDebt              = 0.;
          longTermDebtPrevious      = 0.;
      }else{

          //Set any nan values to JsonFunctions::MISSING_VALUE
          netDebtIssued             = 0.;

          if(   !JsonFunctions::isJsonFloatValid(longTermDebt) 
             || !JsonFunctions::isJsonFloatValid(longTermDebtPrevious)){
            netDebtIssuedAlternative = JsonFunctions::MISSING_VALUE;
          }        
      }


      double freeCashFlowToEquity = 
          netIncome
          + (depreciation)
          - (netCapitalExpenditures)
          - (changeInNonCashWorkingCapital)
          + netDebtIssued
          + netDebtIssuedAlternative;

      //I'm willing to accept a missing changeInNonCashWorkingCapital as
      //this value is often missing from Eod's records, and often does not
      //amount to much in comparision to the other quantities.      
      if(   !JsonFunctions::isJsonFloatValid(netIncome) 
         || !JsonFunctions::isJsonFloatValid(depreciation)
         || !JsonFunctions::isJsonFloatValid(netCapitalExpenditures)
         || (   !JsonFunctions::isJsonFloatValid(netDebtIssued) 
             && !JsonFunctions::isJsonFloatValid(netDebtIssuedAlternative))){
        if(setNansToMissingValue){
          freeCashFlowToEquity = JsonFunctions::MISSING_VALUE;
        }else{
          freeCashFlowToEquity = std::nan("1");
        }
      }


      if(appendTermRecord){
        termNames.push_back("freeCashFlowToEquity_netIncome");
        termNames.push_back("freeCashFlowToEquity_shortLongTermDebtTotal");
        termNames.push_back("freeCashFlowToEquity_shortLongTermDebtTotalPrevious");
        termNames.push_back("freeCashFlowToEquity_netDebtIssued");
        termNames.push_back("freeCashFlowToEquity_longTermDebtTotal");
        termNames.push_back("freeCashFlowToEquity_longTermDebtTotalPrevious");
        termNames.push_back("freeCashFlowToEquity_netDebtIssuedAlternative");

        termNames.push_back("freeCashFlowToEquity");

        termValues.push_back(netIncome);
        termValues.push_back(shortLongTermDebtTotal); 
        termValues.push_back(shortLongTermDebtTotalPrevious); 
        termValues.push_back(netDebtIssued);
        termValues.push_back(longTermDebt); 
        termValues.push_back(longTermDebtPrevious); 
        termValues.push_back(netDebtIssuedAlternative);

        termValues.push_back(freeCashFlowToEquity);
      }

      return freeCashFlowToEquity;
    };

    //==========================================================================
    /**
     Warren Buffets definition for owners earnings is a bit more conservative
     than free-cash-flow-to-equity because the cash flow created by debt is
     not included.
     * */
    static double calcOwnersEarnings(nlohmann::ordered_json &jsonData, 
                                     std::vector< std::string> &dateSet,
                                     std::vector< std::string> &previousDateSet,
                                     const char *timeUnit,   
                                     bool appendTermRecord,
                                     bool setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

    
      //Definition from Ch. 3 of Damodaran (page 40/172 22%)     
      //Damodaran (2011). The little book of valuation    

      double netIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"netIncome",
          setNansToMissingValue);

      std::string parentName = "ownersEarnings_";

      double netCapitalExpenditures = 
        calcNetCapitalExpenditures( jsonData, 
                                    dateSet,
                                    previousDateSet,
                                    timeUnit,
                                    appendTermRecord,
                                    parentName,
                                    setNansToMissingValue,
                                    termNames,
                                    termValues);

      double changeInNonCashWorkingCapital = 
        calcChangeInNonCashWorkingCapital( jsonData, 
                                    dateSet,
                                    previousDateSet,
                                    timeUnit,
                                    appendTermRecord,
                                    parentName,
                                    setNansToMissingValue,
                                    termNames,
                                    termValues);  

      double ownersEarnings =  
          netIncome
          - netCapitalExpenditures
          - changeInNonCashWorkingCapital;     

      //I'm willing to accept a missing changeInNonCashWorkingCapital as
      //this is often missing from Eod's data and is not large compared to the
      //other quantities
      if(   !JsonFunctions::isJsonFloatValid(netIncome) 
         || !JsonFunctions::isJsonFloatValid(netCapitalExpenditures)){
        if(setNansToMissingValue){
          ownersEarnings = JsonFunctions::MISSING_VALUE;
        }else{
          ownersEarnings = std::nan("1");
        }
      }           

      if(appendTermRecord){
        termNames.push_back("ownersEarnings_netIncome");
        termNames.push_back("ownersEarnings");

        termValues.push_back(netIncome);
        termValues.push_back(ownersEarnings);
      }

      return ownersEarnings;
    };


    //==========================================================================
    /*
    static double calcTaxRateFromTheTaxProvision(
                                     nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     std::string &parentCategoryName,
                                     bool setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      double taxProvision = JsonFunctions::getJsonFloat(
        jsonData[FIN][IS][timeUnit][date.c_str()]["taxProvision"], setNansToMissingValue);

      double incomeBeforeTaxes =  JsonFunctions::getJsonFloat(
        jsonData[FIN][IS][timeUnit][date.c_str()]["incomeBeforeTax"], setNansToMissingValue);

      double taxRate = taxProvision/incomeBeforeTaxes;

      if(     !JsonFunctions::isJsonFloatValid(taxProvision)
          ||  !JsonFunctions::isJsonFloatValid(incomeBeforeTaxes)){
        if(setNansToMissingValue){
          taxRate=JsonFunctions::MISSING_VALUE;
        }else{
          taxRate=std::nan("1");
        }
      }

      //Update the argument and result record
      if(appendTermRecord){

        termNames.push_back(parentCategoryName + "taxRate_taxProvision");
        termNames.push_back(parentCategoryName + "taxRate_incomeBeforeTax");
        termNames.push_back(parentCategoryName + "taxRate");

        termValues.push_back(taxProvision);
        termValues.push_back(incomeBeforeTaxes);
        termValues.push_back(taxRate);
      }

      return taxRate;
    };
    */

    //==========================================================================
    static double calcReinvestmentRate(nlohmann::ordered_json &jsonData, 
                                     std::vector< std::string > &dateSet,
                                     std::vector< std::string > &previousDateSet,  
                                     const char *timeUnit,
                                     double taxRate,
                                     bool appendTermRecord,
                                     std::string &parentCategoryName,
                                     bool setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

    
      //Damodaran definition (page 40/172 22%)     
      
      double operatingIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,IS,timeUnit,dateSet,"operatingIncome",
          setNansToMissingValue);


      std::string parentName = parentCategoryName;
      parentName.append("reinvestmentRate_");

      double afterTaxOperatingIncome = 
        operatingIncome*(1.0-taxRate);

      double netCapitalExpenditures = 
        calcNetCapitalExpenditures( jsonData, 
                                    dateSet,
                                    previousDateSet,
                                    timeUnit,
                                    appendTermRecord,
                                    parentName,
                                    setNansToMissingValue,
                                    termNames,
                                    termValues);

      double changeInNonCashWorkingCapital = 
        calcChangeInNonCashWorkingCapital( jsonData, 
                                    dateSet,
                                    previousDateSet,
                                    timeUnit,
                                    appendTermRecord,
                                    parentName,
                                    setNansToMissingValue,
                                    termNames,
                                    termValues);          

      double reinvestmentRate =  
        (netCapitalExpenditures+changeInNonCashWorkingCapital
        )/afterTaxOperatingIncome;

      //I'm allowing missing values for changeInNonCashWorkingCapital to
      //be accepted because this field is often not reported ... and 
      //probably does not account.
      if(     !JsonFunctions::isJsonFloatValid(netCapitalExpenditures)
          ||  !JsonFunctions::isJsonFloatValid(afterTaxOperatingIncome)){
        if(setNansToMissingValue){
          reinvestmentRate=JsonFunctions::MISSING_VALUE;
        }else{
          reinvestmentRate=std::nan("1");
        }
      }

      if(appendTermRecord){

        termNames.push_back(parentCategoryName 
          + "reinvestmentRate_operatingIncome");
        termNames.push_back(parentCategoryName 
          + "reinvestmentRate_taxRate");
        termNames.push_back(parentCategoryName 
          + "reinvestmentRate_afterTaxOperatingIncome");
        termNames.push_back(parentCategoryName 
          + "reinvestmentRate");

        termValues.push_back(operatingIncome);
        termValues.push_back(taxRate);
        termValues.push_back(afterTaxOperatingIncome);
        termValues.push_back(reinvestmentRate);
      }

      return reinvestmentRate;
    };   

    //==========================================================================
    static double calcFreeCashFlowToFirm(nlohmann::ordered_json &jsonData, 
                                     std::vector< std::string> &dateSet,
                                     std::vector< std::string> &previousDateSet,                                     
                                     const char *timeUnit,
                                     double taxRate,
                                     bool appendTermRecord,
                                     bool setNansToMissingValue,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      double totalCashFromOperatingActivities =  
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"totalCashFromOperatingActivities",
          setNansToMissingValue);

      std::string resultName("freeCashFlowToFirm_");

      std::string parentCategoryName("freeCashFlowToFirm_");

      double reinvestmentRate = calcReinvestmentRate(jsonData,
                                                    dateSet,
                                                    previousDateSet,
                                                    timeUnit,
                                                    taxRate,
                                                    appendTermRecord,
                                                    parentCategoryName,
                                                    setNansToMissingValue,
                                                    termNames,
                                                    termValues);
                                                    
      double freeCashFlowToFirm =  
        totalCashFromOperatingActivities*(1-taxRate)*(1-reinvestmentRate);

      if(     !JsonFunctions::isJsonFloatValid(totalCashFromOperatingActivities)
          ||  !JsonFunctions::isJsonFloatValid(taxRate)
          ||  !JsonFunctions::isJsonFloatValid(reinvestmentRate)){
        if(setNansToMissingValue){
          freeCashFlowToFirm=JsonFunctions::MISSING_VALUE;
        }else{
          freeCashFlowToFirm=std::nan("1");
        }
      }

      if(appendTermRecord){

        termNames.push_back(resultName + "totalCashFromOperatingActivities");
        termNames.push_back("freeCashFlowToFirm");

        termValues.push_back(totalCashFromOperatingActivities);
        termValues.push_back(freeCashFlowToFirm);

      }

      return freeCashFlowToFirm;
    };

    //==========================================================================
    /**
     * This function calculates the residual cash flow defined in the book
     * `The end of accounting and the path forward' by Baruch Lev and Feng Gu.
     * in Chapter 18:
     * 
     * Residual Cash Flow = cash flow from operations (1)
     *                    + research and development (2)
     *                    - normal capital expenditures (3, mean of 3-5 years)
     *                    - cost of equity capital (4)
     * 
     * Terms 1 and 2 directly come from reported financial data.
     *                    
    */
    static double calcResidualCashFlow(
        nlohmann::ordered_json &jsonData, 
        std::vector< std::string > &dateSet,
        const char *timeUnit,
        double costOfEquityAsAPercentage,
        std::vector< std::vector< std::string >> &datesToAverageCapitalExpenditures,
        bool appendTermRecord,
        bool setNansToMissingValue,
        std::vector< std::string> &termNames,
        std::vector< double > &termValues){

      double totalCashFromOperatingActivities = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,CF,timeUnit,dateSet,"totalCashFromOperatingActivities",
          setNansToMissingValue);

      //Not all firms actually have a research and development entry
      double researchDevelopment = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,IS,timeUnit,dateSet,"researchDevelopment",
          true);  
      
      //Extract the mean capital expenditure for the list of dates given
      double capitalExpenditureMean = 0;     
      double capitalExpenditure     = 0; 
      double capitalExpenditureCount = 0;

      for(size_t i =0; i < datesToAverageCapitalExpenditures.size(); ++i){
        capitalExpenditure = 
          FinancialAnalysisToolkit::sumFundamentalDataOverDates(
            jsonData,FIN,CF,timeUnit,datesToAverageCapitalExpenditures[i],
            "capitalExpenditures",setNansToMissingValue);

        if(!JsonFunctions::isJsonFloatValid(capitalExpenditure)){
          break;
        }        
        capitalExpenditureMean += capitalExpenditure;
        capitalExpenditureCount+=1.0;
      }

    
      if(capitalExpenditureCount>0){
        capitalExpenditureMean = capitalExpenditureMean
                                /capitalExpenditureCount;
      }
      //Evaluate the cost of equity
      double totalStockholderEquity = JsonFunctions::getJsonFloat( 
        jsonData[FIN][BAL][timeUnit][dateSet[0].c_str()]["totalStockholderEquity"],
        setNansToMissingValue);

      //double totalStockholderEquity = 
      //  FinancialAnalysisToolkit::sumFundamentalDataOverDates(
      //    jsonData,FIN,BAL,timeUnit,dateSet,"totalStockholderEquity",
      //    setNansToMissingValue); 

      double costOfEquity = JsonFunctions::MISSING_VALUE; 
      
      if(   JsonFunctions::isJsonFloatValid(totalStockholderEquity)
         && JsonFunctions::isJsonFloatValid(costOfEquityAsAPercentage) ){
          costOfEquity = totalStockholderEquity*costOfEquityAsAPercentage;
      }
  
      double residualCashFlow =  totalCashFromOperatingActivities
                            + researchDevelopment
                            - capitalExpenditureMean
                            - costOfEquity;
      
      //I'm willing to accept that researchDevelopment is not reported
      //because this often is not available in EODs reports
      if(   !JsonFunctions::isJsonFloatValid(totalCashFromOperatingActivities)
         || !JsonFunctions::isJsonFloatValid(capitalExpenditureMean)
         || !JsonFunctions::isJsonFloatValid(costOfEquity)){
        if(setNansToMissingValue){
          residualCashFlow = JsonFunctions::MISSING_VALUE;
        }else{
          residualCashFlow = std::nan("1");
        }
      }

      if(appendTermRecord){
        termNames.push_back("residualCashFlow_totalStockholderEquity");
        termNames.push_back("residualCashFlow_costOfEquityAsAPercentage");
        termNames.push_back("residualCashFlow_costOfEquity");
        termNames.push_back("residualCashFlow_totalCashFromOperatingActivities");
        termNames.push_back("residualCashFlow_researchDevelopment");
        termNames.push_back("residualCashFlow_capitalExpenditureMean");
        termNames.push_back("residualCashFlow");

        termValues.push_back(totalStockholderEquity);
        termValues.push_back(costOfEquityAsAPercentage);
        termValues.push_back(costOfEquity);
        termValues.push_back(totalCashFromOperatingActivities);
        termValues.push_back(researchDevelopment);
        termValues.push_back(capitalExpenditureMean);
        termValues.push_back(residualCashFlow);
      }

      return residualCashFlow;

    }

    //==========================================================================
    static double calcEnterpriseValue(nlohmann::ordered_json &fundamentalData, 
                                    double marketCapitalization, 
                                    std::vector< std::string > &dateSet,
                                    const char *timeUnit, 
                                    bool appendTermRecord,                                      
                                    std::string &parentCategoryName,
                                    bool setNansToMissingValue,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

      double shortLongTermDebtTotal = JsonFunctions::getJsonFloat(
        fundamentalData[FIN][BAL][timeUnit][dateSet[0].c_str()]
        ["shortLongTermDebtTotal"],setNansToMissingValue);

      //This is an alternative to shortLongTermDebtTotal if 
      //shortLongTermDebtTotal is missing
      double longTermDebt = JsonFunctions::getJsonFloat(
        fundamentalData[FIN][BAL][timeUnit][dateSet[0].c_str()]
        ["longTermDebt"],setNansToMissingValue);

      if(JsonFunctions::isJsonFloatValid(shortLongTermDebtTotal)){
        longTermDebt = 0.;        
      }else if(JsonFunctions::isJsonFloatValid(longTermDebt)){
        shortLongTermDebtTotal= JsonFunctions::MISSING_VALUE;
      }else{
        shortLongTermDebtTotal = JsonFunctions::MISSING_VALUE;
        longTermDebt = JsonFunctions::MISSING_VALUE;
      }

      //Not all firms have an entry for cash and equivalents
      double cashAndEquivalents = JsonFunctions::getJsonFloat(
        fundamentalData[FIN][BAL][timeUnit][dateSet[0].c_str()]["cashAndEquivalents"],
        true);

      double cash = JsonFunctions::getJsonFloat(
        fundamentalData[FIN][BAL][timeUnit][dateSet[0].c_str()]["cash"],
        setNansToMissingValue);
      

      if(JsonFunctions::isJsonFloatValid(cashAndEquivalents)){
        cash = 0.;        
      }

      //From Investopedia: https://www.investopedia.com/terms/e/enterprisevalue.asp
      // EV = MC + Total Debt - C
      // EV: enterprise value
      // MC: market capitalization
      // Total Debt: total debt
      // C: cash and equivalents
      //
      // C: I'm willing to put in cash here if cashAndEquivalents is not
      //    available. I'm also willing to ignore this field if neither cash
      //    and cash equivalents are not available.
      double enterpriseValue = marketCapitalization
                              + (shortLongTermDebtTotal+longTermDebt) 
                              - (cashAndEquivalents + cash);

      
      if(    !JsonFunctions::isJsonFloatValid(shortLongTermDebtTotal)
             && !JsonFunctions::isJsonFloatValid(longTermDebt) ){
        if(setNansToMissingValue){
          enterpriseValue = JsonFunctions::MISSING_VALUE;
        }else{
          enterpriseValue = std::nan("1");
        }
      }

      if(appendTermRecord){
        termNames.push_back(parentCategoryName+"enterpriseValue_shortLongTermDebtTotal");
        termNames.push_back(parentCategoryName+"enterpriseValue_longTermDebt");
        termNames.push_back(parentCategoryName+"enterpriseValue_cashAndEquivalents");
        termNames.push_back(parentCategoryName+"enterpriseValue_cash");
        termNames.push_back(parentCategoryName+"enterpriseValue_marketCapitalization");
        termNames.push_back(parentCategoryName+"enterpriseValue");

        termValues.push_back(shortLongTermDebtTotal);
        termValues.push_back(longTermDebt);
        termValues.push_back(cashAndEquivalents);
        termValues.push_back(cash);
        termValues.push_back(marketCapitalization);
        termValues.push_back(enterpriseValue);      
      }

      return enterpriseValue;


    }

    //==========================================================================
    static double calcPresentValueOfDiscountedFutureCashFlows(
                                nlohmann::ordered_json &jsonData, 
                                std::vector< std::string > &dateSet,
                                std::vector< std::string > &previousDateSet,
                                const char *timeUnit,   
                                double riskFreeRate,
                                double costOfCapital,
                                double costOfCapitalMature,
                                double taxRate,
                                int numberOfYearsForTerminalValuation,                              
                                bool appendTermRecord,
                                bool setNansToMissingValue,
                                std::vector< std::string> &termNames,
                                std::vector< double > &termValues){

      std::string parentName("presentValueDCF_");

      if(appendTermRecord){
        termNames.push_back("presentValueDCF_riskFreeRate");
        termNames.push_back("presentValueDCF_costOfCapital");
        termValues.push_back(riskFreeRate);
        termValues.push_back(costOfCapital);
      }

      double reinvestmentRate = calcReinvestmentRate(jsonData,
                                                    dateSet,
                                                    previousDateSet,
                                                    timeUnit,
                                                    taxRate,
                                                    appendTermRecord,
                                                    parentName,
                                                    setNansToMissingValue,
                                                    termNames,
                                                    termValues);

      double returnOnInvestedCapital = 
                    calcReturnOnInvestedCapital(  jsonData,
                                                  dateSet,
                                                  timeUnit,
                                                  appendTermRecord,
                                                  parentName,
                                                  setNansToMissingValue,
                                                  termNames,
                                                  termValues);

      double operatingIncomeGrowth = reinvestmentRate*returnOnInvestedCapital;

      double retentionRatio = calcRetentionRatio( jsonData,
                                                  dateSet,
                                                  timeUnit,
                                                  appendTermRecord,
                                                  parentName,
                                                  setNansToMissingValue,
                                                  termNames,
                                                  termValues);

      double returnOnEquity = calcReturnOnEquity(   jsonData,
                                                    dateSet,
                                                    timeUnit,
                                                    appendTermRecord,                                                    
                                                    parentName,
                                                    setNansToMissingValue,
                                                    termNames,
                                                    termValues);                                       

      double netIncomeGrowth = retentionRatio*returnOnEquity;

      if(appendTermRecord){

        termNames.push_back("presentValueDCF_taxRate");
        termNames.push_back("presentValueDCF_reinvestmentRate");
        termNames.push_back("presentValueDCF_returnOnInvestedCapital");
        termNames.push_back("presentValueDCF_operatingIncomeGrowth");
        termNames.push_back("presentValueDCF_retentionRatio");
        termNames.push_back("presentValueDCF_returnOnEquity");
        termNames.push_back("presentValueDCF_netIncomeGrowth");

        termValues.push_back(taxRate);
        termValues.push_back(reinvestmentRate);
        termValues.push_back(returnOnInvestedCapital);
        termValues.push_back(operatingIncomeGrowth);
        termValues.push_back(retentionRatio);
        termValues.push_back(returnOnEquity);
        termValues.push_back(netIncomeGrowth);
      }

      std::vector< double > afterTaxOperatingIncomeVector(
                              1+numberOfYearsForTerminalValuation);
      std::vector< double > reinvestmentVector(
                              1+numberOfYearsForTerminalValuation);
      std::vector< double > freeCashFlowToFirmVector(
                              1+numberOfYearsForTerminalValuation);

      //double totalCashFromOperatingActivities =
      //  JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]  
      //                ["totalCashFromOperatingActivities"]); 

      double operatingIncome = 
        FinancialAnalysisToolkit::sumFundamentalDataOverDates(
          jsonData,FIN,IS,timeUnit,dateSet,"operatingIncome",
          setNansToMissingValue);

      //double revenue = 
      //  FinancialAnalysisToolkit::sumFundamentalDataOverDates(
      //    jsonData,FIN,IS,timeUnit,dateSet,"totalRevenue",
      //    setNansToMissingValue);


      double afterTaxOperatingIncome = 
        operatingIncome*(1.0-taxRate);

      if(appendTermRecord){
        termNames.push_back("presentValueDCF_afterTaxOperatingIncome_operatingIncome");
        termNames.push_back("presentValueDCF_afterTaxOperatingIncome_taxRate");
        termNames.push_back("presentValueDCF_afterTaxOperatingIncome");

        termValues.push_back(operatingIncome);
        termValues.push_back(taxRate);
        termValues.push_back(afterTaxOperatingIncome);
      }


      for(int i=0; i<=numberOfYearsForTerminalValuation;++i){
        
        if(i==0){
          afterTaxOperatingIncomeVector[i]=afterTaxOperatingIncome;
          reinvestmentVector[i]=0.;
          freeCashFlowToFirmVector[i]=0.;
        }else{
          afterTaxOperatingIncomeVector[i] = 
            afterTaxOperatingIncomeVector[i-1]*(1.0+operatingIncomeGrowth);
          reinvestmentVector[i]=
            afterTaxOperatingIncomeVector[i]*reinvestmentRate;
          freeCashFlowToFirmVector[i]=afterTaxOperatingIncomeVector[i]
                                     -reinvestmentVector[i];
        }
        if(appendTermRecord){
          std::stringstream sstreamName;
          sstreamName.str(std::string());
          sstreamName << "presentValueDCF_afterTaxOperatingIncome_"<< i;
          termNames.push_back(sstreamName.str());

          sstreamName.str(std::string());
          sstreamName << "presentValueDCF_reinvestment_"<< i;
          termNames.push_back(sstreamName.str());          

          sstreamName.str(std::string());
          sstreamName << "presentValueDCF_freeCashFlowToFirm_"<< i;
          termNames.push_back(sstreamName.str());

          termValues.push_back(afterTaxOperatingIncomeVector[i]);
          termValues.push_back(reinvestmentVector[i]);
          termValues.push_back(freeCashFlowToFirmVector[i]);
        }
      }

      double reinvestmentRateStableGrowth = riskFreeRate/costOfCapitalMature;

      double terminalAfterTaxOperatingIncome = 
        afterTaxOperatingIncomeVector[numberOfYearsForTerminalValuation];

      double terminalValue = 
        (terminalAfterTaxOperatingIncome
          *(1+riskFreeRate)
          *(1-reinvestmentRateStableGrowth)
        )/(costOfCapitalMature-riskFreeRate);

      double presentValue = 0;

      for(int i=1; i<=numberOfYearsForTerminalValuation; ++i){
        presentValue += freeCashFlowToFirmVector[i]/std::pow(1.+costOfCapital,i);
      }
      presentValue += terminalValue 
        / std::pow(1.+costOfCapital,numberOfYearsForTerminalValuation);

      if(    !JsonFunctions::isJsonFloatValid( reinvestmentRate)
          || !JsonFunctions::isJsonFloatValid( returnOnInvestedCapital)
          || !JsonFunctions::isJsonFloatValid( retentionRatio)
          || !JsonFunctions::isJsonFloatValid( returnOnEquity)
          || !JsonFunctions::isJsonFloatValid( operatingIncome)
          || !JsonFunctions::isJsonFloatValid( taxRate)){

        if(setNansToMissingValue){
          presentValue = JsonFunctions::MISSING_VALUE;
        }else{
          presentValue = std::nan("1");
        }
      }


      if(appendTermRecord){
        termNames.push_back("presentValueDCF_terminalValue_afterTaxOperatingIncome");
        termNames.push_back("presentValueDCF_terminalValue_riskFreeRate");
        termNames.push_back("presentValueDCF_terminalValue_reinvestmentRateStableGrowth");
        termNames.push_back("presentValueDCF_terminalValue_costOfCapital");
        termNames.push_back("presentValueDCF_terminalValue_costOfCapitalMature");
        termNames.push_back("presentValueDCF_terminalValue");
        termNames.push_back("presentValueDCF");

        termValues.push_back(terminalAfterTaxOperatingIncome);
        termValues.push_back(riskFreeRate);
        termValues.push_back(reinvestmentRateStableGrowth);
        termValues.push_back(costOfCapital);
        termValues.push_back(costOfCapitalMature);
        termValues.push_back(terminalValue);
        termValues.push_back(presentValue);
      }

      return presentValue;

    };


};

#endif