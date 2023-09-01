#pragma once

#include <memory>
#include "Response.h"
#include "Status.h"

namespace http {

static constexpr const char* errpage_template = "<!DOCTYPE html><html><head><title>%d %s</title></head>"
                              "<body><h1>%d %s</h1><p>(Google what it means yourself)</p></body></html>";

static std::pair<Response, std::unique_ptr<char[]>> defaultErrPage(int error) {
  const char* err_msg = http::getStatus(error);
  // template is ~128 bytes, max status code length is ~32 byte, 128 + 2 * 32 + 2 * 3 < 256 QED
  auto body = std::make_unique<char[]>(256);
  size_t content_length = snprintf(body.get(), 256, errpage_template,
                                   error, err_msg, error, err_msg);
  return {std::forward<Response>(Response::builder()
              .message(error)
              .content_length(content_length)
              .header("Content-Type", "text/html")
              .build()),
          std::move(body)
  };
}
}