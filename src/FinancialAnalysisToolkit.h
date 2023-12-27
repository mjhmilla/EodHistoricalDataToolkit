#ifndef FINANCIAL_ANALYSIS_TOOLKIT
#define FINANCIAL_ANALYSIS_TOOLKIT

#include <cmath>
#include <string>
#include <stdlib.h>
#include <algorithm>

#include <nlohmann/json.hpp>
#include "JsonFunctions.h"


const char *GEN = "General";
const char *FIN = "Financials";
const char *BAL = "Balance_Sheet";
const char *CF  = "Cash_Flow";
const char *IS  = "Income_Statement";

const char *Y = "yearly";
const char *Q = "quarterly";


class FinancialAnalysisToolkit {

  public:

    static void getIsinCountryCodes(const std::string& isin, 
                            nlohmann::ordered_json &exchangeList,
                            std::string &isinCountryISO2,
                            std::string &isinCountryISO3){

      isinCountryISO2="";
      isinCountryISO3="";
      bool found=false;
      auto iterExchange=exchangeList.begin();
      while(iterExchange != exchangeList.end() && !found){
        std::string countryISO2 =(*iterExchange)["CountryISO2"].get<std::string>();
        std::string countryISO3 =(*iterExchange)["CountryISO3"].get<std::string>();
        if(  isin.find(countryISO2)==0 
          || isin.find(countryISO3)==0){
          isinCountryISO2=countryISO2;
          isinCountryISO3=countryISO3;
          found=true;
        }
        ++iterExchange;
      }
    };

    static bool doesIsinHaveMatchingExchange(const std::string &isin,
                                      nlohmann::ordered_json &exchangeList,
                                      const std::string &exchangeSymbolListFolder){

      auto iterExchange=exchangeList.begin();
      bool exists=false;
      while(iterExchange != exchangeList.end() && !exists){

        std::string countryISO2 =(*iterExchange)["CountryISO2"].get<std::string>();
        std::string countryISO3 =(*iterExchange)["CountryISO3"].get<std::string>();

        if(countryISO2.length()>0 && countryISO3.length()>0){
          if(  isin.find(countryISO2)==0 || isin.find(countryISO3)==0){
            //Check to see if the corresponding file exists
            std::string exchangeCode = (*iterExchange)["Code"].get<std::string>();  
            std::string exchangeSymbolListPath = exchangeSymbolListFolder;
            exchangeSymbolListPath.append(exchangeCode);
            exchangeSymbolListPath.append(".json");
            bool fileExists = std::filesystem::exists(exchangeSymbolListPath.c_str());
            if(fileExists){
              exists=true;
            }                   
          }
        }
        ++iterExchange;
      }  
      return exists;
    };

    static bool isExchangeAndIsinConsistent( std::string &exchange,
                                      std::string &isin,
                                      nlohmann::ordered_json &exchangeList){

      auto iterExchange=exchangeList.begin();
      bool consistent=false;
      while(iterExchange != exchangeList.end() && !consistent){

        std::string countryISO2 =(*iterExchange)["CountryISO2"].get<std::string>();
        std::string countryISO3 =(*iterExchange)["CountryISO3"].get<std::string>();

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

    static void createEodJsonFileName(const std::string &ticker, 
                                      const std::string &exchangeCode,
                                      std::string &updEodFileName)
    {
      updEodFileName=ticker;
      updEodFileName.append(".");
      updEodFileName.append(exchangeCode);
      updEodFileName.append(".json");
    };



    static double calcReturnOnCapitalDeployed( nlohmann::ordered_json &jsonData, 
                                    std::string &date, 
                                    const char *timeUnit,
                                    bool appendTermRecord,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      // Return On Capital Deployed
      //  Source: https://www.investopedia.com/terms/r/roce.asp
      double longTermDebt = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["longTermDebt"]);       

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
        termNames.push_back("returnOnCapitalDeployed_longTermDebt");
        termNames.push_back("returnOnCapitalDeployed_totalShareholderEquity");
        termNames.push_back("returnOnCapitalDeployed_operatingIncome");
        termNames.push_back("returnOnCapitalDeployed");

        termValues.push_back(longTermDebt);
        termValues.push_back(totalShareholderEquity);
        termValues.push_back(operatingIncome);
        termValues.push_back(returnOnCapitalDeployed);
      }

      return returnOnCapitalDeployed;
    };

