#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>    //inet_addr
#include <unistd.h>    //write
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include "mylib.hpp"
#include "numberpipe.hpp"

using namespace std;

int main(int argc , char *argv[]){
    int port = atoi(argv[1]);
    int socket_desc , client_sock , c , read_size;
    struct sockaddr_in server , client;
    char buf[20000];

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create socket");
    }
    puts("Socket created");
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    int option = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    NumberPipe* numberpipe = new NumberPipe();
    int* std;

    while (1){
        //accept connection from an incoming client
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0){
            perror("accept failed");
            return 1;
        }
        puts("Connection accepted");
        pid_t pid;
        vector<string> envp;
        envp.push_back("PATH=bin:.");
        
        write(client_sock, "% ", 2);

        //Receive a message from client
        while((read_size = recv(client_sock, buf, 20000, 0)) > 0){
            cout << buf;
            std = store_std();
            dup2(client_sock, STDOUT_FILENO);
            dup2(client_sock, STDERR_FILENO);

            string command = buf;
            for(int i=0; i<20000; i++)
                buf[i] = '\0';
            replace(command, "\n", "");
            replace(command, "\r", "");

            numberpipe->rotate();
            command = numberpipe->find_number_pipe(command);

            if(command.find("exit") != string::npos){
                restore_std(std);
                close(client_sock);
                break;
            }
            else if(is_env(command, envp)){
                write(client_sock, "% ", 2);
            }
            else if(command.find("|") == string::npos){ // if no pipes // fork area
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
                    // userpipelist.close_in_pipe();
                    numberpipe->close_in_pipe();
                    waitpid(pid, NULL, 0);
                    numberpipe->erase_in_pipe();
                    numberpipe->clear_flag();
                    write(client_sock, "% ", 2);
                }
            }
            else { // contain pipes
                vector<string> cmds = split(command, " | ");
                int pipes[cmds.size()-1][2];

                for(int i=0; i<cmds.size(); i++){ // ls, cat
                    if(i < cmds.size()-1){
                        pipe(pipes[i]);
                    }
                    while((pid = fork()) < 0){
                        usleep(1000);
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
                        if(i==0){
                            // userpipelist.close_in_pipe();
                            numberpipe->close_in_pipe();
                        }
                        waitpid(pid, NULL, 0);
                        if(i==cmds.size()-1){
                            numberpipe->erase_in_pipe();
                            numberpipe->clear_flag();
                            write(client_sock, "% ", 2);
                        }
                    }
                }
            }
            restore_std(std);
        }
        if(read_size == 0){
            puts("Client disconnected");
            fflush(stdout);
        }
        else if(read_size == -1){
            perror("recv failed");
        }
    }
    return 0;
}