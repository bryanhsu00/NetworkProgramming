#include <iostream>
#include <vector>
#include "userlist.hpp"
#include "mylib.hpp"
#include <fcntl.h>

using namespace std;

struct less_than_key {
    inline bool operator() (const User& struct1, const User& struct2){
        return (struct1.id < struct2.id);
    }
};

User::User(int id, int fd, string ip_str){
    this->id = id;
    this->fd = fd;
    this->name = "(no name)";
    NumberPipe numberpipe;
    this->numberpipe = numberpipe;
    this->ip_str = ip_str;
    this->envp.push_back("PATH=bin:.");
}

void UserList::addUser(int fd, string ip_str){
    string msg = "****************************************\r\n"
                    "** Welcome to the information server. **\r\n"
                    "****************************************\r\n";
    int count = 1;
    if(userlist.size() == 0){
        User user(count, fd, ip_str);
        userlist.push_back(user);
        write(user.fd, msg.c_str(), msg.length());
    }
    else{
        while(true){
            bool exist = false;
            for(int i=0; i<userlist.size(); i++){ // 4 1 2 
                if(userlist[i].id == count){
                    exist = true;
                    count++;
                    break;
                }
            }
            if(!exist){
                User user(count, fd, ip_str);
                userlist.push_back(user);
                write(user.fd, msg.c_str(), msg.length());
                break;
            }
        }
    }
}

void UserList::welcome(string ip_str){
    string msg = "*** User '(no name)' entered from " + ip_str + ". ***\r\n";
    broadcast(msg);
}

void UserList::who(int fd){
    sort(userlist.begin(), userlist.end(), less_than_key());
    string msg = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\r\n";
    for(int i=0; i<userlist.size(); i++){
        if(userlist[i].fd == fd){
            msg += to_string(userlist[i].id) + "\t" + userlist[i].name + "\t" + userlist[i].ip_str + "\t" + "<-me\r\n";
        }
        else{
            msg += to_string(userlist[i].id) + "\t" + userlist[i].name + "\t" + userlist[i].ip_str + "\r\n";
        }
    }
    write(fd, msg.c_str(), msg.length());
    write(fd, "% ", 2);
}

void UserList::name(int fd, string command){
    User* user = get_user_from_fd(fd);
    string name = split(command, " ")[1];
    for(int i=0; i<userlist.size(); i++){
        if(userlist[i].name == name){
            string msg = "*** User '" + name + "' already exists. ***\r\n";
            write(fd, msg.c_str(), msg.length());
            write(fd, "% ", 2);
            return;
        }
    }
    user->name = name;
    string msg = "*** User from " + user->ip_str + " is named '" + user->name + "'. ***\r\n";
    broadcast(msg);
    write(fd, "% ", 2);
}

void UserList::yell(int fd, string command){
    command.erase(0, 4);
    User* user = get_user_from_fd(fd);
    string msg = "*** " + user->name + " yelled ***:" + command + "\r\n";
    broadcast(msg);
    write(fd, "% ", 2);
}

void UserList::tell(int fd, string command){
    // tell 2 Plz help me, my friends!
    vector<string> cmd_arg = split(command, " ");
    string recv_msg = "";
    for(int i=2; i<cmd_arg.size(); i++){
        if(i == cmd_arg.size() - 1)
            recv_msg = recv_msg + cmd_arg[i];
        else
            recv_msg = recv_msg + cmd_arg[i] + " ";
    }
    User* recver = get_user_from_id(stoi(cmd_arg[1]));
    if(recver == NULL){
        string msg = "*** Error: user #" + cmd_arg[1] + " does not exist yet. ***\r\n";
        write(fd, msg.c_str(), msg.length());
        write(fd, "% ", 2);
        return;
    }
    User* sender = get_user_from_fd(fd);

    string msg = "*** " + sender->name + " told you ***: " + recv_msg + "\r\n";
    write(recver->fd, msg.c_str(), msg.length());
    write(fd, "% ", 2);
}

User* UserList::get_user_from_fd(int fd){
    for(int i=0; i<userlist.size(); i++){
        if(userlist[i].fd == fd){
            return &userlist[i];
        }
    }
    return NULL;
}

User* UserList::get_user_from_id(int id){
    for(int i=0; i<userlist.size(); i++){
        if(userlist[i].id == id){
            return &userlist[i];
        }
    }
    return NULL;
}

void UserList::broadcast(string msg){
    for(int i=0; i<userlist.size(); i++){
        write(userlist[i].fd, msg.c_str(), msg.length());
    }
}

void UserList::remove_from_fd(int fd){
    for(int i=0; i<userlist.size(); i++){
        if(userlist[i].fd == fd){
            userlist.erase(userlist.begin()+i);
            return;
        }
    }
}

UserPipe::UserPipe(int from, int to){
    this->from = from;
    this->to = to;
    pipe(this->fd);
}

UserPipeList::UserPipeList(){
    this->in_null = false;
    this->out_null = false;
    this->in_flag = -1;
    this->out_flag = -1;
    this->command = "";
}

