#include "../ds/string.hpp"
#include <iostream>

template <typename... Args> void print(Args &&...args) {
  ((std::cout << ::toString(args) << " "), ...);
  std::cout << std::endl;
}