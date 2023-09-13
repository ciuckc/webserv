#include "/Users/mbatstra/.brew/Cellar/criterion/2.4.1_1/include/criterion/criterion.h"
#include "../srcs/http/RequestHandler.h"
#include <fcntl.h>
#include <iostream>

Test(RequestHandler, get_root) {
  cr_assert(true, "Hello Test\n");
}

Test(RequestHandler, get_index) {
  RequestHandler rh;
  int fd = open("request/get_index", O_RDONLY);
  if (fd < 0) {
    std::runtime_error("can't open file");
  }
  rh.readRequest(fd);
  rh.execRequest(); 
  std::cout << rh.getResponse() << std::endl;
}
