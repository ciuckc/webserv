#include "SendFile.h"

bool SendFile::operator()(Connection& connection) {
  while (!connection.getBuffer().needWrite())
    if (connection.getBuffer().readFrom(fd_))
      return true;

  return false;
}
void SendFile::onDone(Connection&) {
  Log::trace("SendFile done\n");
}
