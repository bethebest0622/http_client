#include <rapidjson/error/en.h>
#include "../include/ding_tools.h"

std::string get_sign(const std::string & secret) {
  timeval t;
  gettimeofday(&t, NULL);
  size_t ts = t.tv_sec * 1000 + t.tv_usec / 1000;
  std::string s = std::to_string(ts) + "\n" + secret;
  return "&timestamp=" + std::to_string(ts) + "&sign=" + util::curl_util::url_transform(util::curl_util::hmac_sha256(secret, s, true));
}

std::string util::ding_util::translate_content(const std::string & s) { return util::common_util::replace(util::common_util::replace(s, "\n", "\\n"), "\"", ""); }

std::string util::ding_util::DingClient::DingChannel::ToStr(Enum e) {
  if (e == Enum::BT_REPORT) return "BT_REPORT";
  else if (e == Enum::PROCESS_MONITOR) return "PROCESS_MINITOR";
  else if (e == Enum::TRADE_ERROR) return "TRADE_ERROR";
  else if (e == Enum::FATAL_ERROR) return "FATAL_ERROR";
  else if (e == Enum::TRADE_REPORT) return "TRADE_REPORT";
  else if (e == Enum::WITHDRAW) return "WITHDRAW";
  else if (e == Enum::WEB_ORDER) return "WEB_ORDER";
  else if (e == Enum::PROCESS_MANAGER) return "PROCESS_MANAGER";
  else if (e == Enum::PNL_REPORT) return "PNL_REPORT";
  else if (e == Enum::TEST) return "TEST";
  else if (e == Enum::PNL_TEST) return "PNL_TEST";
  else if (e == Enum::HUGE_PNL) return "HUGE_PNL";
  else if (e == Enum::TINY_PNL) return "TINY_PNL";
  else if (e == Enum::BALANCE) return "BALANCE";
  else return "UNKNOWN_CHANNEL";
}

// https://oapi.dingtalk.com/robot/send?access_token=86ce1488f958b919169117d9af6848812994832d0955f4fc28673d0591b42f6a
static const std::unordered_map<util::ding_util::DingClient::DingChannel::Enum, std::pair<std::string, std::string>> ding_map_ = {
{util::ding_util::DingClient::DingChannel::PROCESS_MONITOR, {"13fcc99bec42d9a103911431aa3028c85bf8af20c171f7100bc19c4ac1caeaaa", "SECf56594d9c4df1a66e1156b657cdeea05e66c48540e8515f6c536ce019a4311fc"}},
{util::ding_util::DingClient::DingChannel::TRADE_ERROR,     {"78d6785919b6967c3693464fbc8e22ee99631e59045985f463b67c879dad0da5", "SEC43c07b2ff60283f4d192d4c7bf9ee288244c12b9897f7c61f79066ac5683f59b"}},
// {util::ding_util::DingClient::DingChannel::TRADE_ERROR,     {"dingamovfekpvgfngfk5", "uqcQ6lVNWVjDfwMVsm0vxb4xtXKzsO7WWJ8WOwp5HWkt9GgnSgTbUxm61roCK1u2"}},
{util::ding_util::DingClient::DingChannel::FATAL_ERROR,     {"414a096efe7c30a559428df0aac4e02bc15512f6285cf5a69a30f1cd6d7ef418", "SECa24d7140dcae32d65d835cde9212ff29a4a243bd8165cf308bc3143263cc8ea1"}},
{util::ding_util::DingClient::DingChannel::TRADE_REPORT,    {"86bf4642c92dacf5b203f65d46ec339b74fc0acf02c79220a995d2fbcee65919", "SEC3dfbf29dd7c124fa5739988f94015e07e5eab3648612a36b1fb198f31332356b"}},
{util::ding_util::DingClient::DingChannel::BT_REPORT,       {"b58a94d1997ab632035a14cc77386c666253c88bc13df641bab903beeeb69794", "SEC3cbd55b79a8478c676797d2d74cd0a5f544d959b6bcf9d828eb01e83241c4538"}},
{util::ding_util::DingClient::DingChannel::WITHDRAW,        {"1ecdf449b7c30cada18c5227d9359b9a8aeed99e7b14b975c1a49e98499eaa27", "SEC5ff82ba1817fa2f6d6dd16db7a9b5852406afd4cff06f7209e4b75ef77b506f5"}},
{util::ding_util::DingClient::DingChannel::WEB_ORDER,       {"79e027b45135b071878f14f01bece98ad4fc379a75561550e2c16f2e63be4910", "SECa5bb338fd7dc936445c66a0ebc68bcccea195cd72cc4043e5f189f8365338d64"}},
{util::ding_util::DingClient::DingChannel::PROCESS_MANAGER, {"4c0d5bfc10c0ca0e1456551863555e00bc8eea6dc0c7ff0cad684c9436bb25bb", "SECb52902e8ef4012dc818e550ace8345ff2b9514d071be8fa968c24ae8f33b5c4d"}},
{util::ding_util::DingClient::DingChannel::TEST,            {"dc4acfb1396082c2526c8d706daf02aea1073c0ed6cf53d62afb997f502be3ac", "SEC4dd860edb00c15b654b59dc6af0c6ecaa418db53239608eb213fd9e7e7aba495"}},
{util::ding_util::DingClient::DingChannel::PNL_REPORT,      {"afdc89129b199f8c60a92e13aeee67043031e0dc442a27f2b6f0f98debb09b09", "SECd90e4aca6465274323e6bd5da82b94b31806b38ca7ccaf0c8b71c51252db9ba7"}},
{util::ding_util::DingClient::DingChannel::PNL_TEST,        {"451e83070f9d844cbed76aef90f3d335345fee0b958f5e99a87ee088b67f44e2", "SEC74d91fb259a146add4c21c3600791a33615d780ec76f3283e2f62d218901537a"}},
{util::ding_util::DingClient::DingChannel::HUGE_PNL,        {"939e8cca2c7e908a013de6c33e4b77fbe7f6c1648bc93ee51d99958dd66ef438", "SEC84a8fcebd8ffb2d4a428d7fdf42f744c19f574021e54230f368c9c2132b7389d"}},
{util::ding_util::DingClient::DingChannel::TINY_PNL,        {"92bc5748d6365555b3984792c149fda01df18315005e0d44d43e0cc45a3d16aa", "SECfe09cfa57fe260dad9006861a8544ba3f76f236a3c885d6cbb9139eaf0bcc06f"}},
{util::ding_util::DingClient::DingChannel::BALANCE,         {"708c4897410aa2776ee2c563715397011b8be848b29e929b1154b348f37f6c22", "SECa18ed50ed323c06f4c4189264547e2f50f4a61ca748a50fab1b51d83a03c065a"}},
};

