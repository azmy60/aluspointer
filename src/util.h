#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <iomanip> // std::hex
#include <sstream>

template<typename T>
std::string int_to_hex(T i)
{
  std::stringstream stream;
  stream << std::setfill ('0') << std::setw(sizeof(T)*2) 
          << std::hex << i;
  return stream.str();
}

#endif // UTIL_H
