
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
        getJsonFloat(jsonData[FIN][IS][Q][date.c_str()]["ebit"]);

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

      return (netIncome-dividendsPaid) / (longTermDebt+totalShareholderEquity);
    };

    static double calcReturnOnAssets(nlohmann::ordered_json &jsonData, 
                                     std::string &date){
      // Return On Assets
      //  Source: https://www.investopedia.com/terms/r/returnonassets.asp

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