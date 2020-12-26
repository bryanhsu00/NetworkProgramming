#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include "mylib.hpp"
#include "numberpipe.hpp"

using namespace std;

int main(int argc , char *argv[]){
    NumberPipe* numberpipe = new NumberPipe();
    vector<string> envp;
    envp.push_back("PATH=bin:.");

    while (1){
        cout << "% ";
        string command;
        getline(cin, command);

        if(command.find("exit") != string::npos){
            exit(0);
        }

        numberpipe->rotate();
        command = numberpipe->find_number_pipe(command);

        if(is_env(command, envp)){
            continue;
        }

        if(command.find("|") == string::npos){ // if no pipes // fork area
            pid_t pid;
            pid = fork();
            if(pid == 0){ // child
                if(command.find(" > ") != string::npos){
                    vector<string> cmd_arg = split(command, " > ");
                    int filefd = open(cmd_arg[1].c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                    command = cmd_arg[0];
                    dup2(filefd, STDOUT_FILENO);
                }
                numberpipe->apply_dup2_stdin();
                numberpipe->apply_dup2_stdout();
                execute(command, envp);
            }
            else{ // parent
                numberpipe->close_in_pipe();
                numberpipe->erase_in_pipe();
                if(numberpipe->errflag == -1 && numberpipe->outflag == -1){ // not pipe n
                    if(numberpipe->inflag != -1){ // has in pipe
                        vector<pid_t> pidlist = numberpipe->get_pid();
                        for(int i=0; i<pidlist.size(); i++){
                            int status;
                            waitpid(pidlist[i], &status, 0);
                        }
                    }
                    waitpid(pid, NULL, 0);
                }
                else{ // is pipe n
                    numberpipe->add_pid(pid);
                    waitpid(-1, NULL, WNOHANG);
                }
                numberpipe->clear_flag();
            }
        }
        else { // contain pipes
            vector<string> cmds = split(command, " | ");
            int pipes[cmds.size()-1][2];

            for(int i=0; i<cmds.size(); i++){ // ls, cat
                pid_t pid;
                if(i < cmds.size()-1){
                    pipe(pipes[i]);
                }
                while((pid = fork()) < 0){
                    // usleep(1000);
                    waitpid(-1, NULL, 0);
                }
                if(pid == 0){
                    if(i == 0){ // first
                        numberpipe->apply_dup2_stdin();
                        close(pipes[i][0]);
                        dup2(pipes[i][1], STDOUT_FILENO);
                    }
                    else if(i == cmds.size()-1){ // last
                        close(pipes[i-1][1]);
                        dup2(pipes[i-1][0], STDIN_FILENO);
                        if(cmds[i].find(">") != string::npos){
                            vector<string> cmd_arg = split(cmds[i], " > ");
                            int filefd = open(cmd_arg[1].c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                            cmds[i] = cmd_arg[0];
                            dup2(filefd, STDOUT_FILENO);
                        }
                        numberpipe->apply_dup2_stdout();
                    }
                    else{ // mid
                        dup2(pipes[i-1][0], STDIN_FILENO);
                        dup2(pipes[i][1], STDOUT_FILENO);
                        close(pipes[i-1][1]);
                        close(pipes[i][0]);
                    }
                    execute(cmds[i], envp);
                }
                else{
                    if(i != 0){ // mid
                        close(pipes[i-1][0]);
                        close(pipes[i-1][1]);
                    }
                    if(i == 0){
                        numberpipe->close_in_pipe();
                    }
                    if(numberpipe->errflag == -1 && numberpipe->outflag == -1){ // not pipe n
                        if(numberpipe->inflag != -1){ // has in pipe
                            vector<pid_t> pidlist = numberpipe->get_pid();
                            for(int i=0; i<pidlist.size(); i++){
                                int status;
                                waitpid(pidlist[i], &status, 0);
                            }
                        }
                        waitpid(pid, NULL, 0);
                    }
                    else{
                        numberpipe->add_pid(pid);
                        waitpid(-1, NULL, WNOHANG);
                    }
                    if(i == cmds.size()-1){
                        numberpipe->erase_in_pipe();
                        numberpipe->clear_flag();
                    }
                }
            }
        }
    }
    return 0;
}