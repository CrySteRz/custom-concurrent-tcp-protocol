#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/sysinfo.h>
#include <cstring>

class Authenticate {
public:
  // TODO: Vezi de ce nu merge bine
  static bool login(char *username, char *password) {
    if (strcmp(username, "admin") == 0 && strcmp(password, "admin") == 0) {
      return true;
    }
    return false;
};
};