
#include <cstdio>
#include <fstream>
#include <string>
#include <regex>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"

#include <boost/algorithm/string/classification.hpp> // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp> // Include for boost::split
#include <boost/algorithm/string.hpp> // Include for boost::iequals

unsigned int COLUMN_WIDTH = 30;

struct primaryTickerPatchData {
  std::string currency;
  std::vector< std::string > exchanges;
  std::vector< std::string > tickersInError;
  std::vector< bool > patchFound;
};


unsigned int calcLengthOfMatchingWords(std::string& strA, std::string& strB){
  std::vector<std::string> wordsA,wordsB;
  boost::split(wordsA, strA, boost::is_any_of(" "), boost::token_compress_on);
  boost::split(wordsB, strB, boost::is_any_of(" "), boost::token_compress_on);

  unsigned int lengthOfMatchingWords=0;
  for(unsigned int i=0; i<wordsA.size();++i){
    for(unsigned int j=0; j<wordsB.size();++j){
      if(boost::iequals(wordsA[i],wordsB[j])){
        lengthOfMatchingWords += wordsA[i].length();
      }
    }
  }

  return lengthOfMatchingWords;
}

void trim(std::string& str,
          const std::string& whitespace)
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin != std::string::npos){
      const auto strEnd = str.find_last_not_of(whitespace);
      const auto strRange = strEnd - strBegin + 1;
      str = str.substr(strBegin, strRange);
    }

}
// Function to find Levenshtein distance between string `X` and `Y`.
// `m` and `n` is the total number of characters in `X` and `Y`, respectively
// This source code comes from:
//    https://www.techiedelight.com/levenshtein-distance-edit-distance-problem/
// which was described in
//    https://en.wikipedia.org/wiki/Levenshtein_distance
unsigned int calcLevenshteinDistance(std::string& x, std::string& y)
{
    unsigned int m = x.length();
    unsigned int n = y.length();  

    // For all pairs of `i` and `j`, `T[i, j]` will hold the Levenshtein distance
    // between the first `i` characters of `X` and the first `j` characters of `Y`.
    // Note that `T` holds `(m+1)×(n+1)` values.
    int T[m + 1][n + 1];
 
    // initialize `T` by all 0's
    std::memset(T, 0, sizeof(T));
 
    // we can transform source prefixes into an empty string by
    // dropping all characters 
    for (int i = 1; i <= m; ++i){
        T[i][0] = i;                // (case 1)
    }
 
    // we can reach target prefixes from empty source prefix
    // by inserting every character 
    for (int j = 1; j <= n; ++j){
        T[0][j] = j;                // (case 1)
    }
 
    int substitutionCost;
 
    // fill the lookup table in a bottom-up manner
    for (int i = 1; i <= m; ++i){
        for (int j = 1; j <= n; ++j){
            if (x[i - 1] == y[j - 1]) {                 // (case 2)
                substitutionCost = 0;                   // (case 2)
            } else{
                substitutionCost = 1;                   // (case 3c)
            }
            T[i][j] = std::min(std::min(T[i - 1][j] + 1,      // deletion (case 3b)
                                        T[i][j - 1] + 1),     // insertion (case 3a)
                      T[i - 1][j - 1] + substitutionCost);    // replace (case 2 & 3c)
        }
    }
    return T[m][n];
}

