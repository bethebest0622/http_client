#pragma once
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

// #include <curl/curl.h>
#include <drogon/drogon.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <trantor/net/EventLoopThreadPool.h>

#include "util/file_util.h"

namespace util::curl_util {
  typedef rapidjson::Document MyJson;
  void Show(const rapidjson::Document& doc, const std::string& topic = "");
  void Show(const rapidjson::Value&, const std::string& = "");
  std::string ToStr(const rapidjson::Document& doc, const std::string & topic = "");
  std::string ToStr(const rapidjson::Value&, const std::string & = "");

  std::string hmac_sha256(const std::string &, const std::string &, bool = false);
  std::string hmac_sha512(const std::string &, const std::string &, bool = false);
  std::string ed25519_encrypt(const std::string &, const std::string &);

  std::string hex_hmac(const std::string &, const std::string &);

  std::string url_transform(const std::string &);

  struct Handler {
    explicit Handler(const std::string &, trantor::EventLoop *);
    // virtual ~Handler();
    Handler(const Handler&);
    drogon::HttpClientPtr ptr_;
    drogon::HttpRequestPtr req_ptr_;
    std::mutex mut_;
    Handler& operator=(const Handler &);
  };

  typedef Handler HType;
  typedef std::function<void(const drogon::ReqResult &, const drogon::HttpResponsePtr &)> FType;
  typedef std::function<void(const MyJson &)> RType;

  #define ADD(type) \
    template <typename T>\
    MyJson type(const std::string & path, const T & headers = {}, const std::string & post_data = "") { return sync_execute(drogon::HttpMethod::type, path, headers, post_data); }\
    template <typename T>\
    MyJson type(const FType & f, const std::string & path, const T & headers = {}, const std::string & post_data = "") { return async_execute(f, drogon::HttpMethod::type, path, headers, post_data); }\
    template <typename T>\
    MyJson type(const RType & f, const std::string & path, const T & headers = {}, const std::string & post_data = "") { return sync_execute(f, drogon::HttpMethod::type, path, headers, post_data); }\

  class HttpClient {
   public:
    HttpClient(const std::string &);
    virtual ~HttpClient();

    MyJson Get(const std::string &, const std::vector<std::pair<std::string, std::string>> & = {});
    MyJson Post(const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");
    MyJson Put(const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");
    MyJson Delete(const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");

    void Get(const FType &,    const std::string &, const std::vector<std::pair<std::string, std::string>> & = {});
    void Post(const FType &,   const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");
    void Put(const FType &,    const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");
    void Delete(const FType &, const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");

    void Get(const RType &,    const std::string &, const std::vector<std::pair<std::string, std::string>> & = {});
    void Post(const RType &,   const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");
    void Put(const RType &,    const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");
    void Delete(const RType &, const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");

    // void Start();

   private:
    void async_execute(const FType &, drogon::HttpMethod, const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");
    MyJson sync_execute(drogon::HttpMethod, const std::string &, const std::vector<std::pair<std::string, std::string>> & = {}, const std::string & = "");

    HType & GetHandler();

    std::vector<HType> handlers_;
    std::atomic<int32_t> handler_idx_ = 0;

    drogon::HttpClientPtr sync_client_;
    drogon::HttpRequestPtr sync_req_;

    std::thread hb_thread_;
    trantor::EventLoopThreadPool etp_;
    inline static std::function<void(const drogon::ReqResult &, const drogon::HttpResponsePtr &, const RType &, const std::string &)> ff_;
  };
}
