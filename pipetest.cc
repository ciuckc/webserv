#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    int fd[2];
    if (pipe(fd) == -1) {
        return 1;
    }
    
    int pid = fork();
    if (pid < 0) {
        return 2;
    }
    
    if (pid == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        // std::cout << "PID: " << pid << std::endl;
        execve("cgi.py", NULL, NULL);
		    return (0);
    }
    
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]);
    close(fd[1]);
    std::string s, temp;
    std::stringstream ss;
    // while (std::cin >> temp) {s += temp;}
    ss << std::cin.rdbuf();
    std::cout << ss.str();
    waitpid(pid, NULL, 0);
    return 0;
}
