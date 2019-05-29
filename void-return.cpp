#include <iostream>

void func() {
  std::cout << "hello world!" << std::endl;
}

void func2() {
  return func(); // permitted
}

void func3(char *memory) {
  return delete[] memory; // permitted
}

int main(int argc, char **argv) {

  func2();

  char* memory = new char[3];
  func3(memory);
  return 0;
}
