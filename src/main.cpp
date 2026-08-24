#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <unistd.h>
int main(int argc , char * argv[]) {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
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
        }
        else
        {
          const char *path_var = std::getenv("PATH");
          if (!path_var)
          {
            std::cout << s << ": not found\n";
            continue;
          }

          std::istringstream path_stream(path_var);
          std::string dir;
          bool found = false;

          while (std::getline(path_stream, dir, ':'))
          {
            std::string file_path = dir + "/" + s;

            if (access(file_path.c_str(), F_OK) == 0 &&
                access(file_path.c_str(), X_OK) == 0)
            {
              std::cout << s << " is " << file_path << "\n";
              found = true;
              break;
            }
          }

          if (!found)
            std::cout << s << ": not found" << std::endl;
        }
        }
        else
        {
          std::cout << input << ": command not found\n";
        }
    }
    return 0;
  }