int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string scanFileName;
  std::string exchangeListFileName;
  std::string exchangeSymbolListFolder;
  std::string outputFolder;
  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will read the output from the scan "
    "function and will try to find patches for the problems identified "
    "by scan. For example, a company that is on a German "
    "exchange, does its accounting in USD, yet does not have a "
    "PrimaryTicker entry is probably in error: this company does not "
    "reside in Germany if it does its accounting in USD and so it must "
    "have a missing PrimaryTicker. Information from the exchange list, "
    "and exchange-symbol-lists is searched to find a company that resides"
    "on an exchange with the same currency (USD in this example) and has "
    "a very similar company name."
    ,' ', "0.0");




    TCLAP::ValueArg<std::string> scanFileNameInput("c",
      "scan_file_name", 
      "The full path and file name for the file produced by the scan "
      "function (e.g. STU.scan.json).",
      true,"","string");

    cmd.add(scanFileNameInput);

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","EXCHANGE_CODE", 
      "The exchange code. For example: US",
      true,"","string");

    cmd.add(exchangeCodeInput);  


    TCLAP::ValueArg<std::string> exchangeSymbolListFolderInput("s",
      "exchange_symbol_list", "A folder that contains json files, one per "
      "exchange, that lists the tickers in each exchange",
      true,"","string");


    cmd.add(exchangeSymbolListFolderInput);  

    TCLAP::ValueArg<std::string> exchangeListFileNameInput("l","exchange_list", 
      "The exchanges-list json file generated by EOD",
      true,"","string");

    cmd.add(exchangeListFileNameInput); 

    TCLAP::ValueArg<std::string> outputFolderInput("o","output_folder_path", 
      "The path to the folder that will contain the output json files "
      "produced by this analysis",
      true,"","string");

    cmd.add(outputFolderInput);

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    scanFileName              = scanFileNameInput.getValue();
    exchangeCode              = exchangeCodeInput.getValue();
    exchangeListFileName      = exchangeListFileNameInput.getValue(); 
    exchangeSymbolListFolder  = exchangeSymbolListFolderInput.getValue();   
    outputFolder              = outputFolderInput.getValue();
    verbose                   = verboseInput.getValue();

    if(verbose){
      std::cout << "  Scan File Name" << std::endl;
      std::cout << "    " << scanFileName << std::endl;
      
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Exchange List File" << std::endl;
      std::cout << "    " << exchangeListFileName << std::endl;

      std::cout << "  Exchange Symbol List" << std::endl;
      std::cout << "    " << exchangeSymbolListFolder << std::endl;

      std::cout << "  Output Folder" << std::endl;
      std::cout << "    " << outputFolder << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  std::string validFileExtension = exchangeCode;
  validFileExtension.append(".json");



  unsigned int count=0;
  unsigned int errorCount=0;

  using json = nlohmann::ordered_json;
  std::ifstream scanFileStream(scanFileName.c_str());
  json scanResults = json::parse(scanFileStream);

  json patchResults;

  //Scan the list of exhanges for potential patches
  if(verbose){
    std::cout << std::endl;
    std::cout << "Scanning exchange-symbol-lists for companies " << std::endl;
    std::cout << "with the same name. These are stored as a" << std::endl;
    std::cout << "potential PrimaryTickerPatch" << std::endl;
  }

  //load the exchange-list
  std::ifstream exchangeListFileStream(exchangeListFileName.c_str());
  json exchangeList = json::parse(exchangeListFileStream);

  //scan the errors and group them by currency
  std::vector< primaryTickerPatchData > primaryTickerPatchDataVector;

  unsigned int patchCount = 0;


  //1. Sort MissingPrimaryTicker errors by currency.
  //2. Identify the exchanges that use that currency
  //3. Then use this list to look for matching company names so that you 
  //   only have to open each exchange symbol list once.
  for(auto& i : scanResults){

    if(i["MissingPrimaryTicker"].get<bool>()){

      json patchEntry = 
        json::object( 
            { 
              {"Code", i["Code"].get<std::string>()},
              {"Name", i["Name"].get<std::string>()},
              {"Exchange", i["Exchange"].get<std::string>()},
              {"PrimaryTicker", i["PrimaryTicker"].get<std::string>()},
              {"CurrencyCode", i["CurrencyCode"].get<std::string>()},
              {"currency_symbol", i["currency_symbol"].get<std::string>()},
              {"MissingPrimaryTicker", i["MissingPrimaryTicker"].get<bool>()},
              {"MissingCurrencySymbol", i["MissingCurrencySymbol"].get<bool>()},
              {"PatchFound",false},
              {"PatchPrimaryTicker", ""},
              {"PatchName",""},
              {"PatchSimilarityScore",0.}
            }
          );

      patchResults[i["Code"].get<std::string>()]= patchEntry;

      std::string currency_symbol;
      FinancialAnalysisToolkit::
        getJsonString(i["currency_symbol"],currency_symbol);

      bool appendNewPatchData=true;
      for(auto& j : primaryTickerPatchDataVector){
        //If the currency exists, then add to the existing patch
        if( j.currency.compare(currency_symbol)==0){
          appendNewPatchData=false;
          std::string code;
          FinancialAnalysisToolkit::getJsonString(i["Code"],code);            
          j.tickersInError.push_back(code);
          j.patchFound.push_back(false);
        }
      }

      //If the currency doesn't exist, then make a new entry
      if(appendNewPatchData){
        primaryTickerPatchData patch;
        patch.currency = currency_symbol;

        std::string code;
        FinancialAnalysisToolkit::getJsonString(i["Code"],code);            
        patch.tickersInError.push_back(code);
        patch.patchFound.push_back(false);

        //Make a list that contains every exchange that uses this currency
        for(auto& itExc : exchangeList){
          std::string exchangeCurrency;
          FinancialAnalysisToolkit:: 
            getJsonString(itExc["Currency"],exchangeCurrency);
            if(patch.currency.compare(exchangeCurrency)==0){
              std::string exchangeCode;
              FinancialAnalysisToolkit::getJsonString(itExc["Code"],exchangeCode);
              patch.exchanges.push_back(exchangeCode);
            }
        }
        primaryTickerPatchDataVector.push_back(patch);
      }
    }
  }

  //The data in primaryTickerPatchDataVector has been organized so that
  //the file for a specific exchange only needs to be opened once to 
  //reduce the run time.
  for (auto& itPatchData : primaryTickerPatchDataVector){
    for (auto& itExc : itPatchData.exchanges ){
      //load the exchange-symbol-list
      std::string exchangeSymbolListPath = exchangeSymbolListFolder;
      exchangeSymbolListPath.append(itExc);
      exchangeSymbolListPath.append(".json");
      std::ifstream exchangeSymbolListFileStream(exchangeSymbolListPath);
      json exchangeSymbolList = json::parse(exchangeSymbolListFileStream);        

      //for(auto& indexTicker : itPatchData.tickersInError){
      for(unsigned int indexTicker=0; 
            indexTicker<itPatchData.tickersInError.size();  ++indexTicker){


        std::string companyNameInError;
        std::string tickerName = itPatchData.tickersInError[indexTicker];
        FinancialAnalysisToolkit::
          getJsonString(scanResults[tickerName]["Name"],companyNameInError);

        //Remove text between pairs of ( ) and - -
        companyNameInError = std::regex_replace(companyNameInError, 
                                                std::regex("-.*-.*\\(.*\\)"), "");
        //Remove non-alphanumeric characters                                                
        companyNameInError = std::regex_replace(companyNameInError, 
                                                std::regex("[^a-zA-Z\\d\\s:]"), "");
        //Trim leading and following whitespace
        trim(companyNameInError," ");

        unsigned int minScoreA = std::round(companyNameInError.length()*0.5); //Just to filter out Tbk, GmbH, etc
        if(minScoreA < 5){
          minScoreA = 5;
        }

        bool candidateFound = false;
        unsigned int score  = 0;
        double bestScore    = patchResults[tickerName]["PatchSimilarityScore"];

        std::string bestName;
        std::string bestCode;

        std::string companyName;

        for(auto& itSym : exchangeSymbolList){
          FinancialAnalysisToolkit::getJsonString(itSym["Name"],companyName);
          
          //Remove text between pairs of ( ) and - -
          //companyName = std::regex_replace(companyName, 
          //                                std::regex("-.*-.*\\(.*\\)"), "");
          //Remove non-alphanumeric characters                                                                          
          //companyName = std::regex_replace(companyName, 
          //                               std::regex("[^a-zA-Z\\d\\s:]"), "");
          //Trim leading and following whitespace
          //trim(companyNameInError," ");


          //The LevenshteinDistance is really neat, but it fails when
          //the word that would match best has additional stuff added to it
          //that is longer. In this case a company that doesn't share any
          //exact matching words but has the same length can have the 
          //lowest Levenshtein distance.
          //
          //What I really want to know is how many words in each string
          //exactly match, and to calculate those matching characters as
          //the score.
          //distance = 
          //  calcLevenshteinDistance(companyName,companyNameInError);

          score=calcLengthOfMatchingWords(companyNameInError,companyName);

          if(companyName.compare("Jasa Marga Tbk")==0 
            && companyNameInError.compare("JASA MARGA")==0){
              bool here=true;
            }

          unsigned int minScoreB = std::round(companyName.length()*0.5); //Just to filter out Tbk, GmbH, etc
          if(minScoreB < 5){
            minScoreB = 5;
          }

          if(score > bestScore && score > minScoreA && score > minScoreB ){
            bestScore=score;
            candidateFound=true;
            FinancialAnalysisToolkit::
              getJsonString(itSym["Name"],bestName);            
            FinancialAnalysisToolkit::
              getJsonString(itSym["Code"],bestCode);
          }
        }

        if(candidateFound==true){
            std::string ex;
            
            bestCode.append(".");
            bestCode.append(itExc);
            patchResults[tickerName]["PatchFound"] = true;
            patchResults[tickerName]["PatchPrimaryTicker"] = bestCode;
            patchResults[tickerName]["PatchName"]=bestName;
            patchResults[tickerName]["PatchSimilarityScore"] = bestScore;

            itPatchData.patchFound[indexTicker]=true;

        }
      }

    }
  }
  //Report on the items that were patched and not.
  if(verbose){
    std::cout << std::endl;
    
    std::cout << "Patches found for : " << std::endl;
    unsigned int  countPatches = 0;
    for(auto& itData : primaryTickerPatchDataVector){
      for(unsigned int i=0; i<itData.patchFound.size();++i){
        if(itData.patchFound[i]==true){
          std::string companyNameInError("");
          FinancialAnalysisToolkit::
            getJsonString(scanResults[itData.tickersInError[i]]["Name"],
                          companyNameInError);
          ++countPatches;
          std::cout << countPatches 
              << ".  " << itData.tickersInError[i] << std::endl
              << "  " << companyNameInError << std::endl
              << "  " << patchResults[itData.tickersInError[i]]
                                     ["PatchName"].get<std::string>() 
              << std::endl;
        }
      }
    }
    std::cout << std::endl << std::endl;
    std::cout << "Patches could not be found for : " << std::endl;
    unsigned int  countMissingPatches = 0;
    for(auto& itData : primaryTickerPatchDataVector){
      for(unsigned int i=0; i<itData.patchFound.size();++i){
        if(itData.patchFound[i]==false){
          std::string companyNameInError("");
          FinancialAnalysisToolkit::
            getJsonString(scanResults[itData.tickersInError[i]]["Name"],
                          companyNameInError);
          ++countMissingPatches;
          std::cout << countMissingPatches 
              << ".  " << itData.tickersInError[i] << std::endl
              << "  " << companyNameInError  << std::endl;
          std::cout << "  " << itData.currency << ": ";
          for(unsigned int j=0; j<itData.exchanges.size();++j){
            std::cout << itData.exchanges[j] << " " ;
          }
          std::cout << std::endl;
        }
      }
    }
  }
    



  std::string outputFilePath(outputFolder);
  std::string outputFileName(exchangeCode);
  outputFileName.append(".patch");
  outputFileName.append(".json");
  
  //Update the extension 
  outputFilePath.append(outputFileName);

  std::ofstream outputFileStream(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << patchResults;
  outputFileStream.close();

  std::cout << "success" << std::endl;

  return 0;
}
