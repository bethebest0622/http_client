#pragma once
#include "./curl_util.h"

namespace util::ding_util {
  class DingClient {
   public:
    DingClient();
    struct DingChannel {
      enum Enum : char {
        PROCESS_MONITOR = 'P',
        TRADE_REPORT = 'T',
        TRADE_ERROR = 'E',
        FATAL_ERROR = 'F',
        BT_REPORT = 'B',
        WITHDRAW = 'W',
        WEB_ORDER = 'O',
        PROCESS_MANAGER = 'M',
        TEST = 't',
        PNL_REPORT = 'R',
        PNL_TEST = 'S',
        HUGE_PNL = 'H',
        TINY_PNL = 'Y',
        BALANCE = 'L' 
      };  
      static std::string ToStr(Enum);
    };

    void send_raw(const std::string &, DingChannel::Enum = DingChannel::BT_REPORT); 
    void send_ding(const std::string &, const std::string &, DingChannel::Enum = DingChannel::BT_REPORT);
    std::string ding_str(const std::string &, const std::string &);
   private:
    util::curl_util::HttpClient curl_client_;
    inline static std::vector<std::pair<std::string, std::string>> header_ = {{"Content-Type", "application/json"}};
  };
  std::string translate_content(const std::string &); 
}