    static double calcReturnOnInvestedCapital(nlohmann::ordered_json &jsonData, 
                                    std::string &date,
                                    const char *timeUnit,
                                    bool zeroNansInDividendsPaid,
                                    bool appendTermRecord,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){
      // Return On Invested Capital
      //  Source: https://www.investopedia.com/terms/r/returnoninvestmentcapital.asp
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
        termNames.push_back("returnOnInvestedCapital_longTermDebt");
        termNames.push_back("returnOnInvestedCapital_totalShareholderEquity");
        termNames.push_back("returnOnInvestedCapital_netIncome");
        termNames.push_back("returnOnInvestedCapital_dividendsPaid");
        termNames.push_back("returnOnInvestedCapital");

        termValues.push_back(longTermDebt);
        termValues.push_back(totalShareholderEquity);
        termValues.push_back(netIncome);
        termValues.push_back(dividendsPaid);
        termValues.push_back(returnOnInvestedCapital);

      }

      return returnOnInvestedCapital;
    };

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

    /**
     * https://corporatefinanceinstitute.com/resources/accounting/cash-conversion-ratio/
    */
    static double calcCashConversionRatio(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit,
                                     double defaultTaxRate,
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
      double freeCashFlow = calcFreeCashFlow(jsonData,date,timeUnit, 
        defaultTaxRate, appendTermRecord,categoryName, termNames, termValues);

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

    /**
       https://www.investopedia.com/terms/l/leverageratio.asp
    */
    static double calcDebtToCapitalizationRatio(
                                    nlohmann::ordered_json &jsonData, 
                                    std::string &date,
                                    const char *timeUnit,
                                    bool zeroNanInShortTermDebt,
                                    bool appendTermRecord,
                                    std::vector< std::string> &termNames,
                                    std::vector< double > &termValues){

      double shortTermDebt = 
              JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["shortTermDebt"]);
      if(std::isnan(shortTermDebt) && zeroNanInShortTermDebt){
        shortTermDebt=0.;
      }
      double longTermDebt = 
              JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["longTermDebt"]);      
      double totalStockholderEquity = 
              JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["totalStockholderEquity"]);

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


    static double calcFreeCashFlow(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit,
                                     double defaultTaxRate,
                                     bool appendTermRecord,
                                     std::string parentCategoryName,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      
      //Investopedia definition
      //https://www.investopedia.com/terms/f/freecashflow.asp

      double totalCashFromOperatingActivities = 
      JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                    ["totalCashFromOperatingActivities"]);

      double interestExpense = 
      JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                    ["interestExpense"]);

      std::string resultName(parentCategoryName);
      resultName.append("freeCashFlow_");                    

      double taxRate = calcTaxRate(jsonData,date,timeUnit,appendTermRecord,
                                    resultName,
                                    termNames,termValues);      

      if(std::isnan(taxRate)){
        taxRate=defaultTaxRate;
        termValues[termValues.size()-1]=defaultTaxRate;
      }                                                  

      double taxShieldOnInterestExpense = interestExpense*taxRate;


      double capitalExpenditures = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["capitalExpenditures"]);

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
                                     bool appendTermRecord,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

      
      //Investopedia definition

      double netIncome = 
      JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                    ["netIncome"]);

      double depreciation = 
      JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                    ["depreciation"]);

      double capitalExpenditures = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["capitalExpenditures"]);

      double otherNonCashItems = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["otherNonCashItems"]);

      double otherNonCashItemsPrevious = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][previousDate.c_str()]
                      ["otherNonCashItems"]);             

      double netDebt = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][date.c_str()]["netDebt"]);

      double netDebtPrevious = JsonFunctions::getJsonFloat(
        jsonData[FIN][BAL][timeUnit][previousDate.c_str()]["netDebt"]);

      //https://www.investopedia.com/terms/f/freecashflowtoequity.asp
      //Note that the sign for netDebtChanges must be such that more debt
      //produces a posive value for freeCashFlowToEquity
      double freeCashFlowToEquity = 
          netIncome
          + depreciation
          - capitalExpenditures
          - (otherNonCashItems-otherNonCashItemsPrevious)
          + (netDebt-netDebtPrevious);
      
      //          + (totalCashFromFinancingActivities-salePurchaseOfStock) 

      if(appendTermRecord){
        termNames.push_back("freeCashFlowToEquity_netIncome");
        termNames.push_back("freeCashFlowToEquity_depreciation");
        termNames.push_back("freeCashFlowToEquity_capitalExpenditures");
        termNames.push_back("freeCashFlowToEquity_otherNonCashItems");
        termNames.push_back("freeCashFlowToEquity_otherNonCashItemsPrevious");
        termNames.push_back("freeCashFlowToEquity_netDebt");
        termNames.push_back("freeCashFlowToEquity_netDebtPrevious");
        termNames.push_back("freeCashFlowToEquity");

        termValues.push_back(netIncome);
        termValues.push_back(depreciation);
        termValues.push_back(capitalExpenditures);
        termValues.push_back(otherNonCashItems);
        termValues.push_back(otherNonCashItemsPrevious);
        termValues.push_back(netDebt); 
        termValues.push_back(netDebtPrevious); 
        termValues.push_back(freeCashFlowToEquity);
      }

      return freeCashFlowToEquity;
    };

    /**
     Warren Buffets definition for owners earnings is a bit more conservative
     than free-cash-flow-to-equity and has the additional benefit of not using
     any fields in EODs data that are null. 

     Free cash flow to equity is a quantity that estimates how much money
     could have been returned to investors from a company. This quantity is
     an input needed to value a company following the methodology described
     in Ch. 3 of Damodaran.

    ----------
     From the book                           
     FCFE =  net Income                     | netIncome 
           + depreciation                   | + depreciation
           - cap. expenditures              | - capitalExpenditures
           - change in non-cash capital     | - otherNonCashItems
           - (principal repaid - new debt)  | + ?

    ----------
  
    Unfortunately I cannot find the fields within EOD that I could use
    to evaluate the change in debt. Perhaps if I took the change in short
    and long term debt from year to year. An alternative is to use the more
    conservative owners earnings:

    ----------
     From the book                           
     FCFE =  net Income                     | netIncome 
           + depreciation                   | + depreciation
           - cap. expenditures              | - capitalExpenditures
           - change in non-cash capital     | - otherNonCashItems
    ----------

    which makes use of fields that are populated, and I have hand checked
    for one company, Alphabet.

     * */
    static double calcOwnersEarnings(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit,
                                     bool appendTermRecord,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

    
      //Damodaran definition (page 40/172 22%)     
      double  netIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["netIncome"]);
      double depreciation =                                            
       JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["depreciation"]);
      double capitalExpenditures = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]  
                      ["capitalExpenditures"]);             
      double otherNonCashItems = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["otherNonCashItems"]);

      double ownersEarnings =  netIncome
                              + depreciation
                              - capitalExpenditures
                              - otherNonCashItems;  

      if(appendTermRecord){
        termNames.push_back("ownersEarnings_netIncome");
        termNames.push_back("ownersEarnings_depreciation");
        termNames.push_back("ownersEarnings_capitalExpenditures");
        termNames.push_back("ownersEarnings_otherNonCashItems");
        termNames.push_back("ownersEarnings");

        termValues.push_back(netIncome);
        termValues.push_back(depreciation);
        termValues.push_back(capitalExpenditures);
        termValues.push_back(otherNonCashItems);
        termValues.push_back(ownersEarnings);
      }                            
      return ownersEarnings;
    };

    static double calcTaxRate(nlohmann::ordered_json &jsonData, 
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


    static double calcReinvestmentRate(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit,
                                     double defaultTaxRate,
                                     bool appendTermRecord,
                                     std::string &parentCategoryName,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){

    
      //Damodaran definition (page 40/172 22%)     

      double totalCashFromOperatingActivities =
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]  
                      ["totalCashFromOperatingActivities"]); 

      std::string termCategoryName = parentCategoryName;
      termCategoryName.append("_reinvestmentRate_");

      double taxRate = calcTaxRate(jsonData,date,timeUnit, appendTermRecord,
                                    termCategoryName,termNames,termValues);

      if(std::isnan(taxRate)){
        taxRate=defaultTaxRate;
        termValues[termValues.size()-1]=defaultTaxRate;
      }

      double afterTaxOperatingIncome = 
        totalCashFromOperatingActivities*(1.0-taxRate);

      double capitalExpenditures = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]  
                      ["capitalExpenditures"]);             
      double otherNonCashItems = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["otherNonCashItems"]);

      double reinvestmentRate =  
        (capitalExpenditures + otherNonCashItems)/afterTaxOperatingIncome;

      //Update the argument and result record
      if(appendTermRecord){

        termNames.push_back(parentCategoryName + "reinvestmentRate_totalCashFromOperatingActivities");
        termNames.push_back(parentCategoryName + "reinvestmentRate_afterTaxOperatingIncome");
        termNames.push_back(parentCategoryName + "reinvestmentRate_capitalExpenditure");
        termNames.push_back(parentCategoryName + "reinvestmentRate_otherNonCashItems");
        termNames.push_back(parentCategoryName + "reinvestmentRate");

        termValues.push_back(totalCashFromOperatingActivities);
        termValues.push_back(afterTaxOperatingIncome);
        termValues.push_back(capitalExpenditures);
        termValues.push_back(otherNonCashItems);
        termValues.push_back(reinvestmentRate);
      }

      return reinvestmentRate;
    };   


    static double calcFreeCashFlowToFirm(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit,
                                     double defaultTaxRate,
                                     bool appendTermRecord,
                                     std::vector< std::string> &termNames,
                                     std::vector< double > &termValues){


      double totalCashFromOperatingActivities = JsonFunctions::getJsonFloat(
        jsonData[FIN][CF][timeUnit][date.c_str()]["totalCashFromOperatingActivities"]); 


      std::string resultName("freeCashFlowToFirm_");

      double taxRate = calcTaxRate(jsonData, date, timeUnit, appendTermRecord,
                                    resultName,termNames,termValues);

      if(std::isnan(taxRate)){
        taxRate=defaultTaxRate;
        termValues[termValues.size()-1]=defaultTaxRate;
      }

      double operatingIncomeAfterTax = 
              totalCashFromOperatingActivities*(1.0-taxRate);

      double capitalExpenditures =  JsonFunctions::getJsonFloat(
        jsonData[FIN][CF][timeUnit][date.c_str()]["capitalExpenditures"]); 

      //double reinvestmentRate = 
      //  calcReinvestmentRate(jsonData,date,timeUnit);      

      double otherNonCashItems = JsonFunctions::getJsonFloat(
          jsonData[FIN][CF][timeUnit][date.c_str()]["otherNonCashItems"]);
                                                     

      double freeCashFlowToFirm =   operatingIncomeAfterTax 
                                  - capitalExpenditures
                                  - otherNonCashItems;
      if(appendTermRecord){

        termNames.push_back(resultName + "totalCashFromOperatingActivities");
        termNames.push_back(resultName + "operatingIncomeAfterTax");
        termNames.push_back(resultName + "capitalExpenditures");
        termNames.push_back(resultName + "otherNonCashItems");
        termNames.push_back("freeCashFlowToFirm");

        termValues.push_back(totalCashFromOperatingActivities);
        termValues.push_back(operatingIncomeAfterTax);
        termValues.push_back(capitalExpenditures);
        termValues.push_back(otherNonCashItems);
        termValues.push_back(freeCashFlowToFirm);
      }

      return freeCashFlowToFirm;
    };

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
        jsonData[FIN][CF][timeUnit][date.c_str()]["totalCashFromOperatingActivities"]);  
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

};

#endif