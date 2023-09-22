#pragma once

#include <bitset>
#include <vector>

#include "http/Method.h"
#include "util/Log.h"

class ConfigRoute {
 private:
  std::string root_;
  std::bitset<HTTP::TOTAL_METHODS> accepted_methods_;
  std::vector<std::string> index_files_;
  bool auto_index_{false};
  std::string redirect_;
  std::string updload_dir_;

 public:
  ConfigRoute() = default;
  ~ConfigRoute() = default;

  void setRoot(const std::string& root) {
    root_ = root;
  }
  void addAcceptedMethod(HTTP::Method method) {
    if (accepted_methods_.test(method)) {
      Log::warn("Method was already added to the list of allowed methods.\n");
    }
    accepted_methods_.set(method);
  }
  void addIndexFile(std::string&& file) {
    index_files_.push_back(file);
  }
  void setAutoIndex(bool val) {
    auto_index_ = val;
  }
  void setRedirect(const std::string& url) {
    redirect_ = url;
  }
  void setUploadDir(const std::string& path) {
    updload_dir_ = path;
  }

  [[nodiscard]] const std::string& getRoot() const {
    return root_;
  }
  [[nodiscard]] bool isMethodAllowed(HTTP::Method method) const {
    return accepted_methods_.none() || accepted_methods_.test(method);
  }
  [[nodiscard]] const std::vector<std::string>& getIndexFiles() const {
    return index_files_;
  }
  [[nodiscard]] bool isAutoIndex() const {
    return auto_index_;
  }
  [[nodiscard]] const std::string& getRedir() const {
    return redirect_;
  }
  [[nodiscard]] const std::string& getUploadDirPath() const {
    return updload_dir_;
  }
};
