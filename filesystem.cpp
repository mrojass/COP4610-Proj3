#include <iostream>
#include <string>
#include <cstdint>
using namespace std;

int fsinfo();
int open(string file_name, string mode);
int close(string file_name);
int create(string file_name);
int read(string file_name, uint start_pos, uint num_bytes);
int write(string file_name, uint start_pos, string quoted_data);
int rm(string file_name);
int cd(string dir_name);
int ls(string dir_name);
int mkdir(string dir_name);
int rmdir(string dir_name);
int size(string entry_name);
int undelete();

int main(void)
{
	string cmd;
	cout << "prompt$ ";
	cin >> cmd;
	return 0;
}