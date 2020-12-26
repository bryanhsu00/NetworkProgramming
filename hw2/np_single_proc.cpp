#include <stdio.h> /* Basic I/O routines 		*/
#include <sys/types.h> /* standard system types 	*/
#include <netinet/in.h> /* Internet address structures 	*/
#include <sys/socket.h> /* socket interface functions 	*/
#include <netdb.h> /* host to IP resolution 	*/
#include <sys/time.h> /* for timeout values 		*/
#include <unistd.h> /* for table size calculations 	*/
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include "mylib.hpp"
#include "userlist.hpp" 
#include <iostream>
#include <fcntl.h>
#include <sys/wait.h>

using namespace std;

// extern char **environ;

#define	BUFLEN 1024 /* buffer length */

int main(int argc, char *argv[]){
    int port = atoi(argv[1]);
    int read_size = 0;
    int server, client; /* socket descriptor */
    char buf[BUFLEN]; /* buffer for incoming data */
    struct sockaddr_in sa, csa; /* Internet address struct */
    fd_set open_sockets, wait_read_sockets; /* set of open sockets */
    int tablesize; /* size of file descriptors table */

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
        perror("socket: allocation failed");

    int option = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if (bind(server, (struct sockaddr *)&sa, sizeof(sa)))
        perror("bind");

    if (listen(server, 1024))
        perror("listen");

    tablesize = getdtablesize();

    FD_ZERO(&open_sockets);
    FD_SET(server, &open_sockets);
    cout << "starting ~" << endl;

    UserList userlist;
    UserPipeList userpipelist;
    int* std;

    while (1) {
        wait_read_sockets = open_sockets;
        select(tablesize, &wait_read_sockets, NULL, NULL, NULL);
        if (FD_ISSET(server, &wait_read_sockets)) {
            socklen_t client_len = sizeof(csa);
            client = accept(server, (struct sockaddr *)&csa, &client_len);
            if (client < 0)
                continue;
            FD_SET(client, &open_sockets);
            char ip_arr[INET_ADDRSTRLEN];
            string ip_str = inet_ntop( AF_INET, &csa.sin_addr, ip_arr, INET_ADDRSTRLEN );
            string port = to_string(ntohs(csa.sin_port));
            userlist.addUser(client, ip_str + ":" + port);
            userlist.welcome(ip_str + ":" + port);
            write(client, "% ", 2);
        }
        else{
            for (int i=0; i<tablesize; i++){
                if (i != server && FD_ISSET(i, &wait_read_sockets)){
                    read_size = read(i, buf, BUFLEN);
                    cout << buf << endl;
                    string command = buf;
                    for(int i=0; i<1024; i++)
                        buf[i] = '\0';
                    if (read_size == 0){
                        close(i);
                        FD_CLR(i, &open_sockets);
                    }
                    else{
                        pid_t pid;
                        User* user = userlist.get_user_from_fd(i);
                        NumberPipe* numberpipe = &user->numberpipe;

                        // replace(command, "\r\n", "");
                        replace(command, "\n", "");
                        replace(command, "\r", "");

                        numberpipe->rotate();

                        command = userpipelist.find_userpipe_stdin(command, user, &userlist);
                        command = userpipelist.find_userpipe_stdout(command, user, &userlist);
                        command = numberpipe->find_number_pipe(command);

                        std = store_std();
                        dup2(i, STDOUT_FILENO);
                        dup2(i, STDERR_FILENO);

                        if(command == "who"){
                            userlist.who(i);
                        }
                        else if(command.find("name") != string::npos){
                            userlist.name(i, command);
                        }
                        else if(command.find("yell") != string::npos){
                            userlist.yell(i, command);
                        }
                        else if(command.find("tell") != string::npos){
                            userlist.tell(i, command);
                        }
                        else if(command.find("exit") != string::npos){
                            string username = user->name;
                            int user_id = user->id;
                            userlist.remove_from_fd(i);
                            close(i);
                            FD_CLR(i, &open_sockets);
                            string msg = "*** User '" +  username + "' left. ***\r\n";
                            userlist.broadcast(msg);
                            userpipelist.remove(user_id);
                        }
                        else if(is_env(command, user->envp)){
                            write(user->fd, "% ", 2);
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
                                userpipelist.apply_dup2_stdin();
                                numberpipe->apply_dup2_stdin();
                                userpipelist.apply_dup2_stdout();
                                numberpipe->apply_dup2_stdout();
                                execute(command, user->envp);
                            }
                            else{ // parent
                                // userpipelist.close_in_pipe();
                                numberpipe->close_in_pipe();
                                waitpid(pid, NULL, 0);
                                userpipelist.erase_in_pipe();
                                numberpipe->erase_in_pipe();
                                userpipelist.clear_flag();
                                numberpipe->clear_flag();
                                write(user->fd, "% ", 2);
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
                                        userpipelist.apply_dup2_stdin();
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
                                        userpipelist.apply_dup2_stdout();
                                        numberpipe->apply_dup2_stdout();
                                    }
                                    else{ // mid
                                        dup2(pipes[i-1][0], STDIN_FILENO);
                                        dup2(pipes[i][1], STDOUT_FILENO);
                                        close(pipes[i-1][1]);
                                        close(pipes[i][0]);
                                    }
                                    execute(cmds[i], user->envp);
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
                                        userpipelist.erase_in_pipe();
                                        numberpipe->erase_in_pipe();
                                        userpipelist.clear_flag();
                                        numberpipe->clear_flag();
                                        write(user->fd, "% ", 2);
                                    }
                                }
                            }
                        }
                        restore_std(std);
                    }
                }
            }
        }
    }
}
