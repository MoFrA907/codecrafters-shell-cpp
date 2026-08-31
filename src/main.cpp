#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <filesystem>

struct Job {
    int job_number;
    pid_t pid;
    std::string command;
};
std::vector<Job> jobs_list;
int next_job_number = 1;

std::vector<std::string> parse_input(const std::string &input) {
    std::vector<std::string> tokens;
    std::string current;       // the token currently being built
    char quote_type = '\0';
    bool has_content = false;
    bool escape_sequence = false;
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

// A single redirect: which fd to point (1 = stdout, 2 = stderr), to which file,
// and whether to append instead of truncate.
struct Redirect {
    int target_fd;
    std::string file;
    bool append;
};

// Scans tokens for ">", "1>", "2>", ">>", "1>>", "2>>" and strips them (plus their
// filename) out of tokens. Returns all redirects found, in the order they appeared.
// On syntax error, clears tokens and returns whatever was parsed so far (caller
// should bail on empty tokens).
std::vector<Redirect> extract_redirects(std::vector<std::string> &tokens) {
    std::vector<Redirect> redirects;

    for (size_t i = 0; i < tokens.size(); /* no auto-increment */) {
        int target_fd = -1;
        bool append = false;
        if (tokens[i] == ">" || tokens[i] == "1>") {
            target_fd = 1;
        } else if (tokens[i] == "2>") {
            target_fd = 2;
        } else if (tokens[i] == ">>" || tokens[i] == "1>>") {
            target_fd = 1;
            append = true;
        } else if (tokens[i] == "2>>") {
            target_fd = 2;
            append = true;
        }

        if (target_fd == -1) {
            ++i;
            continue;
        }

        if (i + 1 >= tokens.size()) {
            std::cerr << "syntax error: expected filename after '" << tokens[i] << "'\n";
            tokens.clear();
            redirects.clear();
            return redirects;
        }

        redirects.push_back({target_fd, tokens[i + 1], append});
        tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
        // don't increment i: the erase shifted the next token into position i
    }

    return redirects;
}

void retrieve_path() {
    std::cout << std::filesystem::current_path().string() << std::endl;
}

void run_external(const std::vector<std::string> &tokens, const std::string &full_path,
                   const std::vector<Redirect> &redirects, bool is_background,
                   const std::string &job_command) {
    pid_t pid = fork();
    if ( pid < 0 ) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        for (const auto &r : redirects) {
            int flags = O_WRONLY | O_CREAT | (r.append ? O_APPEND : O_TRUNC);
            int fd = open(r.file.c_str(), flags, 0644);
            if (fd == -1) { perror("open"); exit(1); }
            dup2(fd, r.target_fd);
            close(fd);
        }

        std::vector<char*> args;
        for (const auto &token : tokens) {
          args.push_back(const_cast<char*>(token.c_str()));
        }
        args.push_back(nullptr);

        execvp(full_path.c_str() ,args.data());
        std::cerr << tokens[0] << ": command not found\n";
        std::exit(1);
    }
    if (is_background) {
        int job_number = next_job_number++;
        jobs_list.push_back({job_number, pid, job_command});
        std::cout << "[" << job_number << "] " << pid << std::endl;
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
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
        // check for background job request

        bool is_background = false ;
        if (!tokens.empty() && tokens.back() == "&") {
            is_background = true;
            tokens.pop_back();
        }

        // >' / '1>' / '2>' + filename before dispatching
        std::vector<Redirect> redirects = extract_redirects(tokens);
        if (tokens.empty())
            continue;   // syntax error already printed

        std::string job_command;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) job_command += " ";
            job_command += tokens[i];
        }

        bool builtin_cmd = (tokens[0] == "pwd" || tokens[0] == "cd" || tokens[0] == "echo" || tokens[0] == "type" || tokens[0] == "jobs");
        std::vector<std::pair<int,int>> saved_fds; // (fd_number, saved_dup)
        if (builtin_cmd && !redirects.empty()) {
            bool open_failed = false;
            for (const auto &r : redirects) {
                int flags = O_WRONLY | O_CREAT | (r.append ? O_APPEND : O_TRUNC);
                int redirect_fd = open(r.file.c_str(), flags, 0644);
                if (redirect_fd == -1) {
                    perror("open");
                    open_failed = true;
                    break;
                }
                saved_fds.push_back({r.target_fd, dup(r.target_fd)});
                dup2(redirect_fd, r.target_fd);
                close(redirect_fd);
            }
            if (open_failed) {
                // restore anything we already redirected, then skip this command
                for (auto &sf : saved_fds) {
                    dup2(sf.second, sf.first);
                    close(sf.second);
                }
                continue;
            }
        }

        std::string &cmd = tokens[0];

        if (cmd == "jobs") continue;

        if (cmd == "pwd")
        {
            retrieve_path();
        }
        else if (cmd == "cd")
        {
            if (tokens.size() < 2)
            {
                std::cerr << "cd: missing argument\n";
            }
            else
            {
                std::string target = tokens[1];

                if (target == "~")
                {
                    const char *home = std::getenv("HOME");
                    if (home) target = home;
                    else { std::cerr << "cd: HOME not set\n"; target.clear(); }
                }

                if (!target.empty()) {
                    if (!std::filesystem::exists(target) || !std::filesystem::is_directory(target))
                        std::cout << "cd: " << target << ": No such file or directory\n";
                    else
                        std::filesystem::current_path(target);
                }
            }
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
            }
            else
            {
                std::string s = tokens[1];

                if (s == "echo" || s == "exit" || s == "type" || s == "pwd" || s == "cd" || s == "jobs")
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
        }
        else
        {
            std::string path = find_in_path(cmd);
            if (!path.empty())
                run_external(tokens, path, redirects, is_background, job_command);
            else
                std::cout << input << ": command not found\n";
        }

        // restore any redirected fds for builtins
        if (builtin_cmd && !saved_fds.empty()) {
            std::cout.flush();
            std::cerr.flush();
            for (auto &sf : saved_fds) {
                dup2(sf.second, sf.first);
                close(sf.second);
            }
        }
    }
    return 0;
}