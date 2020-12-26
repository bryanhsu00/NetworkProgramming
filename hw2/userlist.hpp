#include <iostream>
#include <vector>
#include "numberpipe.hpp"

using namespace std;

class User {
    public:
        string name;
        int id;
        int fd;
        NumberPipe numberpipe;
        string ip_str;
        vector<string> envp;

        User(int id, int fd, string ip_str);
};

class UserList {
    public:
        vector<User> userlist;

        void addUser(int fd, string ip_str);

        void welcome(string ip_str);

        void who(int fd);

        void name(int fd, string command);

        void yell(int fd, string command);

        void tell(int fd, string command);

        void broadcast(string msg);

        User* get_user_from_fd(int fd);

        User* get_user_from_id(int id);
        
        void remove_from_fd(int fd);
};

class UserPipe {
    public:
        int from;
        int to;
        int fd[2];

        UserPipe(int from, int to);
};

class UserPipeList {
    public:
        string command;
        int in_flag;
        int out_flag;
        bool in_null;
        bool out_null;

        vector<UserPipe *> userpipelist;

        UserPipeList();

        string find_userpipe_stdin(string command, User* user, UserList* userlist);

        string find_userpipe_stdout(string command, User* user, UserList* userlist);

        bool userpipe_exist(int from, int to);

        int get_userpipe(int from, int to);

        void apply_dup2_stdin();

        void apply_dup2_stdout();

        // void close_in_pipe();

        void erase_in_pipe();

        void clear_flag();

        void remove(int to_id);
};


