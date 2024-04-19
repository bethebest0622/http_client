#include "../include/process_monitor.h"

util::process::ProcessMonitor::ProcessMonitor(const libconfig::Config & cfg) : cfg_(cfg) {
  START_LIBCONFIG;
  freq_ = cfg_.lookup("freq");
  hb_cnt_ = cfg_.exists("hb_cnt") ? cfg_.lookup("hb_cnt") : -1;
  if (freq_ <= 0) EE("Error Freq %d\n", freq_);
  const auto & processes = cfg_.lookup("processes");
  for (int i = 0; i < processes.getLength(); ++i) {
    const std::string & pattern = processes[i];
    check_list_.push_back(pattern);
  }
  END_LIBCONFIG("ProcessMonitor");
}

void util::process::ProcessMonitor::refresh() {
  pm_.clear();
  pm_ = util::file_util::GetProcessList();
}

void util::process::ProcessMonitor::Run() {
  size_t cnt = 0;
  while (true) {
    refresh();
    std::string s;
    for (const std::string & p : check_list_) {
      if (!check_exist(p)) s += p + "\n\n";
    }
    if (!s.empty()) {
      std::string header = util::common_util::GetHostname() + " ProcessMonitor Check\n";
      dclient_.send_ding(header, header + s, util::ding_util::DingClient::DingChannel::PROCESS_MONITOR);
    }
    sleep(freq_);
    if (hb_cnt_ > 0 && cnt++ % hb_cnt_ == 0)  {
      std::string header = util::common_util::GetHostname() + " ProcessMonitor HeartBeating\n";
      dclient_.send_ding(header, header, util::ding_util::DingClient::DingChannel::PROCESS_MONITOR);
    }
  }
}

bool util::process::ProcessMonitor::check_exist(const std::string & pattern) {
  return pm_.find(pattern) != pm_.end();
}

void util::process::ProcessMonitor::print() const {
  util::common_util::PrintVector(pm_, "List Process");
}
