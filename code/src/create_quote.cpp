#include <aws/core/Aws.h>
#include <aws/core/platform/Environment.h>
#include <aws/lambda-runtime/runtime.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cpp_redis/cpp_redis>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

using namespace aws::lambda_runtime;

int advanceWord(int position, std::string quote) {
  for (int i = position; i < +quote.size(); i++) {
    if (quote[i] == ' ') {
      return i + 1;
    }
  }
  return -1; // No Space? Something went wrong...
}

std::string genereate_quote () {
  auto const redis_host = Aws::Environment::GetEnv("QUOTE_REDIS_HOST");
  auto const redis_port = Aws::Environment::GetEnv("QUOTE_REDIS_PORT");


  int key_start = 0;
  std::string quote = "";
  std::random_device rd;
  std::mt19937 gen(rd());

  cpp_redis::active_logger =
      std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);
  cpp_redis::client client;

  client.connect(std::string(redis_host.c_str()), std::atoi(redis_port.c_str()),
                 [](const std::string &host, std::size_t port,
                    cpp_redis::client::connect_state status) {
                   if (status == cpp_redis::client::connect_state::dropped) {
                     std::cout << "client disconnected from " << host << ":"
                               << port << std::endl;
                   }
                 });
  auto start_key = client.randomkey();
  client.sync_commit();
  auto key = start_key.get().as_string();
  std::cout << key << std::endl;
  quote += key;

  while (quote.back() != '.' && quote.back() != '?' && quote.back() != '!') {
    const auto key = quote.substr(key_start, quote.size());
    auto values_req = client.get(key);
    client.sync_commit();
    auto values = values_req.get().as_string();

    if (values == "") {
      return quote;
    } else {
      std::vector<std::string> words;
      std::istringstream s(values);
      words.insert(words.end(), std::istream_iterator<std::string>(s),
                   std::istream_iterator<std::string>());

      std::uniform_int_distribution<> word_dis(0, words.size() - 1);
      int index = word_dis(gen);
      quote += " " + words[index];
    }
    key_start = advanceWord(key_start, quote);
  }

}

static invocation_response my_handler(invocation_request const &req) {
  // response must be formatted according to https://aws.amazon.com/premiumsupport/knowledge-center/malformed-502-api-gateway/
  try {
    auto const quote = genereate_quote();
    Aws::Utils::Json::JsonValue response;
    response.WithString("body", quote.c_str()).WithInteger("statusCode", 200).WithBool("isBase64Encoded", false);
    // Aws::String aws_quote(quote.c_str(), quote.size());
    // Aws::String aws_quote_key("quote");
    // response.WithString(aws_quote_key, aws_quote);
    auto const apig_response = response.View().WriteCompact();
    // return invocation_response::success(quote, "application/json");
    return invocation_response::success(apig_response.c_str(), "application/json");
  } catch (const std::exception &e) {
    return invocation_response::failure(e.what(), "application/json");
  }
}

int main() {
  Aws::SDKOptions options;
  Aws::InitAPI(options);
  run_handler(my_handler);
  Aws::ShutdownAPI(options);
  return 0;
}