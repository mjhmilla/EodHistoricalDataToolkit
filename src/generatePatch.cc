
#include <cstdio>
#include <fstream>
#include <string>


#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "JsonFunctions.h"
#include "StringToolkit.h"
#include "CurlToolkit.h"

const char TRADING_VIEW_URL[] = "https://www.tradingview.com/symbols/";
const char GOOGLE_SEARCH_URL[] = "https://www.google.com/search?q=";


struct primaryTickerPatchData {
  std::string currency;
  std::vector< std::string > exchanges;
  std::vector< std::string > tickersInError;
  std::vector< bool > patchFound;
};


const double MIN_MATCHING_WORD_FRACTION = 0.3;


int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string scanFileName;
  std::string exchangeListFileName;
  std::string mainExchangeListFileName;
  std::string exchangeSymbolListFolder;
  std::string outputFolder;
  std::string tradingViewExchangeCodes;
  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will read the output from the scan "
    "function and will try to find patches for the problems identified "
    "by scan. For example, a company that is on a German "
    "exchange, but has an ISIN from another country, its PrimaryTicker should"
    "also be from that country. If the ISIN is missing, the currency used for "
    "accounting should be the currency of the exchange listed in the "
    "PrimaryTicker. If these consistency checks fail, then the appropriate"
    "exchange-symbol-lists to find either a company with a matching ISIN, "
    "or a company with a very similar name."
    ,' ', "0.0");




    TCLAP::ValueArg<std::string> scanFileNameInput("c",
      "scan_file_name", 
      "The full path and file name for the file produced by the scan "
      "function (e.g. STU.scan.json).",
      true,"","string");

    cmd.add(scanFileNameInput);

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
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

    TCLAP::ValueArg<std::string> mainExchangeListFileNameInput("m","main_exchange_list", 
      "The main/largest exchange for this country. For example, in Germany the "
      "Frankfurt Exchange is the largest and can be used as a reference for "
      "entries that are missing in the smaller exchanges in Germany "
      "(e.g. Stuttgart)",
      false,"","string");

    cmd.add(mainExchangeListFileNameInput);  


    TCLAP::ValueArg<std::string> tradingViewExchangeCodeInput("t",
      "trading_view_exchange_code", 
      "ISIN data can be patched by searching www.tradingview.com. TradingView "
      "has its own codes for each exchange. Pass in the code that corresponds"
      "to the exchange of interest here. This feature requires that you have"
      "an account to TradingView and be logged in to function. This feature"
      "also assumes that the first occurance of \"isin\" in the html returned"
      "by TradingView is followed by the isin number."
      "(e.g. SWB for the Stuttgart Boerse)",
      false,"","string");

    cmd.add(tradingViewExchangeCodeInput);  




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
    mainExchangeListFileName  = mainExchangeListFileNameInput.getValue();
    exchangeListFileName      = exchangeListFileNameInput.getValue(); 
    exchangeSymbolListFolder  = exchangeSymbolListFolderInput.getValue();  
    tradingViewExchangeCodes   = tradingViewExchangeCodeInput.getValue(); 
    outputFolder              = outputFolderInput.getValue();
    verbose                   = verboseInput.getValue();

    if(verbose){
      std::cout << "  Scan File Name" << std::endl;
      std::cout << "    " << scanFileName << std::endl;
      
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Exchange List File" << std::endl;
      std::cout << "    " << exchangeListFileName << std::endl;

      std::cout << "  Main Exchange List File" << std::endl;
      std::cout << "    " << mainExchangeListFileName << std::endl;

      std::cout << "  Exchange Symbol List" << std::endl;
      std::cout << "    " << exchangeSymbolListFolder << std::endl;

      std::cout << "  TradingView exchange code" << std::endl;
      std::cout << "    " << tradingViewExchangeCodes << std::endl;

      std::cout << "  Output Folder" << std::endl;
      std::cout << "    " << outputFolder << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  //Break out the set of exchanges to query
  std::vector< std::string > tradingViewExchangeList;
  if(tradingViewExchangeCodes.length()>0){
    size_t idxA=0;
    size_t idxB = tradingViewExchangeCodes.find_first_of('-');
    std::string exchange;
    while(idxB != std::string::npos){
      exchange = tradingViewExchangeCodes.substr(idxA,idxB-idxA);
      tradingViewExchangeList.push_back(exchange);
      idxA = idxB+1;           
      idxB = tradingViewExchangeCodes.find_first_of('-',idxA);
    }
    idxB = tradingViewExchangeCodes.length();
    exchange = tradingViewExchangeCodes.substr(idxA,idxB);
    tradingViewExchangeList.push_back(exchange);

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
    std::cout << "Scanning exchange-symbol-lists for companies "    <<std::endl;
    std::cout << "with the same ISIN. If the ISIN is not available" <<std::endl;
    std::cout << " a company with a similar name is searched. "     <<std::endl;
    std::cout << "Patches are stored in separate files depending " << std::endl;
    std::cout << " on the type of match that has been made. " << std::endl;
  }

  //load the exchange-list
  std::ifstream exchangeListFileStream(exchangeListFileName.c_str());
  json exchangeList = json::parse(exchangeListFileStream);

  //load the main-exchange-list
  json mainExchangeList;
  bool mainExchangeAvailable=false;
  if(mainExchangeListFileName.length()>0){
    std::ifstream mainExchangeListFileStream(mainExchangeListFileName.c_str());
    mainExchangeList = json::parse(mainExchangeListFileStream);
    mainExchangeAvailable=true;
  }

  //scan the errors and group them by currency
  std::vector< primaryTickerPatchData > primaryTickerPatchDataVector;

  unsigned int patchCount = 0;



  if(verbose){
    std::cout << std::endl;
    std::cout << "Constructing the table of companies with missing "
                 "primary tickers " <<std::endl;
  }
  //1. Sort MissingPrimaryTicker errors by currency.
  //2. Identify the exchanges that use that currency
  //3. Then use this list to look for matching company names so that you 
  //   only have to open each exchange symbol list once.

  for(auto& iterScan : scanResults){

    //Patch the ISIN from the main exchange, if its available
//    std::string isin("");
    std::string isin("");
    std::string isinSrc("");
    //std::string eodIsin(""); //EOD ISIN
    //std::string tvIsin(""); //Trading View ISIN
    //std::string googleIsin(""); //ISIN number scanned from a google search

    JsonFunctions::getJsonString(iterScan["ISIN"],isin);
    isinSrc="EOD";
    if(isin.length()==0){
      if(mainExchangeAvailable){
        auto iterExchange=mainExchangeList.begin();
        bool found=false;
        std::string nameA,nameB;
        JsonFunctions::getJsonString(iterScan["Name"],nameA);

        while(iterExchange != mainExchangeList.end() && !found){
          JsonFunctions::getJsonString((*iterExchange)["Name"],nameB);
          if( nameA.compare(nameB) == 0 ){
            JsonFunctions::getJsonString((*iterExchange)["Isin"],isin);
            found=true;
          }
          ++iterExchange;
        }
      }
      isinSrc="EOD";
    }



    if(iterScan["MissingPrimaryTicker"].get<bool>()){

      std::string code("");
      JsonFunctions::getJsonString(iterScan["Code"],code);
      std::string name("");
      JsonFunctions::getJsonString(iterScan["Name"],name);
      std::string exchange("");
      JsonFunctions::getJsonString(iterScan["Exchange"],exchange);
      std::string primaryTicker("");
      JsonFunctions::getJsonString(iterScan["PrimaryTicker"],primaryTicker);


      ++patchCount;
      if(verbose){
        std::cout << '\n' << '\n'
          << patchCount << "." << '\t' << exchange << ":" << code << std::endl;
        std::cout << '\t' << '\"' << name <<'\"' << std::endl;
        std::cout << '\t' << "          EOD ISIN: "<<isin;  
      }

      //Query trading view if available
      if(isin.length()==0 && tradingViewExchangeList.size() > 0){

        if(code.length()>0){
          bool found = false;
          size_t iter=0;
          std::string tvExchange;

          while(found == false && iter < tradingViewExchangeList.size()){
            tvExchange = tradingViewExchangeList[iter];


            std::string tradingViewSearchUrl(TRADING_VIEW_URL);
            tradingViewSearchUrl.append(tradingViewExchangeList[iter]);
            tradingViewSearchUrl.append("-");
            tradingViewSearchUrl.append(code);
            tradingViewSearchUrl.append("/");
            std::string tradingViewSearchResult;
            bool downloadSuccessful = 
              CurlToolkit::downloadHtmlToString(tradingViewSearchUrl,
                                                tradingViewSearchResult,
                                                false);                                           
            if(downloadSuccessful){
              //std::string fname = code;
              //fname.append(".html");   
              //std::ofstream file(fname);       
              //file << tradingViewSearchResult;
              //file.close();

              //search through the html code for "isin"
              //bool here=true;
              std::string isinStr("\"isin\":");
              std::size_t idx = tradingViewSearchResult.find(isinStr);
              if(idx != std::string::npos){
                std::size_t idxQuoteA = 
                  tradingViewSearchResult.find_first_of('\"',idx+isinStr.length());
                std::size_t idxQuoteB = 
                  tradingViewSearchResult.find_first_of('\"',idxQuoteA+1);
                if(idxQuoteA != std::string::npos && 
                  idxQuoteB != std::string::npos){
                  isin = tradingViewSearchResult.substr(idxQuoteA+1,
                                                        idxQuoteB-idxQuoteA-1);
                  isinSrc = "TradingView ";
                  isinSrc.append(tvExchange);
                  found=true;                                             


                }
              }
            }
            ++iter;
          }
          if(verbose){
            std::cout << '\n' << '\t' << "Tradingview's ISIN: ";
            if(isin.length()>0){
              std::cout << isin << " (" << tvExchange << ")";
            }                     
          }


        }

      }

      //Query google
      if(isin.length()==0){
        if(verbose){ 
          std::cout  << '\n' << '\t' << "     Google's ISIN: ";
        }

        std::string googleSearchUrl(GOOGLE_SEARCH_URL);
        std::string nameMod = name;
        size_t idx=nameMod.find(' ',0);
        while(idx != std::string::npos){
          nameMod.at(idx)='+';
          idx=nameMod.find(' ',idx+1);
        }
        googleSearchUrl.append(nameMod);
        googleSearchUrl.append("+");
        googleSearchUrl.append(code);
        googleSearchUrl.append("+ISIN");
        std::string googleSearchResult;
        bool downloadSuccessful = 
          CurlToolkit::downloadHtmlToString(googleSearchUrl,
                                            googleSearchResult,
                                            false); 

        //std::string fname = code;
        //fname.append(".html");   
        //std::ofstream file(fname);       
        //file << googleSearchResult;
        //file.close();

        std::string isinLabel("ISIN");
        //This is the search  
        idx=0;
        idx = googleSearchResult.find(isinLabel, idx); 
        bool found = false;

        while(idx != std::string::npos && !found){

          if(idx != std::string::npos){
            idx=idx+isinLabel.length();
            while(!isalpha(googleSearchResult[idx])){
              ++idx;
            }
            isin = googleSearchResult.substr(idx,12);
            bool validIsin=true;
            size_t idxChar=0;
            //country code
            for(size_t i=0; i<2; ++i){
              if(!isalpha(isin[i])){
                validIsin=false;
              }
            }
            //company-share code
            for(size_t i=2; i<11; ++i){
              if(!isalnum(isin[i])){
                validIsin=false;
              }
            }
            //check digit
            if(!isdigit(isin[11])){
              validIsin=false;
            }

            if(verbose && validIsin){
              std::cout << isin << std::endl;
            }
            if(validIsin){
              found=true;
            }
            idx = googleSearchResult.find(isinLabel, idx);
          }

        }
        if(found){
          isinSrc="Google";
        }else{
          isin="";
          isinSrc="";
        }
        

      }

      json patchEntry = 
        json::object( 
            { 
              {"Code", code},
              {"Name", name},
              {"Exchange", exchange},
              {"ISIN", isin},
              {"ISINSource",isinSrc},
              {"PrimaryTicker", primaryTicker},
              {"PatchFound",false},
              {"PatchPrimaryTicker", ""},
              {"PatchPrimaryExchange", ""},
              {"PatchName",""},
              {"PatchISIN",""},
              {"PatchISINExactMatch",false},
              {"PatchNameSimilarityScore",0.},
              {"PatchNameExactMatch",false},
              {"PatchNameClosest",""},
              {"PatchCodeClosest",""},
              {"PatchExchangeClosest",""},
              {"PatchISINClosest",""},
              {"PatchNameSimilarityScoreClosest",0.},
            }
          );



      patchResults[iterScan["Code"].get<std::string>()]= patchEntry;

      //Retrieve the company's domestic currency
      std::string currency;
      
      if(isin.length()>0){
        auto iterExchange=exchangeList.begin();
        bool found=false;

        while(iterExchange != exchangeList.end() && !found){
          if(  isin.find_first_of((*iterExchange)["CountryISO2"].get<std::string>())==0 
            || isin.find_first_of((*iterExchange)["CountryISO3"].get<std::string>())==0){
            currency =   (*iterExchange)["Currency"].get<std::string>();
            found=true;
          }
          ++iterExchange;
        }
      }
      
      if(currency.length()==0){
        //Retrieve the currency_symbol which appears in the 
        //BalanceSheet        
        JsonFunctions::getJsonString(iterScan["currency_symbol"],currency);
      }


      bool appendNewCurrencyData=true;

      //Add a condition here to append the exchanges that corresponds to
      //the leading digits of the ISIN number
      
      //Scan through the list of existing patches to see if the currency
      //appears in this list or not.
      for(auto& iterPatch : primaryTickerPatchDataVector){
        //If the currency exists, then add to the existing patch
        if( iterPatch.currency.compare(currency)==0){
          appendNewCurrencyData=false;
          std::string code;
          JsonFunctions::getJsonString(iterScan["Code"],code);            
          iterPatch.tickersInError.push_back(code);
          iterPatch.patchFound.push_back(false);          
        }
      }

      //If the currency doesn't appear in the list of tickers to patch, 
      //then make a new entry
      if(appendNewCurrencyData){
        primaryTickerPatchData patch;
        patch.currency = currency;

        std::string code;
        JsonFunctions::getJsonString(iterScan["Code"],code);            
        patch.tickersInError.push_back(code);
        patch.patchFound.push_back(false);

        //Make a list that contains every exchange that uses this currency
        for(auto& iterExchange : exchangeList){
          std::string exchangeCurrency;
          JsonFunctions::getJsonString(iterExchange["Currency"],exchangeCurrency);
            if(patch.currency.compare(exchangeCurrency)==0){
              std::string exchangeCode;
              JsonFunctions::getJsonString(iterExchange["Code"],exchangeCode);
              patch.exchanges.push_back(exchangeCode);
            }
        }
        primaryTickerPatchDataVector.push_back(patch);
      }
    }
  }

  unsigned int numElementsToProcess=0;
  for(auto& iterPatch : primaryTickerPatchDataVector){
    numElementsToProcess += 
      iterPatch.tickersInError.size()*iterPatch.exchanges.size();
  }


  //The data in primaryTickerPatchDataVector has been organized so that
  //the file for a specific exchange only needs to be opened once to 
  //reduce the run time.
  if(verbose){
    std::cout << "Processing "  << std::endl;
  }
  unsigned int tickerCount = 0;
  for (auto& iterPatch : primaryTickerPatchDataVector){

    for (auto& iterExchange : iterPatch.exchanges ){
      //load the exchange-symbol-list
      std::string exchangeSymbolListPath = exchangeSymbolListFolder;
      exchangeSymbolListPath.append(iterExchange);
      exchangeSymbolListPath.append(".json");
      std::ifstream exchangeSymbolListFileStream(exchangeSymbolListPath);
      json exchangeSymbolList = json::parse(exchangeSymbolListFileStream);        

      //for(auto& indexTicker : iterPatch.tickersInError){
      for(unsigned int indexTicker=0; 
            indexTicker<iterPatch.tickersInError.size();  ++indexTicker){

        ++tickerCount;

        //Get the code, isin, name
        std::string code = iterPatch.tickersInError[indexTicker];
        std::string isin = patchResults[code]["ISIN"].get<std::string>();
        std::string nameARaw;
        JsonFunctions::getJsonString(scanResults[code]["Name"],nameARaw);


        if(verbose){
          std::cout << tickerCount << '\t' <<"/" << numElementsToProcess << std::endl;
          std::cout << '\t' << code << std::endl;
          std::cout << '\t' << nameARaw << std::endl;
          std::cout << '\t' << isin << std::endl;
          std::cout << std::endl;
        }        


        bool isinMatchFound= 
          JsonFunctions::getJsonBool(patchResults[code]["PatchISINExactMatch"]);

        if(isin.length()>0 && isinMatchFound == false){
          auto iterSymbol=exchangeSymbolList.begin();
          bool found=false;

          std::string candidateIsin("");

          while(iterSymbol != exchangeSymbolList.end() && !found){
            JsonFunctions::getJsonString((*iterSymbol)["Isin"],candidateIsin);
            if(  isin.compare(candidateIsin)==0){
              patchResults[code]["PatchFound"] = true;
              patchResults[code]["PatchISIN"]                 = isin;
              patchResults[code]["PatchNameSimilarityScore"]  = 1.0;
              patchResults[code]["PatchISINExactMatch"]       = true;

              patchResults[code]["PatchPrimaryTicker"]        = 
                (*iterSymbol)["Code"].get<std::string>();

              patchResults[code]["PatchPrimaryExchange"]      =  iterExchange;

              //The exchange stored here is the acronym of the actual exchange
              //which can differ from the EOD exchange code, which is what we
              //want.
              //patchResults[code]["PatchPrimaryExchange"]      = 
              //  (*iterSymbol)["Exchange"].get<std::string>();

              patchResults[code]["PatchName"]                 = 
                (*iterSymbol)["Name"].get<std::string>();          

              found=true;
              isinMatchFound=true;
            }
            ++iterSymbol;

          }
        }

        //If the ISIN search failed, try to find a company that exists with
        //a similar name
        if(isinMatchFound==false){
          StringToolkit::WordData wordA(nameARaw);

          //Calculate the minimum acceptable score
          bool candidateFound = false;
          double bestScore    = 
            patchResults[code]["PatchNameSimilarityScore"].get<double>();

          std::string bestName;
          std::string bestCode;
          std::string bestIsin;

          for(auto& iterSymbol : exchangeSymbolList){

    
            std::string nameBRaw;
            JsonFunctions::getJsonString(iterSymbol["Name"],nameBRaw);
            StringToolkit::TextData textB(nameBRaw);

            StringToolkit::TextSimilarity simAB;

            simAB.score=0;
            simAB.firstWordsMatch=false;
            simAB.allWordsFound=false;
            simAB.exactMatch=false;

            StringToolkit::evaluateSimilarity(wordA,textB,simAB);

            if( ( simAB.score > bestScore &&
                  simAB.score > MIN_MATCHING_WORD_FRACTION)  ||
                ( simAB.exactMatch)) {
  
              bestScore=simAB.score;
              candidateFound=true;
              JsonFunctions::getJsonString(iterSymbol["Name"],bestName);            
              JsonFunctions::getJsonString(iterSymbol["Code"],bestCode);
              JsonFunctions::getJsonString(iterSymbol["ISIN"],bestIsin);

              //The spelling of ISIN in the json files is not always consistent      
              if(bestIsin.length()==0){
                JsonFunctions::getJsonString(iterSymbol["Isin"], bestIsin);
              }
              if(bestIsin.length()==0){
                JsonFunctions::getJsonString(iterSymbol["isin"], bestIsin);
              }
              

              //bestCode.append(".");
              //bestCode.append(iterExchange);

              patchResults[code]["PatchFound"] = true;
              patchResults[code]["PatchPrimaryTicker"]    = bestCode;
              patchResults[code]["PatchPrimaryExchange"]  = iterExchange;
              patchResults[code]["PatchName"]=bestName;
              patchResults[code]["PatchISIN"]=bestIsin;
              patchResults[code]["PatchNameSimilarityScore"] = bestScore;

              iterPatch.patchFound[indexTicker]=true;

              if(simAB.exactMatch){
                patchResults[code]["PatchNameExactMatch"]=true;
                break;
              }
            }else if( simAB.score > 
              patchResults[code]["PatchNameSimilarityScoreClosest"].get<double>()){
              std::string secondCode,secondName,secondIsin;
              JsonFunctions::getJsonString(iterSymbol["Name"],secondName);            
              JsonFunctions::getJsonString(iterSymbol["Code"],secondCode);            
              JsonFunctions::getJsonString(iterSymbol["ISIN"],secondIsin);

              if(secondIsin.length()==0){
                JsonFunctions::getJsonString(iterSymbol["Isin"], secondIsin);
              }
              if(secondIsin.length()==0){
                JsonFunctions::getJsonString(iterSymbol["isin"], secondIsin);
              }

              patchResults[code]["PatchNameClosest"]=secondName;
              patchResults[code]["PatchCodeClosest"]=secondCode;
              patchResults[code]["PatchExchangeClosest"]=iterExchange;
              patchResults[code]["PatchISINClosest"]=secondIsin;
              patchResults[code]["PatchNameSimilarityScoreClosest"] = simAB.score;
            } 
          }
        }

      }

    }
  }

  //Split patchResults into:
  // patch.json            : exact match
  // patch.candidate.json  : not exact, but met the conditions
  // patch.missing.json    : did not meet conditions

json patchIsinMatchList;
json patchNameMatchList;
json patchNameSimilarList;
json patchMissingList;

for(auto& iterPatch : patchResults){
  bool isinFound  = iterPatch["PatchISINExactMatch"].get<bool>();
  bool patchFound = iterPatch["PatchFound"].get<bool>();
  bool patchExact = iterPatch["PatchNameExactMatch"].get<bool>();

  std::string code;
  JsonFunctions::getJsonString(iterPatch["Code"],code);  
       
  if(isinFound){
    json patchIsinMatchEntry=
      json::object( 
          { 
            {"Name", iterPatch["Name"].get<std::string>()},
            {"Code", iterPatch["Code"].get<std::string>()},
            {"Exchange", iterPatch["Exchange"].get<std::string>()},
            {"ISIN", iterPatch["ISIN"].get<std::string>()},
            {"ISINSource", iterPatch["ISINSource"].get<std::string>()},        
            {"PatchName",iterPatch["PatchName"]},                   
            {"PatchISIN", iterPatch["PatchISIN"].get<std::string>()},  
            {"PrimaryTicker", iterPatch["PatchPrimaryTicker"].get<std::string>()},
            {"PrimaryExchange", iterPatch["PatchPrimaryExchange"].get<std::string>()},            
          }
        );
    patchIsinMatchList[code]=patchIsinMatchEntry; 
  }else if(patchExact){
    json patchExactEntry = 
      json::object( 
          { 
            {"Name", iterPatch["Name"].get<std::string>()},
            {"Code", iterPatch["Code"].get<std::string>()},
            {"Exchange", iterPatch["Exchange"].get<std::string>()},
            {"ISIN", iterPatch["ISIN"].get<std::string>()},   
            {"ISINSource", iterPatch["ISINSource"].get<std::string>()},          
            {"PatchName",iterPatch["PatchName"]},            
            {"PatchISIN", iterPatch["PatchISIN"].get<std::string>()},                               
            {"PrimaryTicker", iterPatch["PatchPrimaryTicker"].get<std::string>()},
            {"PrimaryExchange", iterPatch["PatchPrimaryExchange"].get<std::string>()},            
          }
        );
    patchNameMatchList[code]=patchExactEntry;  
  }else if(!patchExact && patchFound){
    json patchCandidateEntry = 
      json::object( 
          { 
            {"Name", iterPatch["Name"].get<std::string>()},
            {"Code", iterPatch["Code"].get<std::string>()},
            {"Exchange", iterPatch["Exchange"].get<std::string>()},
            {"ISIN", iterPatch["ISIN"].get<std::string>()},
            {"ISINSource", iterPatch["ISINSource"].get<std::string>()},   
            {"PatchName",iterPatch["PatchName"]},
            {"PatchISIN", iterPatch["PatchISIN"].get<std::string>()},                                
            {"PrimaryTicker", iterPatch["PatchPrimaryTicker"].get<std::string>()},            
            {"PrimaryExchange", iterPatch["PatchPrimaryExchange"].get<std::string>()},            
            {"PatchNameSimilarityScore", iterPatch["PatchNameSimilarityScore"].get<double>()},
          }
        );
    patchNameSimilarList[code]=patchCandidateEntry;  

  }else{
//  if(!patchExact && !patchFound){

    std::string patchNameClosest;
    std::string patchCodeClosest;
    std::string patchExchangeClosest;
    std::string patchIsinClosest;
    double patchSimilarityScoreClosest=0;
    JsonFunctions::getJsonString(iterPatch["PatchNameClosest"],patchNameClosest);
    JsonFunctions::getJsonString(iterPatch["PatchCodeClosest"],patchCodeClosest);
    JsonFunctions::getJsonString(iterPatch["PatchExchangeClosest"],patchExchangeClosest);
    JsonFunctions::getJsonString(iterPatch["PatchISINClosest"],patchIsinClosest);
    patchSimilarityScoreClosest= 
      JsonFunctions::getJsonFloat( iterPatch["PatchNameSimilarityScoreClosest"]);

    json patchMissingEntry = 
      json::object( 
          { 
            {"Name", iterPatch["Name"].get<std::string>()},
            {"Code", iterPatch["Code"].get<std::string>()},
            {"Exchange", iterPatch["Exchange"].get<std::string>()},
            {"ISIN", iterPatch["ISIN"].get<std::string>()},
            {"ISINSource", iterPatch["ISINSource"].get<std::string>()},   
            {"PatchNameClosest", patchNameClosest},
            {"PatchCodeClosest", patchCodeClosest},
            {"PatchExchangeClosest",patchExchangeClosest},    
            {"PatchISINClosest",patchIsinClosest},                            
            {"PatchNameSimilarityScoreClosest", patchSimilarityScoreClosest},
          }
        );
    patchMissingList[code]=patchMissingEntry;  
  }



  }

  //Report on the items that were patched and not.
  if(verbose){
    std::cout << std::endl;    
    std::cout << "Patches found with matching ISIN numbers : " << std::endl;
    std::cout << std::endl;

    unsigned int count = 1;
    for(auto& iterPatch : patchIsinMatchList){
      std::cout << count << "." << '\t' 
                << iterPatch["Code"].get<std::string>() << '\t' << '\t'   
                << iterPatch["Name"].get<std::string>() << std::endl
                << '\t' << iterPatch["PrimaryTicker"].get<std::string>() 
        << '\t' << '\t' << iterPatch["PatchName"].get<std::string>() << std::endl;
      std::cout << std::endl;

      ++count;
    }


    std::cout << std::endl;    
    std::cout << "Patches found with matching names : " << std::endl;
    std::cout << std::endl;

    count = 1;
    for(auto& iterPatch : patchNameMatchList){
      std::cout << count << "." << '\t' 
                << iterPatch["Code"].get<std::string>() << '\t' << '\t'   
                << iterPatch["Name"].get<std::string>() << std::endl
                << '\t' << iterPatch["PrimaryTicker"].get<std::string>() 
        << '\t' << '\t' << iterPatch["PatchName"].get<std::string>() << std::endl;
      std::cout << std::endl;

      ++count;
    }

    std::cout << std::endl;    
    std::cout << "Patches found with similar names : " << std::endl;
    std::cout << std::endl;

    count =1;
    for(auto& iterPatch : patchNameSimilarList){
      std::cout << count << "." << '\t' 
                << iterPatch["Code"].get<std::string>() << '\t' 
                << iterPatch["PrimaryTicker"].get<std::string>() << std::endl
                << '\t' << iterPatch["Name"].get<std::string>() << std::endl                
                << '\t' << iterPatch["PatchName"].get<std::string>() << std::endl;

      std::cout << '\t' << iterPatch["PatchNameSimilarityScore"].get<double>() 
                << std::endl << std::endl;
      ++count;
    }


    std::cout << std::endl;    
    std::cout << "Patch missing for : " << std::endl;
    std::cout << std::endl;

    count =1;
    for(auto& iterPatch : patchMissingList){
      std::cout << count << "." << '\t' 
                << iterPatch["Code"].get<std::string>() << std::endl  
                << '\t' << iterPatch["Name"].get<std::string>() << std::endl
                << '\t' << iterPatch["PatchNameClosest"].get<std::string>() << std::endl
                << '\t' << iterPatch["PatchNameSimilarityScoreClosest"].get<double>();                 
      std::cout << std::endl;
      std::cout << std::endl;
      ++count;
    }

  }
    
  //Output the list of matching isin patches
  std::string outputFilePath(outputFolder);
  std::string outputFileName(exchangeCode);
  outputFileName.append(".patch.matching_isin.json");
  outputFilePath.append(outputFileName);

  std::ofstream outputFileStream(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << patchIsinMatchList;
  outputFileStream.close();


  //Output the list of matching by name patches
  outputFilePath = outputFolder;
  outputFileName = exchangeCode;
  outputFileName.append(".patch.matching_name.json");
  outputFilePath.append(outputFileName);

  outputFileStream.open(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << patchNameMatchList;
  outputFileStream.close();

  //Output the list of candidate patches
  outputFilePath = outputFolder;
  outputFileName = exchangeCode;
  outputFileName.append(".patch.similar_name.json");
  outputFilePath.append(outputFileName);

  outputFileStream.open(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << patchNameSimilarList;
  outputFileStream.close();

  //Output the list of missing patches
  outputFilePath = outputFolder;
  outputFileName = exchangeCode;
  outputFileName.append(".patch.missing.json");
  outputFilePath.append(outputFileName);

  outputFileStream.open(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << patchMissingList;
  outputFileStream.close();


  std::cout << "success" << std::endl;

  return 0;
}
