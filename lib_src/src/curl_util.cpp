#include <openssl/hmac.h>
#include "../include/curl_util.h"

#define CURL_SIZE 16

util::curl_util::Handler::Handler(const util::curl_util::Handler & cd) {
  if (this != &cd) {
    ptr_ = cd.ptr_;
    req_ptr_ = cd.req_ptr_;
  }
}

util::curl_util::Handler& util::curl_util::Handler::operator=(const util::curl_util::Handler & cd) {
  if (this != &cd) {
    ptr_ = cd.ptr_;
    req_ptr_ = cd.req_ptr_;
  }
  return *this;
}

util::curl_util::Handler::Handler(const std::string & host, trantor::EventLoop * loop) {
  // INFO("handler host = %s\n", host.c_str());
  ptr_ = drogon::HttpClient::newHttpClient(host, loop);
  ptr_->setSockOptCallback([](int fd) {
    int optval = 10;
    ::setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &optval, static_cast<socklen_t>(sizeof optval));
    ::setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &optval, static_cast<socklen_t>(sizeof optval));
    ::setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &optval, static_cast<socklen_t>(sizeof optval));
  });

  req_ptr_ = drogon::HttpRequest::newHttpRequest();
}

util::curl_util::HttpClient::HttpClient(const std::string & host) : etp_(CURL_SIZE) {
  handlers_.reserve(CURL_SIZE);
  for (size_t i = 0; i < CURL_SIZE; ++i) handlers_.emplace_back(Handler(host, etp_.getNextLoop()));

  sync_client_ = drogon::HttpClient::newHttpClient(host);
  sync_req_ = drogon::HttpRequest::newHttpRequest();

  ff_ = [] (const drogon::ReqResult & r, const drogon::HttpResponsePtr & h, const RType & rf, const std::string & path) {
    if (r != drogon::ReqResult::Ok) {
      std::cout << "error while sending request to server! result: " << r << std::endl;
      return;
    }
    MyJson d;
    if (d.Parse(h->getBody().data()).HasParseError()) ERROR("path:%s, parse:%s, error:%s\n", path.c_str(), h->getBody().data(), rapidjson::GetParseError_En(d.GetParseError()));
    rf(d);
  };
}

void util::curl_util::HttpClient::async_execute(const FType & f, drogon::HttpMethod type, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string & body) {
  auto & h = GetHandler();
  std::lock_guard<std::mutex> lock(h.mut_);
  h.req_ptr_->setMethod(type);
  h.req_ptr_->setPath(path);
  if (type != drogon::HttpMethod::Get) {
    if (!body.empty()) h.req_ptr_->setBody(body);
  }
  for (const auto & [key, value] : extra_http_header) {
    h.req_ptr_->addHeader(key, value);
  }

  h.req_ptr_->setContentTypeString("application/json");

  // INFO("host = %s, body = %s, path = %s\n", h.ptr_->getHost().c_str(), h.req_ptr_->bodyData(), path.c_str());
  // util::common_util::PrintMap(h.req_ptr_->getHeaders(), "header");
  timeval t;
  gettimeofday(&t, NULL);
  h.ptr_->sendRequest(h.req_ptr_, [f, t] (const drogon::ReqResult & rq, const drogon::HttpResponsePtr & hr) {
    timeval t1;
    gettimeofday(&t1, NULL);
    f(rq, hr);
    int lat = (t1.tv_sec - t.tv_sec) * 1000000 + t1.tv_usec - t.tv_usec;
    printf("async latency: %ld %ld %ld %ld %d\n", t.tv_sec, t.tv_usec, t1.tv_sec, t1.tv_usec, lat);
  });
  etp_.start();
}

util::curl_util::HttpClient::~HttpClient() {}

util::curl_util::HType & util::curl_util::HttpClient::GetHandler() {
  return handlers_.at(handler_idx_.fetch_add(1) % CURL_SIZE);
}

util::curl_util::MyJson util::curl_util::HttpClient::Get(const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header) {
  return sync_execute(drogon::HttpMethod::Get, path, extra_http_header);
}

util::curl_util::MyJson util::curl_util::HttpClient::Post(const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string &post_data) {
  return sync_execute(drogon::HttpMethod::Post, path, extra_http_header, post_data);
}

util::curl_util::MyJson util::curl_util::HttpClient::Put(const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string &post_data) {
  return sync_execute(drogon::HttpMethod::Put, path, extra_http_header, post_data);
}

util::curl_util::MyJson util::curl_util::HttpClient::Delete(const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string &post_data) {
  return sync_execute(drogon::HttpMethod::Delete, path, extra_http_header, post_data);
}

void util::curl_util::HttpClient::Get(const RType & f, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header) {
  return async_execute(std::bind(ff_, std::placeholders::_1, std::placeholders::_2, f, path), drogon::HttpMethod::Get, path, extra_http_header, "");
}

void util::curl_util::HttpClient::Post(const RType & f, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string &post_data) {
  return async_execute(std::bind(ff_, std::placeholders::_1, std::placeholders::_2, f, path), drogon::HttpMethod::Post, path, extra_http_header, post_data);
}

void util::curl_util::HttpClient::Delete(const RType & f, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string &post_data) {
  return async_execute(std::bind(ff_, std::placeholders::_1, std::placeholders::_2, f, path), drogon::HttpMethod::Delete, path, extra_http_header, post_data);
}

