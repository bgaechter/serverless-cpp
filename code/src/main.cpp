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

std::map<std::string, std::vector<std::string>> dictionary;

int advanceWord(int position, std::string quote) {
  for (int i = position; i < +quote.size(); i++) {
    if (quote[i] == ' ') {
      return i + 1;
    }
  }
  return -1; // No Space? Something went wrong...
}

std::string createQuote() {
  int key_start = 0;
  std::string quote = "";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> key_dis(1, dictionary.size());
  int start = key_dis(gen);
  for (auto it = dictionary.begin(); start > 0; ++it) {
    --start;
    if (start == 0) {
      quote += it->first;
    }
  }
  while (quote.back() != '.' && quote.back() != '?' && quote.back() != '!') {
    const auto key = quote.substr(key_start, quote.size());
    if (dictionary.count(key) == 0) {
      return quote;
    } else {
      auto words = dictionary[key];

      std::uniform_int_distribution<> word_dis(0, words.size() - 1);
      int index = word_dis(gen);
      quote += " " + words[index];
    }
    key_start = advanceWord(key_start, quote);
  }

  return quote;
}

void sanitizeInput(std::string *input) {
  input->erase(
      std::remove_if(input->begin(), input->end(), [](unsigned char x) {
        return !(std::isalnum(x) || std::isspace(x) || x == '.' || x == '?' ||
                 x == '!');
      }));
  std::transform(input->begin(), input->end(), input->begin(),
                 [](unsigned char x) { return std::tolower(x); });
}

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
  cpp_redis::active_logger =
      std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);
  cpp_redis::client client;

  client.connect("127.0.0.1", 6379,
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

std::string createQuoteFromRedis() {

  int key_start = 0;
  std::string quote = "";
  std::random_device rd;
  std::mt19937 gen(rd());

  cpp_redis::active_logger =
      std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);
  cpp_redis::client client;

  client.connect("127.0.0.1", 6379,
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
    

  return quote;
}

int main(int argc, char *argv[]) {
  std::string input_file("input");
  if (argc == 2) {
    input_file = std::string(argv[1]);
  }
  std::ifstream t(input_file);
  std::string the_input((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());

  std::cout << "Starting this thing - creating dictionary" << std::endl;
  sanitizeInput(&the_input);
  createDictionary(&the_input);
  // saveToRedis();
  std::cout << "Dictionary created - creating quote" << std::endl;
  std::cout << createQuote() << std::endl;

  std::cout << "DEBUG" << std::endl;
  auto redis_quote = createQuoteFromRedis();
  std::cout << redis_quote << std::endl;
  return 0;
}