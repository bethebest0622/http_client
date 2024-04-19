#pragma once
#include "./ding_tools.h"

namespace util::ding_util {
  class MpFile {
   public:
    MpFile(const std::string &); 
    virtual ~MpFile();
    void mp_write(const char *, size_t);
    void mp_write(const std::string &, const std::string &, char);
   private:
    int fd_;
    std::string file_path_;
    struct flock lock_;
  };  

  struct FileHandle {
    enum Enum : char {
      DingNotify = 'D',
    };  
    Enum e_ = Enum::DingNotify;
    size_t last_mod_ = 0;
    size_t last_line_ = 0;
    static std::string ToStr(Enum e); 
  };  

  class FileMonitor {
   public:
    FileMonitor(const libconfig::Config*);
    void Start();
   private:
    void check_one(const std::string &, FileHandle*);
    // virtual bool handle_line(const std::string &); 
    virtual void handle_line(util::ding_util::DingClient::DingChannel::Enum, const std::string &, const std::string&);
    virtual void handle_lines(util::ding_util::DingClient::DingChannel::Enum, const std::vector<std::pair<std::string, std::string>> &); 
    const libconfig::Config* cfg_;
    size_t check_freq_;
    std::unordered_map<std::string, FileHandle> m_; 
    util::ding_util::DingClient dclient_;
  };
}