util::ding_util::DingClient::DingClient() : curl_client_("https://oapi.dingtalk.com") {
}

void util::ding_util::DingClient::send_raw(const std::string & data, DingChannel::Enum e) {
  const auto & it = ding_map_.find(e);
  if (it == ding_map_.end()) EE("send_raw meet unknown channel %s\n", DingChannel::ToStr(e).c_str());
  const auto & [token, secret] = it->second;
  std::string path = "/robot/send?access_token=" + token + get_sign(secret);
  const auto & f = [path, data] (const drogon::ReqResult & r, const drogon::HttpResponsePtr & h) {
    if (r != drogon::ReqResult::Ok) {
      std::cout << "error while sending request to server! result: " << r << std::endl;
      return;
    }
    rapidjson::Document d;
    if (d.Parse(h->getBody().data()).HasParseError()) ERROR("path:%s, parse:%s, error:%s\n", path.c_str(), h->getBody().data(), rapidjson::GetParseError_En(d.GetParseError()));
    int code = d["errcode"].GetInt();
    if (code != 0) {
      // if (code == 130101) {}  // TODO: send too fast, exceed 20 times per minute
      util::curl_util::Show(d, path + " data = " + data);
    }
  };
  curl_client_.Post(f, path, header_, data);
  // const auto & d = curl_client_.Post(u, {}, data);
  // int code = d["errcode"].GetInt();
  // if (code != 0) {
  //   // if (code == 130101) {}  // TODO: send too fast, exceed 20 times per minute
  //   util::curl_util::Show(d, u + " data = " + data);
  // }
}

std::string util::ding_util::DingClient::ding_str(const std::string & title, const std::string & text) {
  if (text.empty()) return "";
  return "{\"msgtype\": \"markdown\",\"markdown\": {\"title\":\"" + title + "\", \"text\":\"" + util::ding_util::translate_content(text) + "\"}}";
}

void util::ding_util::DingClient::send_ding(const std::string & title, const std::string & text, DingChannel::Enum e) {
  const std::string & s = ding_str(title, text);
  if (!s.empty()) send_raw(s, e);
}
