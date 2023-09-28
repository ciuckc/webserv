#pragma once

#include <memory>

#include "Response.h"
#include "config/ConfigServer.h"
#include "io/task/IOTask.h"

namespace HTTP {

static constexpr const char* errpage_template = "<!DOCTYPE html><html><head><title>%d %s</title></head>"
                              "<body><h1>%d %s</h1><p>(Google what it means yourself)</p></body></html>";

std::pair<Response, std::unique_ptr<OTask>> defaultErrPage(int error);
std::pair<Response, std::unique_ptr<OTask>> createError(const ConfigServer& cfg, int error);
}
