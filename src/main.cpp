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
    std::getline(std::cin, input);
    if (input == "exit")
      break;
    if ( input.starts_with("echo") && input.length() > 4) {
      std::cout<< input.substr(5)<<std::endl;
    } else if (input.starts_with("type")) {
      std::string s = input.substr(5);
        if (s == "echo" || s == "exit" || s == "type") {
          std::cout<< s <<" is a shell builtin"<<std::endl;
        } else {
          std::cout<< s <<": not found"<<std::endl;
        }
    }
    else {
      std::cout<<  input << ": command not found\n";
    }
  }
  return 0;
}
