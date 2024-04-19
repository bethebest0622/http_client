#pragma once
#include "./ding_tools.h"

namespace util::process {
  class ProcessMonitor {
   public:
    ProcessMonitor(const libconfig::Config&);
    virtual ~ProcessMonitor() = default;
    void refresh();
    bool check_exist(const std::string &); 
    void print() const;
    void Run();
   protected:
   private:
    const libconfig::Config & cfg_;
    std::unordered_set<std::string> pm_;
    int32_t freq_;
    int32_t hb_cnt_;
    std::vector<std::string> check_list_;
    util::ding_util::DingClient dclient_;
  };  
}
