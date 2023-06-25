
#include <string>

#include <nlohmann/json.hpp>

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
                                              std::string &date){
      // Return On Capital Deployed
      //  Source: https://www.investopedia.com/terms/r/roce.asp
      double longTermDebt = 
        getJsonFloat(jsonData[FIN][BAL][Q][date.c_str()]["longTermDebt"]);       

      double totalShareholderEquity = 
        getJsonFloat(jsonData[FIN][BAL][Q][date.c_str()]["totalStockholderEquity"]);

      double  ebit = 
        getJsonFloat(jsonData[FIN][IS][Q][date.c_str()]["operatingIncome"]);

      //There are two definitions for capital depoloyed, and they should
      //be the same. These values are not the same for Apple, but are within
      //30% of one and other.  
      //1. capitalDeployed = (longTermDebt+totalShareholderEquity);
        //2. capitalDeployed = (totalAssets-currentTotalLiabilities);

        return ebit / (longTermDebt+totalShareholderEquity);
      };

    static double calcReturnOnInvestedCapital(nlohmann::ordered_json &jsonData, 
                                              std::string &date){
      // Return On Invested Capital
      //  Source: https://www.investopedia.com/terms/r/returnoninvestmentcapital.asp
      double longTermDebt = 
        getJsonFloat(jsonData[FIN][BAL][Q][date.c_str()]["longTermDebt"]);       

      double totalShareholderEquity = 
        getJsonFloat(jsonData[FIN][BAL][Q][date.c_str()]["totalStockholderEquity"]);

      double  netIncome = 
        getJsonFloat(jsonData[FIN][CF][Q][date.c_str()]["netIncome"]);

      //Interesting fact: dividends paid can be negative. This would have
      //the effect of increasing the ROIC for a misleading reason.
      double  dividendsPaid = 
        getJsonFloat(jsonData[FIN][CF][Q][date.c_str()]["dividendsPaid"]);
      
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
                                     std::string &date){

      double  netIncome = 
        getJsonFloat(jsonData[FIN][CF][Q][date.c_str()]["netIncome"]);

      double totalAssets = 
        getJsonFloat(jsonData[FIN][BAL][Q][date.c_str()]["totalAssets"]);

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
                                     std::string &date){
      
      double totalRevenue =
        getJsonFloat(jsonData[FIN][IS][Q][date.c_str()]["totalRevenue"]);
      double costOfRevenue = 
        getJsonFloat(jsonData[FIN][IS][Q][date.c_str()]["costOfRevenue"]);

      return (totalRevenue-costOfRevenue)/totalRevenue;

    };

    /**
     * https://www.investopedia.com/terms/o/operatingmargin.asp
    */
    static double calcOperatingProfitMargin(nlohmann::ordered_json &jsonData, 
                                     std::string &date){
      double  operatingIncome = 
        getJsonFloat(jsonData[FIN][IS][Q][date.c_str()]["operatingIncome"]);      
      double totalRevenue =
        getJsonFloat(jsonData[FIN][IS][Q][date.c_str()]["totalRevenue"]);
      return operatingIncome/totalRevenue;
    };    

    /**
     * https://corporatefinanceinstitute.com/resources/accounting/cash-conversion-ratio/
    */
    static double calcCashConversionRatio(nlohmann::ordered_json &jsonData, 
                                     std::string &date){
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
      */
      double  freeCashFlow = 
        getJsonFloat(jsonData[FIN][CF][Q][date.c_str()]["freeCashFlow"]);

      double netIncome = 
        getJsonFloat(jsonData[FIN][IS][Q][date.c_str()]["netIncome"]);

      return freeCashFlow/netIncome;

    };

/*  
  Enterprise value cannot be evaluated without knowing
  the market value ... and this cannot be calculated without
  having the share price.
  double calcReturnOnEnterprise(nlohmann::ordered_json &jsonData, 
                            std::string &date){
    // Return On Enterprise
    //  According to Joel Greenblatt the entprise value of a company
  //  is quite hard to manipulate, and so the return on enterprise
  //  is less likely to be spun by creative accounting.  
  // 
  //  For enterprise value:
  //     EV = MC + TotalDebit - C
  //     MC: market capitalization
  //     Total Debt:
  //     Cash and cash equivalents
  //  https://www.investopedia.com/terms/e/enterprisevalue.asp
  double  netIncome = 
    getJsonFloat(jsonData[FIN][CF][Q][date.c_str()]["netIncome"]);

  double marketCapitalization = 
    getJsonFloat(jsonData[FIN][BAL][Q][date.c_str()]["commonStockSharesOutstanding"]);

  //There are two definitions for capital depoloyed, and they should
  //be the same. These values are not the same for Apple, but are within
  //30% of one and other.  
  //1. capitalDeployed = (longTermDebt+totalShareholderEquity);
  //2. capitalDeployed = (totalAssets-currentTotalLiabilities);

  return netIncome / totalAssets;
}
*/

};