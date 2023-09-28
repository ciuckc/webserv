#include "ErrorPage.h"

#include <fcntl.h>
#include "Status.h"
#include "io/task/SimpleBody.h"
#include "io/task/SendFile.h"

std::pair<Response, std::unique_ptr<OTask>> HTTP::defaultErrPage(int error) {
  const char* err_msg = HTTP::getStatus(error);
  // template is ~128 bytes, max status code length is ~32 byte, 128 + 2 * 32 + 2 * 3 < 256 QED
  auto body = std::make_unique<char[]>(256);
  size_t content_length = snprintf(body.get(), 256, errpage_template,
                                   error, err_msg, error, err_msg);
  return {std::forward<Response>(Response::builder()
                                     .message(error)
                                     .content_length(content_length)
                                     .header("Content-Type: text/html\r\n")
                                     .build()),
          std::make_unique<SimpleBody>(std::move(body), content_length)
  };
}

std::pair<Response, std::unique_ptr<OTask>> HTTP::createError(const ConfigServer &cfg, int error) {
  auto& error_pages = cfg.getErrorPages();
  auto page = error_pages.find(error);
  if (page == error_pages.end())
    return defaultErrPage(error);
  util::FileInfo file_info {};
  if (!file_info.open(page->second.c_str()) || !file_info.isFile()) {
    Log::warn("Invalid error page for code ", error, '\n');
    return defaultErrPage(error);
  }
  int fd = open(page->second.c_str(), O_RDONLY);
  if (fd == -1) {
    Log::warn("Failed to open error page\n");
    return defaultErrPage(error);
  }
  return {std::forward<Response>(Response::builder()
                                     .message(error)
                                     .content_length(file_info.size())
                                     .header("Content-Type: text/html\r\n")
                                     .build()),
          std::make_unique<SendFile>(fd, file_info.size())
  };
}
