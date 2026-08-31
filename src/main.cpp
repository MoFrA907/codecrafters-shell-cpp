#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
std::vector<std::string> parse_input(const std::string &input) {
    std::vector<std::string> tokens;
    std::string current;       // the token currently being built
    char quote_type = '\0';
    bool has_content = false;
    bool escape_sequence = false;
    int saw_escape_char = 0;
    for (char c : input) {
        if (quote_type == '\0') {
            if ( escape_sequence ) {
                current+=c;
                escape_sequence = false;
                has_content = true;
                continue;
            }
            if ( c == '\\' ) {
                escape_sequence = true;

                continue;
            }
            if (c == '\'' or c == '"') {
                quote_type = c;
                has_content = true;
            } else if (c == ' ') {
                if (has_content) {
                    tokens.push_back(current);
                    current.clear();
                    has_content = false;
                }
                // else: skip extra spaces
            } else {

                current += c;
                has_content = true;
            }
        } else {
            // currently INSIDE a quote
            if ( quote_type == '\"') {
                if ( escape_sequence ) {
                    current+=c;
                    escape_sequence = false;
                    has_content = true;
                    continue;
                }
                if ( c == '\\' ) {
                    escape_sequence = true;
                    continue;
                }
            }
            if (c == quote_type) {          // this is the matching closer
                quote_type = '\0';
            } else {
                current += c;               // literal, even if it's a space or the other quote char
            }
        }
    }
    if (has_content) {
        tokens.push_back(current);
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

        std::vector<std::string> tokens = parse_input(input);
        if (tokens.empty())
            continue;   // empty input, just re-prompt

        std::string &cmd = tokens[0];

        if (cmd == "pwd")
        {
            retrieve_path();
        }
        else if (cmd == "cd")
        {
            if (tokens.size() < 2)
            {
                std::cerr << "cd: missing argument\n";
                continue;
            }

            std::string target = tokens[1];

            if (target == "~")
            {
                const char *home = std::getenv("HOME");
                if (home) target = home;
                else { std::cerr << "cd: HOME not set\n"; continue; }
            }

            if (!std::filesystem::exists(target) || !std::filesystem::is_directory(target))
                std::cout << "cd: " << target << ": No such file or directory\n";
            else
                std::filesystem::current_path(target);
        }
        else if (cmd == "echo")
        {
            std::string output;
            for (size_t i = 1; i < tokens.size(); i++)
            {
                if (i > 1) output += " ";
                output += tokens[i];
            }
            std::cout << output << "\n";
        }
        else if (cmd == "type")
        {
            if (tokens.size() < 2)
            {
                std::cerr << "type: missing argument\n";
                continue;
            }
            std::string s = tokens[1];

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
            std::string path = find_in_path(cmd);
            if (!path.empty())
                run_external(tokens, path);
            else
                std::cout << input << ": command not found\n";
        }
    }
    return 0;
}