string UserPipeList::find_userpipe_stdin(string command, User* user, UserList* userlist){ // set command and flag
    regex rgx_in(".*?(\\s<[0-9]+)");
    smatch match_in;
    if(regex_search(command, match_in, rgx_in)){
        this->command = command; // copy origin command
        string str_in = match_in[1].str();
        replace(command, str_in, "");
        replace(str_in, " <", ""); // replace " <" in " <2"
        int from_id = stoi(str_in);
        if(userpipe_exist(from_id, user->id)){
            User* from = userlist->get_user_from_id(from_id);
            string msg = "*** " + user->name + " (#" + to_string(user->id) + ") just received from " + from->name + " (#" + to_string(from->id) + ") by '" + this->command + "' ***\r\n";
            userlist->broadcast(msg);
            this->in_flag = get_userpipe(from_id, user->id);
        }
        else{
            if(userlist->get_user_from_id(from_id) == NULL){
                string msg = "*** Error: user #" + to_string(from_id) + " does not exist yet. ***\r\n";
                write(user->fd, msg.c_str(), msg.length());
                this->in_null = true;
            }
            else{
                string msg =  "*** Error: the pipe #" + to_string(from_id) + "->#" + to_string(user->id) + " does not exist yet. ***\r\n";
                write(user->fd, msg.c_str(), msg.length());
                this->in_null = true;
            }
        }
        return command;
    }
    else
        return command;
}

string UserPipeList::find_userpipe_stdout(string command, User* user, UserList* userlist){
    regex rgx_out(".*?(\\s>[0-9]+)");
    smatch match_out;
    if(regex_search(command, match_out, rgx_out)){
        if(this->command == "")
            this->command = command; // copy origin command
        string str_out = match_out[1].str();
        replace(command, str_out, "");
        replace(str_out, " >", ""); // replace " >" in " >2"
        int to_id = stoi(str_out);
        if(userpipe_exist(user->id, to_id)){
            string msg = "*** Error: the pipe #" + to_string(user->id) + "->#" + to_string(to_id) + " already exists. ***\r\n";
            write(user->fd, msg.c_str(), msg.length());
            this->out_null = true;
        }
        else{
            User* touser = userlist->get_user_from_id(to_id);
            if(touser == NULL){
                string msg = "*** Error: user #" + to_string(to_id) + " does not exist yet. ***\r\n";
                write(user->fd, msg.c_str(), msg.length());
                this->out_null = true;
            }
            else{
                string msg = "*** " + user->name + " (#" + to_string(user->id) + ") just piped '" + this->command +"' to " + touser->name + " (#" + to_string(touser->id) + ") ***\r\n";
                userlist->broadcast(msg);
                UserPipe* newUserPipe = new UserPipe(user->id, to_id);
                this->userpipelist.push_back(newUserPipe);
                this->out_flag = userpipelist.size()-1;
            }
        }
        return command;
    }
    else
        return command;
}

bool UserPipeList::userpipe_exist(int from, int to){
    for(int i=0; i<userpipelist.size(); i++){
        if(userpipelist[i]->from == from){
            if(userpipelist[i]->to == to){
                return true;
            }
        }
    }
    return false;
}

int UserPipeList::get_userpipe(int from, int to){
    for(int i=0; i<userpipelist.size(); i++){
        if(userpipelist[i]->from == from){
            if(userpipelist[i]->to == to){
                return i;
            }
        }
    }
    return -1;
}

void UserPipeList::apply_dup2_stdin(){
    if(in_flag != -1){
        dup2(userpipelist[in_flag]->fd[0], STDIN_FILENO);
    }
    else if(in_null){
        int dev_null = open("/dev/null", O_RDONLY);
        dup2(dev_null, STDIN_FILENO);
    }
}

void UserPipeList::apply_dup2_stdout(){
    if(out_flag != -1){
        dup2(userpipelist[out_flag]->fd[1], STDOUT_FILENO);
    }
    else if(out_null){
        int dev_null = open("/dev/null", O_WRONLY);
        dup2(dev_null, STDOUT_FILENO);
    }
}

// void UserPipeList::close_in_pipe(){
//     if(in_flag != -1){
//         close(userpipelist[in_flag]->fd[1]);
//     }
// }

void UserPipeList::erase_in_pipe(){
    if(in_flag != -1){
        close(userpipelist[in_flag]->fd[0]);
        userpipelist.erase(userpipelist.begin() + in_flag);
    }
    if(out_flag != -1){
        close(userpipelist[out_flag]->fd[1]);
    }
}

void UserPipeList::clear_flag(){
    this->in_null = false;
    this->out_null = false;
    this->in_flag = -1;
    this->out_flag = -1;
    this->command = "";
}

void UserPipeList::remove(int to_id){
    for(int i=0; i<userpipelist.size(); i++){
        if(userpipelist[i]->to == to_id){
            userpipelist.erase(userpipelist.begin()+i);
        }
    }
}
