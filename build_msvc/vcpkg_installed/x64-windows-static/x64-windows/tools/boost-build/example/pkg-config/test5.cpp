#include <string>
#include <iostream>
int main(int, char const** argv) {
  return QWERTY == std::string("UIOP") ? EXIT_SUCCESS : EXIT_FAILURE;
}
