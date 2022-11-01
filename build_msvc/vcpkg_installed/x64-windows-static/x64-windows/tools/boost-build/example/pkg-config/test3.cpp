#include <string>
#include <iostream>
int main(int, char const** argv) {
  return VARIANT == std::string(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
