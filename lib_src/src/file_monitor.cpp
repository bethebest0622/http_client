#include "../include/file_monitor.h"
#include "../include/ding_tools.h"

util::ding_util::MpFile::MpFile(const std::string & file_path) : file_path_(file_path) {
  lock_.l_type = F_WRLCK;
  lock_.l_whence = SEEK_SET;
  lock_.l_start = 0;
  lock_.l_len = 0;
  lock_.l_pid = getpid();

  fd_ = open(file_path.c_str(), O_CREAT | O_APPEND | O_RDWR);
  if (fd_ == -1) EE("MpFile open %s Failed: %s\n", file_path.c_str(), strerror(errno));
}

util::ding_util::MpFile::~MpFile() {
  close(fd_);
}

void util::ding_util::MpFile::mp_write(const std::string & title, const std::string & content, char c) {
  if (fcntl(fd_, F_SETLKW, &lock_) == -1) EE("fcntl start lock failed %s\n", file_path_.c_str());
  const std::string & s = std::format("ding\t{}\t{}\t{}\n", title, util::ding_util::translate_content(content), c);
  write(fd_, s.data(), s.size());
  lock_.l_type = F_UNLCK;
  if (fcntl(fd_, F_SETLKW, &lock_) == -1) EE("fcntl release lock failed %s\n", file_path_.c_str());
}

void util::ding_util::MpFile::mp_write(const char * s, size_t len) {
  if (fcntl(fd_, F_SETLKW, &lock_) == -1) EE("fcntl start lock failed %s\n", file_path_.c_str());
  write(fd_, s, len);
  lock_.l_type = F_UNLCK;
  if (fcntl(fd_, F_SETLKW, &lock_) == -1) EE("fcntl release lock failed %s\n", file_path_.c_str());
}

std::string util::ding_util::FileHandle::ToStr(Enum e) {
  if (e == Enum::DingNotify) return "DingNotify";
  else return "Unknown";
}

util::ding_util::FileMonitor::FileMonitor(const libconfig::Config* cfg) : cfg_(cfg) {
  START_LIBCONFIG;
  check_freq_ = cfg_->lookup("freq").operator unsigned int();
  const auto & file_monitor = cfg_->lookup("file_monitor");
  for (int i = 0; i < file_monitor.getLength(); ++i) {
    const auto & fm = file_monitor[i];
    const std::string & file_path = fm.lookup("file_path");
    const std::string & file_method = fm.lookup("file_method");
    if (file_method == "ding") {
      FileHandle fh;
      fh.last_mod_ = 0;
      fh.last_line_ = 0;
      fh.e_ = FileHandle::DingNotify;
      m_[file_path] = fh;
    } else {
      EE("FileMonitor Failed");
    }
  }
  END_LIBCONFIG("FileMonitor Check");
}

void util::ding_util::FileMonitor::handle_lines(util::ding_util::DingClient::DingChannel::Enum e, const std::vector<std::pair<std::string, std::string> > & lines) {
  // std::unordered_map<util::ding_util::DingClient::DingChannel::Enum, std::vector<std::string> > m;
  std::string s;
  for (const auto & [title, content] : lines) s += content + "\n\n";

  dclient_.send_ding("Group Message " + std::to_string(lines.size()), s, e);
}

void util::ding_util::FileMonitor::handle_line(util::ding_util::DingClient::DingChannel::Enum e, const std::string &title, const std::string& content) {
  dclient_.send_ding(title, content, e);
}

/*
bool util::ding_util::FileMonitor::handle_line(const std::string & line) {
  INFO("Handle Line = %s\n", line.c_str());
  if (line.empty()) return true;
  const auto & v = util::common_util::Split(line, "\t");
  if (v[0] == "ding") {
    if (v.size() != 4) {
      ERROR("DingLine should have 4 element\n");
      return false;
    }
    util::ding_util::DingClient::DingChannel::Enum e = util::ding_util::DingClient::DingChannel::Enum(v[3][0]);
    dclient_.send_ding(v[1], v[2], e);
  } else {
    EE("Unknown channel %s\n", v[0].c_str());
    return false;
  }
  return true;
}
*/

void util::ding_util::FileMonitor::check_one(const std::string & file_path, FileHandle * fh) {
  if (!util::file_util::FileExist(file_path)) {
    ERROR("%s not existed, no check\n", file_path.c_str());
    return;
  }
  struct stat buf;
  int ret = stat(file_path.c_str(), &buf);
  if (ret != 0) {  // file not existed
    ERROR("Stat %s Failed\n", file_path.c_str());
    return;
  }
  // INFO("modtime = %ld, last_mod = %zu\n", buf.st_mtime, fh->last_mod_);
  START_LIBCONFIG;
  if ((size_t)buf.st_mtime > fh->last_mod_) {
    const std::string & s = util::file_util::try_read(file_path);
    const auto & v = util::common_util::Split(s, "\n", true);
    if (v.size() < fh->last_line_) {  // TODO: should be incresed
      ERROR("%s Decrease %zu -> %zu\n", file_path.c_str(), fh->last_line_, v.size());
      return;
    }
    // INFO("last_line = %zu, vsize = %zu\n", fh->last_line_, v.size());
    if (fh->last_mod_ != 0) {  // not first round
      std::vector<std::string> vv(v.begin() + fh->last_line_, v.end());
      std::unordered_map<util::ding_util::DingClient::DingChannel::Enum, std::vector<std::pair<std::string, std::string>> > m;
      for (const std::string & line : vv) {
        const auto & vvv = util::common_util::Split(line, "\t");
        if (vvv[0] == "ding") {
          if (vvv.size() != 4) {
            ERROR("DingLine should have 4 element %zu\n", vvv.size());
            util::common_util::PrintVector(vvv);
            continue;
          }
          util::ding_util::DingClient::DingChannel::Enum e = util::ding_util::DingClient::DingChannel::Enum(vvv[3][0]);
          m[e].push_back(std::make_pair(vvv[1], vvv[2]));
        } else {
          EE("Unknown channel %s\n", vvv[0].c_str());
        }
      }

      for (const auto & [e, vvvv] : m) {
        if (vvvv.size() > 3) {
          handle_lines(e, vvvv);
        } else {
          for (size_t i = 0; i < vvvv.size(); ++i) {
            const auto & [title, content] = vvvv[i];
            handle_line(e, title, content);
            // if (!handle_line(vv[i])) EE("%s BadLine %zu Failed: %s\n", file_path.c_str(), fh->last_line_ + i, vv[i].c_str());
          }
        }
      }
    }
    fh->last_mod_ = buf.st_mtime;
    fh->last_line_ = v.size();
  }
  END_LIBCONFIG("check_one");
}

void util::ding_util::FileMonitor::Start() {
  while (1) {
    for (auto & [file_path, fh] : m_) check_one(file_path, &fh);
    sleep(check_freq_);
  }
}
