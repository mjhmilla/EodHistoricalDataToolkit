
#include <string>
#include <nlohmann/json.hpp>
#include <stdlib.h>

const char *FIN = "Financials";
const char *BAL = "Balance_Sheet";
const char *CF  = "Cash_Flow";
const char *IS  = "Income_Statement";

const char *Y = "yearly";
const char *Q = "quarterly";

class FinancialAnalysisToolkit {

  public:

    static double getJsonFloat(nlohmann::ordered_json &jsonEntry){
      if(  jsonEntry.is_null()){
        return std::nan("1");
      }else{
        if(  jsonEntry.is_number_float()){
          return jsonEntry.get<double>();
        }else if (jsonEntry.is_string()){
          return std::atof(jsonEntry.get<std::string>().c_str());
        }else{
          throw std::invalid_argument("json entry is not a float or string");      
        }
      }
    };

    static double calcReturnOnCapitalDeployed( nlohmann::ordered_json &jsonData, 
                                              std::string &date, 
                                              const char *timeUnit){
      // Return On Capital Deployed
      //  Source: https://www.investopedia.com/terms/r/roce.asp
      double longTermDebt = 
        getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["longTermDebt"]);       

      double totalShareholderEquity = 
        getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["totalStockholderEquity"]);

      double  ebit = 
        getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
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
        getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["longTermDebt"]);       

      double totalShareholderEquity = 
        getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
                      ["totalStockholderEquity"]);

      double  netIncome = 
        getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["netIncome"]);

      //Interesting fact: dividends paid can be negative. This would have
      //the effect of increasing the ROIC for a misleading reason.
      double  dividendsPaid = 
        getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
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
        getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["netIncome"]);

      double totalAssets = 
        getJsonFloat(jsonData[FIN][BAL][timeUnit][date.c_str()]
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
        getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["totalRevenue"]);
      double costOfRevenue = 
        getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["costOfRevenue"]);

      return (totalRevenue-costOfRevenue)/totalRevenue;

    };

    /**
     * https://www.investopedia.com/terms/o/operatingmargin.asp
    */
    static double calcOperatingProfitMargin(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){
      double  operatingIncome = 
        getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["operatingIncome"]);      
      double totalRevenue =
        getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
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
        trust worthy. I would like more transparency in the least. It's a pity
        that SimFin doesn't have better coverage, as their service is remarkably
        transparent. In any case, I'm tempted to avoid fields that don't 
        actually show up in a quaterly or yearly report.
      */
      double  freeCashFlow = 
        getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["freeCashFlow"]);

      double totalCashFromOperatingActivities = 
        getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["totalCashFromOperatingActivities"]);

      double netIncome = 
        getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
                      ["netIncome"]);

      double capitalExpenditures = 
        getJsonFloat(jsonData[FIN][IS][timeUnit][date.c_str()]
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
           - (principal repaid - new debt)  | + (totalCashFromFinancingActivities -salePurchaseOfStock)

     I'm approximating (principlal repaid - new debt) using 
     (totalCashFromFinancingActivities -salePurchaseOfStock) because

       Net payments related to stock-based award activities
     + *Repurchase of stock
     + Proceeds from the issuance of debt, net of costs
     + Repayment of debt
     + Proceeds from sale of interest in consolidated entities
     = *net cash used in financing activities                    

     Of these items I have only the * quantities in EOD, and so

     + Proceeds from the issuance of debt, net of costs
     + Repayment of debt
       
       is approximately 

     + Proceeds from the issuance of debt, net of costs
     + Repayment of debt
     + Proceeds from sale of interest in consolidated entities
        
      which is equal to

      (totalCashFromFinancingActivities -salePurchaseOfStock)
    ----------
     This definition of free-cash-flow-to-equity as defined by investopedia
     is a little easier to work with given EODs data

     https://www.investopedia.com/terms/f/freecashflowtoequity.asp

     FCFE = cash from operations | totalCashFromOperatingActivities
           -capital expenditure  | - capitalExpenditures
           +net debt issued      | + (totalCashFromFinancingActivities 
                                      -salePurchaseOfStock)

     This formula also requires the same approximation needed to get at
     net debut issued from EODs limited list of items. The definition from 
     Investopedia will include stock-based compensation (a big line item in 
     some tech companies), deferred income tax, along with changes in assets 
     and liabilities net of acquisitions. 
     
     For the time being I'm implementing Investopedia's definition because
     I'm more confident that I've implemented this correctly.

     Damodaran, A.(2011). The Little Book of Valuation. Wiley.
    */
    static double calcFreeCashFlowToEquity(nlohmann::ordered_json &jsonData, 
                                     std::string &date,
                                     const char *timeUnit){

      
      //Investopedia definition
      double totalCashFromOperatingActivities = 
      getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                    ["totalCashFromOperatingActivities"]);

      double capitalExpenditures = 
        getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["capitalExpenditures"]);

      double totalCashFromFinancingActivities = 
        getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
                      ["totalCashFromFinancingActivities"]);

      double salePurchaseOfStock = 
        getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
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
      //  getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
      //                ["netIncome"]);
      //double depreciation =                                            
      // getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
      //                ["depreciation"]);
      //double otherNonCashItems = 
      //  getJsonFloat(jsonData[FIN][CF][timeUnit][date.c_str()]
      //                ["otherNonCashItems"]);
      //double fcfeD =  netIncome
      //              + depreciation
      //              - capitalExpenditures
      //              - otherNonCashItems
      //              + (totalCashFromFinancingActivities-salePurchaseOfStock);
      
      return fcfeIP;
    }


};