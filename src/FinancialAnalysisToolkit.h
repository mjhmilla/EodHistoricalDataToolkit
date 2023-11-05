#ifndef FINANCIAL_ANALYSIS_TOOLKIT
#define FINANCIAL_ANALYSIS_TOOLKIT

#include <string>
#include <stdlib.h>

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
                                              const char *timeUnit){
      // Return On Capital Deployed
      //  Source: https://www.investopedia.com/terms/r/roce.asp
      double longTermDebt = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["longTermDebt"]);       

      double totalShareholderEquity = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["totalStockholderEquity"]);

      double  ebit = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["operatingIncome"]);

      //There are two definitions for capital depoloyed, and they should
      //be the same. These values are not the same for Apple, but are within
      //30% of one and other.  
      //1. capitalDeployed = (longTermDebt+totalShareholderEquity);
        //2. capitalDeployed = (totalAssets-currentTotalLiabilities);

        return ebit / (longTermDebt+totalShareholderEquity);
      };

    static double calcReturnOnInvestedCapital(nlohmann::ordered_json &jsonData, 
                                              std::string &date,
                                              const char *timeUnit){
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
      
      //Some companies don't pay dividends: if this field does not appear as
      //an entry in the security filings then value in the EOD json file will
      //be nan.
      if(std::isnan(dividendsPaid)){
        dividendsPaid=0.;
      }

      return (netIncome-dividendsPaid) / (longTermDebt+totalShareholderEquity);
    };

    /**
     *Return On Assets
     *Source: https://www.investopedia.com/terms/r/returnonassets.asp 
     * 
    */      
    static double calcReturnOnAssets(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){

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

      return netIncome / totalAssets;
    };

    /**
     * Gross margin
     * https://www.investopedia.com/terms/g/grossmargin.asp
    */
    static double calcGrossMargin(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){
      
      double totalRevenue =
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["totalRevenue"]);
      double costOfRevenue = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["costOfRevenue"]);

      return (totalRevenue-costOfRevenue)/totalRevenue;

    };

    /**
     * https://www.investopedia.com/terms/o/operatingmargin.asp
    */
    static double calcOperatingMargin(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){
      double  operatingIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["operatingIncome"]);      
      double totalRevenue =
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["totalRevenue"]);

      return operatingIncome/totalRevenue;
    };    

    /**
     * https://corporatefinanceinstitute.com/resources/accounting/cash-conversion-ratio/
    */
    static double calcCashConversionRatio(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){
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
      double  freeCashFlow = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["freeCashFlow"]);

      double totalCashFromOperatingActivities = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["totalCashFromOperatingActivities"]);

      double netIncome = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["netIncome"]);

      double capitalExpenditures = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["capitalExpenditures"]);

      double freeCashFlowCalc = 
        (totalCashFromOperatingActivities-capitalExpenditures);

      double freeCashFlowError = std::abs(freeCashFlow-freeCashFlowCalc)
                                /(0.5*std::abs(freeCashFlow+freeCashFlowCalc));

      if(freeCashFlowError > 0.1){
        std::cout << '\t'
                  << "Warning: Free-cash-flow calculated (" << freeCashFlowCalc 
                  << ") and free-cash-flow from EOD (" << freeCashFlow 
                  << ") differ by " << freeCashFlowError*100.0 << "%" 
                  << std::endl;
      }

      return (freeCashFlowCalc)/netIncome;

    };

    /**
       https://www.investopedia.com/terms/l/leverageratio.asp
    */
    static double calcDebtToCapitalizationRatio(
                                    nlohmann::ordered_json &jsonData, 
                                    std::string &date,
                                    const char *timeUnit){

      double shortTermDebt = 
              JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["shortTermDebt"]);
      double longTermDebt = 
              JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["longTermDebt"]);      
      double shareHoldersEquity = 
              JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["totalStockholderEquity"]);

      double debtToCapitalization=(shortTermDebt+longTermDebt)
                    /(shortTermDebt+longTermDebt+shareHoldersEquity);

      return debtToCapitalization;                    
    };

    /**
     https://www.investopedia.com/terms/i/interestcoverageratio.asp
    */
    static double calcInterestCover(nlohmann::ordered_json &jsonData, 
                                    std::string &date,
                                    const char *timeUnit){
      double  ebit = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["operatingIncome"]);
      double interestExpense = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["interestExpense"]);

      double interestCover=ebit/interestExpense;

      return interestCover;

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
           - change in non-cash capital     | - otherNonCashItems
           - (principal repaid - new debt)  | + ?

     The only difficulty I have is the last line. The suggestion from EOD
     was to use the netDebt field. According to investopedia

      netDebt = (shortTermDebt+longTermDebt)-cashAndCashEquivalents

      This is not at all the same as the change in debt. I could instead look
      at the change in longTermDebt between two years. This would exclude 
      shortTermDebt, but perhaps that's fine.

      https://www.investopedia.com/terms/n/netdebt.asp

     Damodaran, A.(2011). The Little Book of Valuation. Wiley.
    */

    
    static double calcFreeCashFlowToEquity(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){

      
      //Investopedia definition
      double totalCashFromOperatingActivities = 
      JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                    ["totalCashFromOperatingActivities"]);

      double capitalExpenditures = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["capitalExpenditures"]);

      double totalCashFromFinancingActivities = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["totalCashFromFinancingActivities"]);

      double salePurchaseOfStock = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["salePurchaseOfStock"]);


      double fcfeIP = totalCashFromOperatingActivities 
                    - capitalExpenditures
                    + (totalCashFromFinancingActivities-salePurchaseOfStock);

      std::cout << "This does not work: "
                << "(totalCashFromFinancingActivities-salePurchaseOfStock)"
                << "This will not approximate the change in debt from a company"
                << "that issues a dividend."
                << std::endl;

      //Damodaran definition      
      //double  netIncome = 
      //  JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
      //                ["netIncome"]);
      //double depreciation =                                            
      // JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
      //                ["depreciation"]);
      //double otherNonCashItems = 
      //  JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
      //                ["otherNonCashItems"]);
      //double fcfeD =  netIncome
      //              + depreciation
      //              - capitalExpenditures
      //              - otherNonCashItems
      //              + (totalCashFromFinancingActivities-salePurchaseOfStock);
      
      return fcfeIP;
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
                                     const char *timeUnit){

    
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
      return ownersEarnings;
    };

    static double calcTaxRate(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){
      double taxProvision = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]["taxProvision"]);

      double incomeBeforeTaxes = 
        JsonFunctions::getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]["incomeBeforeTax"]);

      double taxRate = taxProvision/incomeBeforeTaxes;

      return taxRate;
    };


    static double calcReinvestmentRate(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){

    
      //Damodaran definition (page 40/172 22%)     

      double totalCashFromOperatingActivities =
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]  
                      ["totalCashFromOperatingActivities"]); 

      double taxRate = calcTaxRate(jsonData,date,timeUnit);

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
      return reinvestmentRate;
    };   


    static double calcFreeCashFlowToFirm(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){

      double totalCashFromOperatingActivities =
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]  
                      ["totalCashFromOperatingActivities"]); 

      double taxRate = calcTaxRate(jsonData, date, timeUnit);

      double operatingIncomeAfterTax = 
              totalCashFromOperatingActivities*(1.0-taxRate);

      double capitalExpenditures = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]  
                      ["capitalExpenditures"]); 

      double reinvestmentRate = 
        calcReinvestmentRate(jsonData,date,timeUnit);      

      double otherNonCashItems = 
        JsonFunctions::getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]["otherNonCashItems"]);


      double fcff =   operatingIncomeAfterTax 
                    - capitalExpenditures
                    - otherNonCashItems;

      return fcff;
    };

};

#endif