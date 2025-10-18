#pragma once

#include "../ds/string.hpp"
#include "../rt/AutoRef.hpp"
#include "../rt/Object.hpp"
#include <fstream>
#include <ios>
#include <string>

class File;
using $File = AutoRef<File>;
class File : virtual public Object {
  $String filename;
  std::fstream stream;

public:
  File(const $String &fname, const $String &mode) : filename(fname) {
    std::ios_base::openmode m = std::ios_base::in;
    if (*mode == newString("r"))
      m = std::ios_base::in;
    else if (*mode == newString("w"))
      m = std::ios_base::out | std::ios_base::trunc;
    else if (*mode == newString("a"))
      m = std::ios_base::out | std::ios_base::app;
    else if (*mode == newString("r+"))
      m = std::ios_base::in | std::ios_base::out;

    stream.open(filename->_str(), m);
  }
  ~File() { stream.close(); }

  $String read() noexcept {
    std::string content, line;
    while (std::getline(stream, line))
      content += line + "\n";
    return newString(content);
  }

  $String readLine() noexcept {
    std::string line;
    std::getline(stream, line);
    return newString(line);
  }

  void write(const $String &str) noexcept { stream << str->_str(); }
};

inline $File open(const $String &fname, const $String &mode) { return $File::make(fname, mode); }