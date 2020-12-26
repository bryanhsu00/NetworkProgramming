#include "numberpipe.hpp"

using namespace std;

void NumberPipe::rotate(){
    if(indexlist.size() > 0){ // rotate
        for(int i=0; i<indexlist.size(); i++){
            indexlist[i]-- ;
            if(indexlist[i] == 0)
                inflag = i;
        }
    }
}

string NumberPipe::find_number_pipe(string command){
    regex reg(".*?\\|[0-9]+");
    regex reg2(".*?\\![0-9]+");

    if(regex_match(command, reg) || regex_match(command, reg2)){
        string num_str;
        if(regex_match(command, reg))
            num_str = split(command, "|").back();
        else
            num_str = split(command, "!").back();

        int exist = -1;
        for(int i=0; i<indexlist.size(); i++)
            if(indexlist[i] == stoi(num_str))
                exist = i;

        if(exist != -1){ // number pipe exist
            if(regex_match(command, reg))
                outflag = exist;
            else
                errflag = exist;
        }
        else{ // number pipe doesn't exist
            // cerr << "match" << endl;
            indexlist.push_back(stoi(num_str));
            int* fd = new int[2];
            pipe(fd);
            pipelist.push_back(fd);
            if(regex_match(command, reg))
                outflag = indexlist.size() - 1;
            else
                errflag = indexlist.size() - 1;
        }
        command = command.substr(0, command.size()-num_str.length()-2);
    }
    return command;
}

void NumberPipe::apply_dup2_stdin(){
    if(inflag != -1){
        dup2(pipelist[inflag][0], STDIN_FILENO);
        close(pipelist[inflag][1]);
    }
}

void NumberPipe::apply_dup2_stdout(){
    if(errflag != -1){
        dup2(pipelist[errflag][1], STDOUT_FILENO);
        dup2(pipelist[errflag][1], STDERR_FILENO);
    }
    if(outflag != -1){
        dup2(pipelist[outflag][1], STDOUT_FILENO);
    }
}

void NumberPipe::close_in_pipe(){
    if(inflag != -1){
        close(pipelist[inflag][1]);
    }
}

void NumberPipe::erase_in_pipe(){
    if(inflag != -1){
        close(pipelist[inflag][0]);
        pipelist.erase(pipelist.begin() + inflag);
        indexlist.erase(indexlist.begin() + inflag);
    }
}

void NumberPipe::clear_flag(){
    inflag = -1;
    outflag = -1;
    errflag = -1;
}