#include "../ds/string.hpp"
#include <iostream>

template <typename... Args> void print(Args &&...args) {
  ((std::cout << ::toString(args) << " "), ...);
  std::cout << std::endl;
}

inline void system($String command) { std::system(command->_str().c_str()); }