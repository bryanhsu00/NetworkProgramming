#include "mylib.hpp"

using namespace std;

bool is_number(const string& s){
    string::const_iterator it = s.begin();
    while (it != s.end() && isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

vector<string> split(string s,string delimiter){
    vector<string> res;
    size_t pos = 0;
    string token;
    while ((pos = s.find(delimiter)) != string::npos) {
        token = s.substr(0, pos);
        res.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    res.push_back(s);
    return res;
}

bool replace(string& str, const string& from, const string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void execute(string command, vector<string> envp){
    clearenv();
    for(int i=0; i<envp.size(); i++){
        char *r = new char[envp[i].length() + 1];
        strcpy(r, envp[i].c_str());
        putenv(r);
    }
    vector<string> cmd_arg = split(command, " ");
    int argcount = cmd_arg.size();
    char** args = (char**) malloc((argcount + 1) * sizeof(char *));
    for(int i=0; i<argcount; i++){
        args[i] = (char *) cmd_arg[i].c_str();
    }
    args[argcount] = NULL;
    if(execvp(args[0], args) == -1){
        // cerr << "Unknown command: [" << cmd_arg[0] << "].";
        replace(cmd_arg[0], "\n", "");
        string msg = "Unknown command: [" + cmd_arg[0] + "].\r\n";
        write(STDERR_FILENO, msg.c_str(), msg.length());
        exit(0);
    }
}

bool is_env(string command, vector<string> &envp){
    if(command.find("printenv") != string::npos){ // printenv PATH
        vector<string> cmd_arg = split(command, " ");
        for(int i=0; i<envp.size(); i++){
            if(envp[i].find(cmd_arg[1]) != string::npos){
                string newstr = envp[i];
                replace(newstr, cmd_arg[1]+"=", "");
                cout << newstr << endl;
            }
        }
        return true;
    }

    if(command.find("setenv") != string::npos){ // setenv PATH bin
        vector<string> cmd_arg = split(command, " ");
        for(int i=0; i<envp.size(); i++){
            if(envp[i].find(cmd_arg[1]) != string::npos){
                envp[i] = cmd_arg[1] + "=" + cmd_arg[2];
            }
        }
        return true;
    }
    return false;
}

// string ch_to_str(char buf[], int buf_size){
//     string result = "";
//     for(int i=0; i<buf_size; i++){
//         if(buf[i] == '\r' || buf[i] == '\n'){
//             break;
//         }
//         result = result + buf[i];
//     }
//     return result;
// }

int* store_std(){
    int* arr= new int[3];
    arr[0] = dup(STDIN_FILENO);
    arr[1] = dup(STDOUT_FILENO);
    arr[2] = dup(STDERR_FILENO);
    return arr;
}

void restore_std(int* arr){
    dup2(arr[0], STDIN_FILENO);
    dup2(arr[1], STDOUT_FILENO);
    dup2(arr[2], STDERR_FILENO);
    close(arr[0]);
    close(arr[1]);
    close(arr[2]);
    delete[] arr;
}