#ifndef FINANCIAL_ANALYSIS_TOOLKIT
#define FINANCIAL_ANALYSIS_TOOLKIT

#include <cmath>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <sstream>

#include <nlohmann/json.hpp>
#include "JsonFunctions.h"


const char *GEN = "General";
const char *TECH= "Technicals";
const char *FIN = "Financials";
const char *BAL = "Balance_Sheet";
const char *CF  = "Cash_Flow";
const char *IS  = "Income_Statement";

const char *Y = "yearly";
const char *Q = "quarterly";




class FinancialAnalysisToolkit {

  public:


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
                                    std::string &date, 
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    std::string &parentCategoryName,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      // Return On Capital Deployed
      //  Source: https://www.investopedia.com/terms/r/roce.asp
      double longTermDebt = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["longTermDebt"]);       

      //Total shareholder equity is the money that would be left over if the 
      //company went out of business immediately. 
      //https://www.investopedia.com/ask/answers/033015/what-does-total-stockholders-equity-represent.asp
      double totalShareholderEquity = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["totalStockholderEquity"]);

      double  operatingIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["operatingIncome"]);

      //There are two definitions for capital depoloyed, and they should
      //be the same. These values are not the same for Apple, but are within
      //30% of one and other.  
      //1. capitalDeployed = (longTermDebt+totalShareholderEquity);
      //2. capitalDeployed = (totalAssets-currentTotalLiabilities);

      double returnOnCapitalDeployed = 
        operatingIncome / (longTermDebt+totalShareholderEquity);

      if(appendTermRecord){
        termNames.push_back(parentCategoryName+"returnOnCapitalDeployed_longTermDebt");
        termNames.push_back(parentCategoryName+"returnOnCapitalDeployed_totalShareholderEquity");
        termNames.push_back(parentCategoryName+"returnOnCapitalDeployed_operatingIncome");
        termNames.push_back(parentCategoryName+"returnOnCapitalDeployed");

        termValues.push_back(longTermDebt);
        termValues.push_back(totalShareholderEquity);
        termValues.push_back(operatingIncome);
        termValues.push_back(returnOnCapitalDeployed);
      }

      return returnOnCapitalDeployed;
    };

    //==========================================================================
    static double calcReturnOnInvestedCapital(nlohmann::ordered_json &jsonData, 
                                    std::string &date,
                                    const char *timeUnit,
                                    bool zeroNansInDividendsPaid,
                                    bool appendTermRecord,
                                    std::string &parentCategoryName,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      // Return On Invested Capital
      //  Source: 
      // https://www.investopedia.com/terms/r/returnoninvestmentcapital.asp
      double longTermDebt = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["longTermDebt"]);       

      double totalShareholderEquity = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["totalStockholderEquity"]);

      double  netIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["netIncome"]);

      //Interesting fact: dividends paid can be negative. This would have
      //the effect of increasing the ROIC for a misleading reason.
      double  dividendsPaid = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["dividendsPaid"]);
      
      if(std::isnan(dividendsPaid) && zeroNansInDividendsPaid){
        dividendsPaid=0.;
      }
      //Some companies don't pay dividends: if this field does not appear as
      //an entry in the security filings then value in the EOD json file will
      //be nan.
      //if(std::isnan(dividendsPaid)){
      //  dividendsPaid=0.;
      //}

      double returnOnInvestedCapital =  
        (netIncome-dividendsPaid) / (longTermDebt+totalShareholderEquity);

      if(appendTermRecord){
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital_longTermDebt");
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital_totalShareholderEquity");
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital_netIncome");
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital_dividendsPaid");
        termNames.push_back(parentCategoryName+"returnOnInvestedCapital");

        termValues.push_back(longTermDebt);
        termValues.push_back(totalShareholderEquity);
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
                                    std::string &date,
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    std::string &parentCategoryName,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      double totalShareholderEquity = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["totalStockholderEquity"]);

      double  netIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["netIncome"]);

      double returnOnEquity = netIncome/totalShareholderEquity;
      if(appendTermRecord){
        termNames.push_back(parentCategoryName+"returnOnEquity_netIncome");
        termNames.push_back(parentCategoryName+"returnOnEquity_totalShareholderEquity");
        termNames.push_back(parentCategoryName+"returnOnEquity");

        termValues.push_back(netIncome);
        termValues.push_back(totalShareholderEquity);
        termValues.push_back(returnOnEquity);

      }

      return returnOnEquity;
    };    

    //==========================================================================
    static double calcRetentionRatio(nlohmann::ordered_json &jsonData, 
                                    std::string &date,
                                    const char *timeUnit,
                                    bool zeroNansInDividendsPaid,
                                    bool appendTermRecord,
                                    std::string &parentCategoryName,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      // Return On Invested Capital
      //  Source: 
      // https://www.investopedia.com/terms/r/returnoninvestmentcapital.asp

      double  netIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["netIncome"]);

      //Interesting fact: dividends paid can be negative. This would have
      //the effect of increasing the ROIC for a misleading reason.
      double  dividendsPaid = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["dividendsPaid"]);
      
      if(std::isnan(dividendsPaid) && zeroNansInDividendsPaid){
        dividendsPaid=0.;
      }

      double retentionRatio =  
        (netIncome-dividendsPaid) / (netIncome);

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
                                     std::string &date,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      double  netIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["netIncome"]);

      double totalAssets = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["totalAssets"]);

      //There are two definitions for capital depoloyed, and they should
      //be the same. These values are not the same for Apple, but are within
      //30% of one and other.  
      //1. capitalDeployed = (longTermDebt+totalShareholderEquity);
      //2. capitalDeployed = (totalAssets-currentTotalLiabilities);

      double returnOnAssets= netIncome / totalAssets;

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
                                     std::string &date,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){
      
      double totalRevenue =
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["totalRevenue"]);
      double costOfRevenue = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["costOfRevenue"]);

      double grossMargin= (totalRevenue-costOfRevenue)/totalRevenue;

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
                                     std::string &date,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){
      double  operatingIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["operatingIncome"]);      
      double totalRevenue =
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["totalRevenue"]);

      double operatingMargin=operatingIncome/totalRevenue;

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
                                     std::string &date,
                                     const char *timeUnit,
                                     double taxRate,
                                     bool appendTermRecord,
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
                                                date,
                                                timeUnit, 
                                                taxRate, 
                                                appendTermRecord,
                                                categoryName, 
                                                termNames, 
                                                termValues);

      double netIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["netIncome"]);

      double cashFlowConversionRatio = (freeCashFlow)/netIncome;

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
                                    std::string &date,
                                    const char *timeUnit,
                                    bool zeroNansInShortTermDebt,
                                    bool appendTermRecord,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

      double shortTermDebt = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][date.c_str()]["shortTermDebt"]);

      if(std::isnan(shortTermDebt) && zeroNansInShortTermDebt){
        shortTermDebt=0.;
      }
      double longTermDebt =  JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][date.c_str()]["longTermDebt"]);  

      double totalStockholderEquity =  JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][date.c_str()]["totalStockholderEquity"]);

      double debtToCapitalizationRatio=(shortTermDebt+longTermDebt)
                    /(shortTermDebt+longTermDebt+totalStockholderEquity);

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
    static double calcInterestCover(nlohmann::ordered_json &jsonData, 
                                    std::string &date,
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

      double  operatingIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["operatingIncome"]);

      double interestExpense = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["interestExpense"]);

      double interestCover=operatingIncome/interestExpense;

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
                                    std::string &date,
                                    const char *timeUnit,
                                    double meanInterestCover,
                                    nlohmann::ordered_json &jsonDefaultSpread,
                                    bool appendTermRecord,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

        double interestCover = FinancialAnalysisToolkit::
          calcInterestCover(jsonData,
                            date,
                            timeUnit,
                            appendTermRecord,
                            termNames,
                            termValues);

        if(std::isnan(interestCover) || std::isinf(interestCover)){
          interestCover = meanInterestCover;
        }

        double defaultSpread = std::nan("1");
        bool found=false;
        unsigned int i=0;        
        int tableSize = jsonDefaultSpread["US"]["default_spread"].size();

        double interestCoverLowestValue = JsonFunctions::getJsonFloat(
              jsonDefaultSpread["US"]["default_spread"].at(0).at(0));
        double interestCoverHighestValue = JsonFunctions::getJsonFloat(
              jsonDefaultSpread["US"]["default_spread"].at(tableSize-1).at(1));

        if(interestCover < interestCoverLowestValue){
          defaultSpread = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(0).at(2));
        
        }else if(interestCover > interestCoverHighestValue){
          defaultSpread = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(tableSize-1).at(2));          
        
        }else{        
          while(found == false 
                && i < tableSize){
            
            double interestCoverLowerBound = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(i).at(0));
            double interestCoverUpperBound = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(i).at(1));
            double defaultSpreadIntervalValue = JsonFunctions::getJsonFloat(
                jsonDefaultSpread["US"]["default_spread"].at(i).at(2));


            if(interestCover >= interestCoverLowerBound
            && interestCover <= interestCoverUpperBound){
              defaultSpread = defaultSpreadIntervalValue;
              found=true;
            }
            ++i;
          } 
        }

        if(std::isnan(defaultSpread) || std::isinf(defaultSpread)){
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
                                    std::string &date,
                                    const char *timeUnit,
                                    double taxRate,
                                    bool appendTermRecord,
                                    std::string parentCategoryName,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

      
      //Investopedia definition
      //https://www.investopedia.com/terms/f/freecashflow.asp

      double totalCashFromOperatingActivities = 
      JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                    ["totalCashFromOperatingActivities"]);

      double interestExpense = JsonFunctions::getJsonFloat(
        jsonData[FIN][IS][timeUnit][date.c_str()]["interestExpense"]);

      std::string resultName(parentCategoryName);
      resultName.append("freeCashFlow_");                    

      /*
      double taxRate = calcTaxRate( jsonData,
                                    date,
                                    timeUnit,
                                    appendTermRecord,
                                    resultName,
                                    termNames,
                                    termValues);      

      if(std::isnan(taxRate)){
        taxRate=defaultTaxRate;
        termValues[termValues.size()-1]=defaultTaxRate;
      }        
      */                                          

      double taxShieldOnInterestExpense = interestExpense*taxRate;


      double capitalExpenditures = JsonFunctions::getJsonFloat(
        jsonData[FIN][CF][timeUnit][date.c_str()]["capitalExpenditures"]);

      double freeCashFlow = 
          totalCashFromOperatingActivities
          + interestExpense
          - taxShieldOnInterestExpense
          - capitalExpenditures;
      

      if(appendTermRecord){
        
        termNames.push_back(resultName + "totalCashFromOperatingActivities");
        termNames.push_back(resultName + "interestExpense");
        termNames.push_back(resultName + "taxShieldOnInterestExpense");
        termNames.push_back(resultName + "capitalExpenditures");
        termNames.push_back(parentCategoryName+"freeCashFlow");

        termValues.push_back(totalCashFromOperatingActivities);
        termValues.push_back(interestExpense);
        termValues.push_back(taxShieldOnInterestExpense);
        termValues.push_back(capitalExpenditures);
        termValues.push_back(freeCashFlow);
 
      }

      return freeCashFlow;
    };    

    //==========================================================================
    static double calcNetCapitalExpenditures(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     std::string &previousDate,
                                     const char *timeUnit,
                                     bool zeroNansInDepreciation,
                                     bool appendTermRecord,
                                     std::string &parentCategoryName,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){
      /*
      Problem: the depreciation field in EOD's json data is often null
      Solution: Since Damodran's formula for net capital expenditures is

      net capital expenditures =  capital expenditure - depreciation  [a]

      where capital expendature is the change in plant, property and equipment
      from the previous year.

      References
        https://www.investopedia.com/terms/c/capitalexpenditure.asp

        Damodaran, A.(2011). The Little Book of Valuation. Wiley.
      */

      double plantPropertyEquipment = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][date.c_str()]["propertyPlantEquipment"]);
      
      double plantPropertyEquipmentPrevious = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][previousDate.c_str()]["propertyPlantEquipment"]);

      double depreciation = JsonFunctions::getJsonFloat(
        jsonData[FIN][CF][timeUnit][date.c_str()]["depreciation"]);
      if(zeroNansInDepreciation && std::isnan(depreciation)){
        depreciation=0.;
      }

      double netCapitalExpenditures = 
        (plantPropertyEquipment-plantPropertyEquipmentPrevious)
        - depreciation;

      if(appendTermRecord){
        termNames.push_back(parentCategoryName 
          + "netCapitalExpenditures_plantPropertyEquipment");
        termNames.push_back(parentCategoryName 
          + "netCapitalExpenditures_plantPropertyEquipmentPrevious");
        termNames.push_back(parentCategoryName 
          + "netCapitalExpenditures_depreciation");
        termNames.push_back(parentCategoryName 
          + "netCapitalExpenditures");

        termValues.push_back(plantPropertyEquipment);
        termValues.push_back(plantPropertyEquipmentPrevious);
        termValues.push_back(depreciation);
        termValues.push_back(netCapitalExpenditures);
      }

      return netCapitalExpenditures;
    };

    //==========================================================================
    static double calcChangeInNonCashWorkingCapital(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     std::string &previousDate,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     std::string &parentCategoryName,
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

      double inventory = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][date.c_str()]["inventory"]);

      double inventoryPrevious = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][previousDate.c_str()]["inventory"]);

      double netReceivables = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][date.c_str()]["netReceivables"]);

     double netReceivablesPrevious = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][previousDate.c_str()]["netReceivables"]);

      double accountsPayable = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][date.c_str()]["accountsPayable"]);

      double accountsPayablePrevious = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][previousDate.c_str()]["accountsPayable"]);

      double changeInNonCashWorkingCapital = 
          ( (inventory-inventoryPrevious)
            +(netReceivables-netReceivablesPrevious)
            -(accountsPayable-accountsPayablePrevious));
            
      if(appendTermRecord){
        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_inventory");
        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_inventoryPrevious");

        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_netReceivables");
        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_netReceivablesPrevious");

        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_accountsPayable");
        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_accountsPayablePrevious");

        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital");

        termValues.push_back(inventory);
        termValues.push_back(inventoryPrevious);
        termValues.push_back(netReceivables);
        termValues.push_back(netReceivablesPrevious);        
        termValues.push_back(accountsPayable);
        termValues.push_back(accountsPayablePrevious);

        termValues.push_back(changeInNonCashWorkingCapital);   
      }  

      //An alternative version is to use the changeToInventory and 
      //changeToAccountReceivables fields. However, this method produces
      //values that differ substantially from the version above and also differ
      //from Damodaran's Ch. 3 example.

      /*
      double changeToInventory = JsonFunctions::getJsonFloat(
        jsonData[FIN][CF][timeUnit][date.c_str()]["changeToInventory"]);

      double changeToAccountReceivables = JsonFunctions::getJsonFloat(
        jsonData[FIN][CF][timeUnit][date.c_str()]["changeToAccountReceivables"]);

      double accountsPayable = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][date.c_str()]["accountsPayable"]);

      double accountsPayablePrevious = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][previousDate.c_str()]["accountsPayable"]);


      double changeInNonCashWorkingCapital = 
          ( (changeToInventory)
            +(changeToAccountReceivables)
            -(accountsPayable-accountsPayablePrevious));


      if(appendTermRecord){
        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_changeToInventory");
        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_changeToAccountReceivables");
        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_accountsPayable");
        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital_accountsPayablePrevious");
        termNames.push_back(parentCategoryName 
          + "changeInNonCashWorkingCapital");

        termValues.push_back(changeToInventory);
        termValues.push_back(changeToAccountReceivables);
        termValues.push_back(accountsPayable);
        termValues.push_back(accountsPayablePrevious);
        termValues.push_back(changeInNonCashWorkingCapital);

      }
      */
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
                                     std::string &date,
                                     std::string &previousDate,
                                     const char *timeUnit,
                                     bool zeroNansInDepreciation,
                                     bool replaceNanInShortLongDebtWithLongDebt,
                                     bool appendTermRecord,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      
      std::string parentName = "freeCashFlowToEquity_";

      double netIncome = 
      JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                    ["netIncome"]);

      double depreciation = 
      JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                    ["depreciation"]);

      double netCapitalExpenditures = 
        calcNetCapitalExpenditures( jsonData, 
                                    date,
                                    previousDate,
                                    timeUnit,
                                    zeroNansInDepreciation,
                                    appendTermRecord,
                                    parentName,
                                    termNames,
                                    termValues);

      double changeInNonCashWorkingCapital = 
        calcChangeInNonCashWorkingCapital( jsonData, 
                                    date,
                                    previousDate,
                                    timeUnit,
                                    appendTermRecord,
                                    parentName,
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
        jsonData[FIN][BAL][timeUnit][date.c_str()]["shortLongTermDebtTotal"]);
      if(std::isnan(shortLongTermDebtTotal) 
                 && replaceNanInShortLongDebtWithLongDebt){
        shortLongTermDebtTotal = JsonFunctions::getJsonFloat(
          jsonData[FIN][BAL][timeUnit][date.c_str()]["longTermDebt"]);
      }


      double shortLongTermDebtTotalPrevious = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][previousDate.c_str()]
                ["shortLongTermDebtTotal"]);
      if(std::isnan(shortLongTermDebtTotalPrevious) 
                 && replaceNanInShortLongDebtWithLongDebt){
        shortLongTermDebtTotalPrevious = JsonFunctions::getJsonFloat(
          jsonData[FIN][BAL][timeUnit][previousDate.c_str()]["longTermDebt"]);
      }

      //https://www.investopedia.com/terms/f/freecashflowtoequity.asp
      //Note that the sign for netDebtChanges must be such that more debt
      //produces a posive value for freeCashFlowToEquity

      double netDebtIssued = (shortLongTermDebtTotal
                              -shortLongTermDebtTotalPrevious);

      double freeCashFlowToEquity = 
          netIncome
          + (depreciation)
          - (netCapitalExpenditures)
          - (changeInNonCashWorkingCapital)
          + netDebtIssued;
      

      if(appendTermRecord){
        termNames.push_back("freeCashFlowToEquity_netIncome");
        termNames.push_back("freeCashFlowToEquity_shortLongTermDebtTotal");
        termNames.push_back("freeCashFlowToEquity_shortLongTermDebtTotalPrevious");
        termNames.push_back("freeCashFlowToEquity_netDebtIssued");
        termNames.push_back("freeCashFlowToEquity");

        termValues.push_back(netIncome);
        termValues.push_back(shortLongTermDebtTotal); 
        termValues.push_back(shortLongTermDebtTotalPrevious); 
        termValues.push_back(netDebtIssued);
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
                                     std::string &date,
                                     std::string &previousDate,
                                     const char *timeUnit,   
                                     bool zeroNansInDepreciation,                              
                                     bool appendTermRecord,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

    
      //Definition from Ch. 3 of Damodaran (page 40/172 22%)     
      //Damodaran (2011). The little book of valuation
      
      double  netIncome = JsonFunctions::getJsonFloat(
        jsonData[FIN][CF][timeUnit][date.c_str()]["netIncome"]);

      std::string parentName = "ownersEarnings_";

      double netCapitalExpenditures = 
        calcNetCapitalExpenditures( jsonData, 
                                    date,
                                    previousDate,
                                    timeUnit,
                                    zeroNansInDepreciation,
                                    appendTermRecord,
                                    parentName,
                                    termNames,
                                    termValues);

      double changeInNonCashWorkingCapital = 
        calcChangeInNonCashWorkingCapital( jsonData, 
                                    date,
                                    previousDate,
                                    timeUnit,
                                    appendTermRecord,
                                    parentName,
                                    termNames,
                                    termValues);  

      double ownersEarnings =  
          netIncome
          - netCapitalExpenditures
          - changeInNonCashWorkingCapital;      

      if(appendTermRecord){
        termNames.push_back("ownersEarnings_netIncome");
        termNames.push_back("ownersEarnings");

        termValues.push_back(netIncome);
        termValues.push_back(ownersEarnings);
      }

      return ownersEarnings;
    };


    //==========================================================================
    static double calcTaxRateFromTheTaxProvision(
                                     nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     std::string &parentCategoryName,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      double taxProvision = JsonFunctions::getJsonFloat(
        jsonData[FIN][IS][timeUnit][date.c_str()]["taxProvision"]);

      double incomeBeforeTaxes =  JsonFunctions::getJsonFloat(
        jsonData[FIN][IS][timeUnit][date.c_str()]["incomeBeforeTax"]);

      double taxRate = taxProvision/incomeBeforeTaxes;

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


    //==========================================================================
    static double calcReinvestmentRate(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     std::string &previousDate,  
                                     const char *timeUnit,
                                     double taxRate,
                                     bool zeroNansInDepreciation,
                                     bool appendTermRecord,
                                     std::string &parentCategoryName,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

    
      //Damodaran definition (page 40/172 22%)     

      double totalCashFromOperatingActivities =
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]  
                      ["totalCashFromOperatingActivities"]); 

      std::string parentName = parentCategoryName;
      parentName.append("reinvestmentRate_");

      double afterTaxOperatingIncome = 
        totalCashFromOperatingActivities*(1.0-taxRate);

      double netCapitalExpenditures = 
        calcNetCapitalExpenditures( jsonData, 
                                    date,
                                    previousDate,
                                    timeUnit,
                                    zeroNansInDepreciation,
                                    appendTermRecord,
                                    parentName,
                                    termNames,
                                    termValues);

      double changeInNonCashWorkingCapital = 
        calcChangeInNonCashWorkingCapital( jsonData, 
                                    date,
                                    previousDate,
                                    timeUnit,
                                    appendTermRecord,
                                    parentName,
                                    termNames,
                                    termValues);          

      double reinvestmentRate =  
        (netCapitalExpenditures+changeInNonCashWorkingCapital
        )/afterTaxOperatingIncome;

      if(appendTermRecord){

        termNames.push_back(parentCategoryName 
          + "reinvestmentRate_totalCashFromOperatingActivities");
        termNames.push_back(parentCategoryName 
          + "reinvestmentRate_taxRate");
        termNames.push_back(parentCategoryName 
          + "reinvestmentRate_afterTaxOperatingIncome");
        termNames.push_back(parentCategoryName 
          + "reinvestmentRate");

        termValues.push_back(totalCashFromOperatingActivities);
        termValues.push_back(taxRate);
        termValues.push_back(afterTaxOperatingIncome);
        termValues.push_back(reinvestmentRate);
      }

      return reinvestmentRate;
    };   

    //==========================================================================
    static double calcFreeCashFlowToFirm(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     std::string &previousDate,                                     
                                     const char *timeUnit,
                                     double taxRate,
                                     bool zeroNansInDepreciation,
                                     bool appendTermRecord,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){


      double totalCashFromOperatingActivities = JsonFunctions::getJsonFloat(
        jsonData[FIN][CF][timeUnit][date.c_str()]["totalCashFromOperatingActivities"]); 


      std::string resultName("freeCashFlowToFirm_");

      /*
      double taxRate = calcTaxRate(jsonData, 
                                    date, 
                                    timeUnit, 
                                    appendTermRecord,
                                    resultName,
                                    termNames,
                                    termValues);

      if(std::isnan(taxRate)){
        taxRate=defaultTaxRate;
        termValues[termValues.size()-1]=defaultTaxRate;
      }
      */

      std::string parentCategoryName("freeCashFlowToFirm_");

      double reinvestmentRate = calcReinvestmentRate(jsonData,
                                                    date,
                                                    previousDate,
                                                    timeUnit,
                                                    taxRate,
                                                    zeroNansInDepreciation,
                                                    appendTermRecord,
                                                    parentCategoryName,
                                                    termNames,
                                                    termValues);
                                                    
      double freeCashFlowToFirm =  
        totalCashFromOperatingActivities*(1-taxRate)*(1-reinvestmentRate);

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
        std::string &date,
        const char *timeUnit,
        double costOfEquityAsAPercentage,
        std::vector< std::string > datesToAverageCapitalExpenditures,
        bool zeroNansInResearchAndDevelopment,
        bool appendTermRecord,
        std::vector< std::string> &termNames,
        std::vector< double > &termValues){

      bool isDataNan = false;

      double totalCashFromOperatingActivities = JsonFunctions::getJsonFloat( 
        jsonData[FIN][CF][timeUnit][date.c_str()]
                ["totalCashFromOperatingActivities"]);  

      if(std::isnan(totalCashFromOperatingActivities)){
        isDataNan=true;
      }

      double researchDevelopment = JsonFunctions::getJsonFloat(
        jsonData[FIN][IS][timeUnit][date.c_str()]["researchDevelopment"]);  

      if(std::isnan(researchDevelopment)){
        //2023/12/25
        //Some firms don't report research and development such as 
        //Master Card. It's possible they have outsourced all of this work
        //and don't have any actual R&D that shows up on their financial
        //statements. 
        if(zeroNansInResearchAndDevelopment)
          researchDevelopment=0.;
        else{
          isDataNan=true;
        }
      }
      
      //Extract the mean capital expenditure for the list of dates given
      double capitalExpenditureMean = 0;      

      for(auto& iter : datesToAverageCapitalExpenditures){        
        capitalExpenditureMean += JsonFunctions::getJsonFloat( 
          jsonData[FIN][CF][timeUnit][iter]["capitalExpenditures"]); 

        if(std::isnan(capitalExpenditureMean)){
          isDataNan=true;
        }        
      }

      if(!isDataNan){
        capitalExpenditureMean = 
          capitalExpenditureMean/
          static_cast<double>(datesToAverageCapitalExpenditures.size());
      }

      //Evaluate the cost of equity
      double totalStockholderEquity = JsonFunctions::getJsonFloat( 
        jsonData[FIN][BAL][timeUnit][date.c_str()]["totalStockholderEquity"]);

      if(std::isnan(totalStockholderEquity)){
        isDataNan=true;
      }

      double residualCashFlow = std::nan("1");
      double costOfEquity = totalStockholderEquity*costOfEquityAsAPercentage;

      if(!isDataNan){      
        residualCashFlow =  totalCashFromOperatingActivities
                            + researchDevelopment
                            - capitalExpenditureMean
                            - costOfEquity;
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
                                    double sharePriceAdjustedClose, 
                                    std::string &date,
                                    const char *timeUnit, 
                                    bool replaceNanInShortLongDebtWithLongDebt,
                                    bool appendTermRecord,                                      
                                    std::string &parentCategoryName,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

      double shortLongTermDebtTotal = JsonFunctions::getJsonFloat(
        fundamentalData[FIN][BAL][timeUnit][date.c_str()]["shortLongTermDebtTotal"]);

      if(std::isnan(shortLongTermDebtTotal) 
                 && replaceNanInShortLongDebtWithLongDebt){
        shortLongTermDebtTotal = JsonFunctions::getJsonFloat(
        fundamentalData[FIN][BAL][timeUnit][date.c_str()]["longTermDebt"]);
      }

      double cashAndEquivalents = JsonFunctions::getJsonFloat(
        fundamentalData[FIN][BAL][timeUnit][date.c_str()]["cashAndEquivalents"]);
      
      double commonStockSharesOutstanding = JsonFunctions::getJsonFloat(
        fundamentalData[FIN][BAL][timeUnit][date.c_str()]
                          ["commonStockSharesOutstanding"]);      

      double marketCapitalization = sharePriceAdjustedClose*commonStockSharesOutstanding;

      double enterpriseValue = marketCapitalization
                              + shortLongTermDebtTotal 
                              - commonStockSharesOutstanding;

      if(appendTermRecord){
        termNames.push_back(parentCategoryName+"enterpriseValue_shortLongTermDebtTotal");
        termNames.push_back(parentCategoryName+"enterpriseValue_cashAndEquivalents");
        termNames.push_back(parentCategoryName+"enterpriseValue_commonStockSharesOutstanding");
        termNames.push_back(parentCategoryName+"enterpriseValue_adjustedClose");
        termNames.push_back(parentCategoryName+"enterpriseValue_marketCapitalization");
        termNames.push_back(parentCategoryName+"enterpriseValue");

        termValues.push_back(shortLongTermDebtTotal);
        termValues.push_back(cashAndEquivalents);
        termValues.push_back(commonStockSharesOutstanding);
        termValues.push_back(sharePriceAdjustedClose);
        termValues.push_back(marketCapitalization);
        termValues.push_back(enterpriseValue);      
      }

      return enterpriseValue;


    }

    //==========================================================================
    static double calcPresentValueOfDiscountedFutureCashFlows(
                                nlohmann::ordered_json &jsonData, 
                                std::string &date,
                                std::string &previousDate,
                                const char *timeUnit,   
                                bool zeroNansInDividendsPaid,
                                bool zeroNansInDepreciation,
                                double riskFreeRate,
                                double costOfCapital,
                                double costOfCapitalMature,
                                double taxRate,
                                int numberOfYearsForTerminalValuation,                              
                                bool appendTermRecord,
                                std::vector< std::string> &termNames,
                                std::vector< double > &termValues){

      std::string parentName("presentValueDCF_");

      if(appendTermRecord){
        termNames.push_back("presentValueDCF_riskFreeRate");
        termNames.push_back("presentValueDCF_costOfCapital");
        termValues.push_back(riskFreeRate);
        termValues.push_back(costOfCapital);
      }

      /*
      double taxRate = calcTaxRate(jsonData, 
                                    date, 
                                    timeUnit, 
                                    appendTermRecord,
                                    parentName,
                                    termNames,
                                    termValues);
      if(std::isnan(taxRate)){
        taxRate=defaultTaxRate;
      }
      */

      double reinvestmentRate = calcReinvestmentRate(jsonData,
                                                    date,
                                                    previousDate,
                                                    timeUnit,
                                                    taxRate,
                                                    zeroNansInDepreciation,
                                                    appendTermRecord,
                                                    parentName,
                                                    termNames,
                                                    termValues);

      double returnOnInvestedCapital = 
                    calcReturnOnInvestedCapital(  jsonData,
                                                  date,
                                                  timeUnit,
                                                  zeroNansInDividendsPaid,
                                                  appendTermRecord,
                                                  parentName,
                                                  termNames,
                                                  termValues);

      double operatingIncomeGrowth = reinvestmentRate*returnOnInvestedCapital;

      double retentionRatio = calcRetentionRatio( jsonData,
                                                  date,
                                                  timeUnit,
                                                  zeroNansInDividendsPaid,
                                                  appendTermRecord,
                                                  parentName,
                                                  termNames,
                                                  termValues);

      double returnOnEquity = calcReturnOnEquity(   jsonData,
                                                    date,
                                                    timeUnit,
                                                    appendTermRecord,                                                    
                                                    parentName,
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

      double totalCashFromOperatingActivities =
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]  
                      ["totalCashFromOperatingActivities"]); 

      double afterTaxOperatingIncome = 
        totalCashFromOperatingActivities*(1.0-taxRate);

      if(appendTermRecord){
        termNames.push_back("presentValueDCF_afterTaxOperatingIncome_totalCashFromOperatingActivities");
        termNames.push_back("presentValueDCF_afterTaxOperatingIncome_taxRate");
        termNames.push_back("presentValueDCF_afterTaxOperatingIncome");

        termValues.push_back(totalCashFromOperatingActivities);
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