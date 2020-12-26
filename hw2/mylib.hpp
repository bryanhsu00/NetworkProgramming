#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <string.h>

using namespace std;

bool is_number(const string& s);

vector<string> split(string s,string delimiter);

bool replace(string& str, const string& from, const string& to);

void execute(string command, vector<string> envp);

bool is_env(string command, vector<string>& envp);

string ch_to_str(char buf[], int buf_size);

int* store_std();

void restore_std(int* arr);