void util::curl_util::HttpClient::Put(const RType & f, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string &post_data) {
  return async_execute(std::bind(ff_, std::placeholders::_1, std::placeholders::_2, f, path), drogon::HttpMethod::Put, path, extra_http_header, post_data);
}

void util::curl_util::HttpClient::Get(const FType & f, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header) {
  return async_execute(f, drogon::HttpMethod::Get, path, extra_http_header, "");
}

void util::curl_util::HttpClient::Post(const FType & f, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string & body) {
  return async_execute(f, drogon::HttpMethod::Post, path, extra_http_header, body);
}

void util::curl_util::HttpClient::Delete(const FType & f, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string & body) {
  return async_execute(f, drogon::HttpMethod::Delete, path, extra_http_header, body);
}

void util::curl_util::HttpClient::Put(const FType & f, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string & body) {
  return async_execute(f, drogon::HttpMethod::Put, path, extra_http_header, body);
}

util::curl_util::MyJson util::curl_util::HttpClient::sync_execute(drogon::HttpMethod type, const std::string & path, const std::vector<std::pair<std::string, std::string>> &extra_http_header, const std::string & body) {
  sync_req_->setMethod(type);
  sync_req_->setPath(path);
  if (type != drogon::HttpMethod::Get) {
    if (!body.empty()) sync_req_->setBody(body);
  }
  for (const auto & [key, value] : extra_http_header) sync_req_->addHeader(key, value);

  sync_req_->setContentTypeString("application/json");
  // INFO("host = %s, body = %s, path = %s\n", sync_client_->getHost().c_str(), sync_req_->bodyData(), path.c_str());
  // util::common_util::PrintMap(sync_req_->getHeaders(), "header");
  const auto & [req_result, reponse_ptr] = sync_client_->sendRequest(sync_req_);
  if (req_result != drogon::ReqResult::Ok) EE("error while sending request to server! result: %d\n", int(req_result));
  MyJson d;
  if (d.Parse(reponse_ptr->getBody().data()).HasParseError()) ERROR("path:%s, parse:%s, error:%s\n", path.c_str(), reponse_ptr->getBody().data(), rapidjson::GetParseError_En(d.GetParseError()));
  return d;
}

static const char base64_chars[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZ""abcdefghijklmnopqrstuvwxyz""0123456789+/";
inline std::string base64_encode(unsigned char const * input, size_t len) {
  std::string ret;
  ret.reserve(len);
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (len--) {
    char_array_3[i++] = *(input++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;
      for (i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) char_array_3[j] = '\0';
    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;
    for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
    while (i++ < 3) ret += '=';
  }

  return ret;
}

static const char HexCodes[] = "0123456789abcdef";
std::string b2a_hex(const unsigned char *byte_arr, int n) {
  std::string HexString;
  HexString.reserve(n);
  for (int i = 0; i < n; ++i) {
    unsigned char BinValue = byte_arr[i];
    HexString += HexCodes[(BinValue >> 4) & 0x0F];
    HexString += HexCodes[BinValue & 0x0F];
  }
  return HexString;
}

std::string util::curl_util::hmac_sha256(const std::string & key, const std::string & qstr, bool is_64) {
  unsigned char* digest = HMAC(EVP_sha256(), key.data(), key.size(), (unsigned char*)qstr.data(), qstr.size(), NULL, NULL);
  if (is_64) return base64_encode(digest, 32);
  else return b2a_hex(digest, 32);
}

std::string util::curl_util::hmac_sha512(const std::string & key, const std::string & qstr, bool is_64) {
  unsigned int len;
  unsigned char* digest = HMAC(EVP_sha512(), key.data(), key.size(), (unsigned char*)qstr.data(), qstr.size(), NULL, &len);
  if (is_64) return base64_encode(digest, len);
  else return b2a_hex(digest, len);  // , 64);  // TODO: this is very tricky, 64 means the buff size
}

std::string util::curl_util::url_transform(const std::string & s) {
  std::string r;
  r.reserve(s.size());
  for (char i : s) {
    switch (i) {
      case '+': { r += "%2B"; break; }
      case ' ': { r += "%20"; break; }
      case '/': { r += "%2F"; break; }
      case '?': { r += "%3F"; break; }
      case '%': { r += "%25"; break; }
      case '#': { r += "%23"; break; }
      case '&': { r += "%26"; break; }
      case '=': { r += "%3D"; break; }
      default: { r += i; break; }
    }
  }
  return r;
}

void util::curl_util::Show(const rapidjson::Document& doc, const std::string& topic) {
  printf("================================%s================================\n", topic.c_str());
  std::cout << ToStr(doc) << std::endl;
}

void util::curl_util::Show(const rapidjson::Value & val, const std::string& topic) {
  printf("================================%s================================\n", topic.c_str());
  std::cout << ToStr(val) << std::endl;
}

std::string util::curl_util::ToStr(const rapidjson::Document& doc, const std::string & topic) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return topic.empty() ? buffer.GetString() : topic + ":" + buffer.GetString();
}

std::string util::curl_util::ToStr(const rapidjson::Value & doc, const std::string & topic) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return topic.empty() ? buffer.GetString() : topic + ":" + buffer.GetString();
}

inline size_t curl_cb(char *content, size_t size, size_t nmemb, std::string* ptr) {
  ptr->append(content, size * nmemb);
  return size * nmemb;
}
