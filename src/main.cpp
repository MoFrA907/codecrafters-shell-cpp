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
      std::cin>>input;
      if ( input.starts_with("echo")) {
        std::cout<< input.substr(5);
      }if (input == "exit") {break;}
      std::cout<<  input << ": command not found\n";

    }
}
