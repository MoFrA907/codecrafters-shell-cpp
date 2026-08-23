#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  std::string input;
  while (true) {
    std::cout << "$ ";
    if (!std::getline(std::cin, input)) break;
    if ( input.starts_with("echo") && input.length() > 4) {
      std::cout<< input.substr(4);
      continue;
    }
    if (input == "exit") {break;}
    std::cout<<  input << ": command not found\n";
  }
  return 0;
}
