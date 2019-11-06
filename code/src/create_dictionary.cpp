#include <aws/core/Aws.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/Outcome.h>

#include <cpp_redis/cpp_redis>

#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <aws/lambda-runtime/runtime.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iterator>

using namespace aws::lambda_runtime;
char const TAG[] = "LAMBDA_ALLOC";
auto const redis_host = Aws::Environment::GetEnv("QUOTE_REDIS_HOST");
auto const redis_port = Aws::Environment::GetEnv("QUOTE_REDIS_PORT");
std::map<std::string, std::vector<std::string>> dictionary;

void createDictionary(std::string *input) {

  std::istringstream iss(*input);
  std::string word;
  std::string prev_word = "";
  std::string prev_prev_word = "";
  while (iss >> word) {
    if (prev_word != "" && prev_prev_word != "") {
      dictionary[prev_prev_word + " " + prev_word].push_back(word);
    }
    prev_prev_word = prev_word;
    prev_word = word;
  }
}

void saveToRedis() {
  std::cout << "Redis: connecting to " << redis_host << ":" << redis_port << std::endl;
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
  for (auto it = dictionary.begin(); it != dictionary.end(); ++it) {
    std::ostringstream oss;
    std::copy(it->second.begin(), it->second.end(),
              std::ostream_iterator<std::string>(oss, " "));
    auto set = client.set(it->first, oss.str());
    client.sync_commit();
  }
}


static invocation_response my_handler(aws::lambda_runtime::invocation_request const& req, Aws::S3::S3Client const& client)
{
  Aws::Utils::Json::JsonValue eventJson(req.payload.c_str());

  auto records = eventJson.View().GetArray("Records");

  auto key = records[0].GetObject("s3").GetObject("object").GetString("key");
  auto bucket = records[0].GetObject("s3").GetObject("bucket").GetString("name");

  std::cout << "Downloading " << key << " from " << bucket << std::endl;

  Aws::String base64_encoded_file;
  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket(bucket);
  request.SetKey(key);
  auto outcome = client.GetObject(request);
  std::cout << "Sent request" << std::endl;
  if (outcome.IsSuccess()) {
    auto& documentStream = outcome.GetResult().GetBody();
    std::ostringstream ss;
    ss << documentStream.rdbuf();
    auto document = ss.str();
    createDictionary(&document);
    saveToRedis();
  } else {
    auto error = outcome.GetError();
    std::cout << "ERROR: " << error.GetExceptionName() << ": " 
        << error.GetMessage() << std::endl;
  }


  return invocation_response::success("{\"hello\":\"world\"}", "application/json");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
    return [] {
        return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
            "console_logger", Aws::Utils::Logging::LogLevel::Trace);
    };
}

int main() {
  Aws::SDKOptions options;
  options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
  options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();

  Aws::InitAPI(options);
  {
    Aws::Client::ClientConfiguration config;
    config.region = Aws::Environment::GetEnv("AWS_REGION");
    std::cout << "Region: " << config.region << std::endl;
    config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";

    auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
    Aws::S3::S3Client client(credentialsProvider, config);
    auto handler_fn = [&client] (aws::lambda_runtime::invocation_request const& req) {
      return my_handler(req, client);
    };
    run_handler(handler_fn);
  }
  Aws::ShutdownAPI(options);
  return 0;
}