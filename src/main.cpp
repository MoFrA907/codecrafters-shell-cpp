#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
std::vector<std::string> tokenize(const std::string &input) {
  std::vector<std::string> tokens;
  std::stringstream iss(input);
  std::string word;
  while ( iss >> word ) {
      tokens.push_back(word);
  }
  return tokens;
}
std::string find_in_path(const std::string &name)
{
  const char *path_var = std::getenv("PATH");
  if (!path_var)
    return "";

  std::istringstream path_stream(path_var);
  std::string dir;

  while (std::getline(path_stream, dir, ':'))
  {
    std::string file_path = dir + "/" + name;
    if (access(file_path.c_str(), F_OK) == 0 &&
        access(file_path.c_str(), X_OK) == 0)
    {
      return file_path;
    }
  }
  return "";
}
void retrieve_path() {
    std::cout<<std::filesystem::current_path().string()<<std::endl;
}
void run_external(const std::vector<std::string> &tokens, const std::string &full_path) {
    pid_t pid = fork();
    if ( pid < 0 ) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        std::vector<char*> args;
        for (const auto &token : tokens) {
          args.push_back(const_cast<char*>(token.c_str()));
        }
        args.push_back(nullptr);

        execvp(full_path.c_str() ,args.data());
        std::cerr << tokens[0] << ": command not found\n";
        std::exit(1);
    }
      int status;
      waitpid(pid,&status,0);
  }

int main()
{
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::string input;

    while (true)
    {
        std::cout << "$ ";
        if (!std::getline(std::cin, input))
            break;

        if (input == "exit")
            break;

        if (input == "pwd")
        {
            retrieve_path();
            continue;
        }

        if (input.starts_with("cd"))
        {
            std::vector<std::string> tokens = tokenize(input);

            if (tokens.size() < 2)
            {
                std::cerr << "cd: missing argument\n";
                continue;
            }

            std::string target = tokens[1];

            if (target == "~")
            {
                const char *home = std::getenv("HOME");
                if (home)
                    target = home;
                else
                {
                    std::cerr << "cd: HOME not set\n";
                    continue;
                }
            }

            if (!std::filesystem::exists(target) || !std::filesystem::is_directory(target))
            {
                std::cout << "cd: " << target << ": No such file or directory\n";
            }
            else
            {
                std::filesystem::current_path(target);
            }
            continue;   // <-- makes sure we don't fall through to echo/type/external below
        }

        if (input.starts_with("echo") && input.length() > 4)
        {
            std::cout << input.substr(5) << std::endl;
        }
        else if (input.starts_with("type"))
        {
            std::string s = input.substr(5);

            if (s == "echo" || s == "exit" || s == "type" || s == "pwd" || s == "cd")
            {
                std::cout << s << " is a shell builtin" << std::endl;
            }
            else
            {
                std::string path = find_in_path(s);
                if (!path.empty())
                    std::cout << s << " is " << path << "\n";
                else
                    std::cout << s << ": not found" << std::endl;
            }
        }
        else
        {
            std::vector<std::string> tokens = tokenize(input);
            if (tokens.empty())
                continue;

            std::string path = find_in_path(tokens[0]);
            if (!path.empty())
                run_external(tokens, path);
            else
                std::cout << input << ": command not found\n";
        }
    }
    return 0;
}