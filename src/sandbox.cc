
#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>
#include <curl/curl.h>

unsigned int COLUMN_WIDTH = 30;

namespace
{
  std::size_t callback(
    const char* in,
    std::size_t size,
    std::size_t num,
    std::string* out)
  {
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
  }
}

int main (int argc, char* argv[]) {

  //use libcurl to get AAPL.US
  const std::string url("https://eodhistoricaldata.com/api/fundamentals/AAPL.US?api_token=demo");

  CURL* curl = curl_easy_init();

  // Set remote URL.
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  // Don't bother trying IPv6, which would increase DNS resolution time.
  curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

  // Don't wait forever, time out after 10 seconds.
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

  // Follow HTTP redirects if necessary.
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  // Response information.
  long httpCode(0);
  std::unique_ptr<std::string> httpData(new std::string());

  // Hook up data handling function.
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

  // Hook up data container (will be passed as the last parameter to the
  // callback handling function).  Can be any pointer type, since it will
  // internally be passed as a void pointer.
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());

  // Run our HTTP GET command, capture the HTTP response code, and clean up.
  curl_easy_perform(curl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
  curl_easy_cleanup(curl);

  std::cout << httpCode << std::endl;

  if (httpCode == 200)
  {
    std::cout << "\nGot successful response from " << url << std::endl;


    //read out all of the json keys
    using json = nlohmann::ordered_json;
    //std::ifstream f("../json/AAPL.US.json");
    json data = json::parse(*httpData.get());


    std::cout << data["General"]["Code"] << '\n';

    for (auto it = data.begin(); it != data.end(); ++it)
    {
        std::cout << "key: " << it.key() << '\n';
        for (auto jt = data[it.key()].begin(); jt != data[it.key()].end(); ++jt)
        {
//          std::cout << std::setw(COLUMN_WIDTH) << jt.key() 
//                    << std::setw(COLUMN_WIDTH) << jt.value() << '\n';
          std::cout << std::setw(COLUMN_WIDTH) << jt.key()  << '\n';          

        }


    }
  }
  std::printf("%s\n","Success");
  return 0;
}
