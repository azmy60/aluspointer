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

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);

std::string base64_decode(std::string const& encoded_string);

#endif // UTIL_H
