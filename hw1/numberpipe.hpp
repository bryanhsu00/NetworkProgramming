#include "mylib.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <regex>
#include <unistd.h>

using namespace std;

class NumberPipe {
    public:
        vector<int> indexlist;
        vector<int *> pipelist;
        vector<vector<pid_t>> pidlist;
        int inflag = -1, outflag = -1, errflag = -1;

        void rotate();

        string find_number_pipe(string command);

        void apply_dup2_stdin();

        void apply_dup2_stdout();

        void close_in_pipe();

        void erase_in_pipe();

        void clear_flag();

        void add_pid(pid_t pid);

        vector<pid_t> get_pid();
};