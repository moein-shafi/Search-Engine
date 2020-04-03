#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/stat.h> 
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <algorithm>

using namespace std;

void remove_spaces(string &str)
{
    int begin, end;
    for (int i = 0; i < str.size(); i++)
    {
        if (str[i] != ' ')
        {
            begin = i;
            break;
        }
    }
    
    for (int i = str.size() - 1; i >= 0; i--)
    {
        if (str[i] != ' ')
        {
            end = i;
            break;
        }
    }
    str.erase(end + 1);
    str.erase(0, begin); 
}

void get_fields(string line, vector<string> &fields, string method)
{
    char* input_copy = strdup(line.c_str());
    char* temp = strtok(input_copy, method.c_str());
    string field;

    while (temp != NULL)
    {
        field = temp;
        remove_spaces(field);
        fields.push_back(field);
        temp = strtok(NULL, method.c_str());
    }
    delete input_copy;
}

void add_to_sort_list(string line,
                        int sort_field_index,
                        vector<vector<string> > &sorted_lines)
{
    vector<string> new_line;
    get_fields(line, new_line, "-");
    if (sorted_lines.size() == 0)
    {
        sorted_lines.push_back(new_line);
        return;
    }

    int cmp;
    for (int i = 0; i < sorted_lines.size(); i++)
    {
        if ((int)new_line[sort_field_index][0] < 58 && (int)new_line[sort_field_index][0] > 47)
        {
            if (stof(new_line[sort_field_index].c_str()) <= stof(sorted_lines[i][sort_field_index].c_str()))
                cmp = 0;
            else
                cmp = 1;
        }
        else
            cmp = new_line[sort_field_index].compare(sorted_lines[i][sort_field_index]);

        if (cmp <= 0)
        {
            sorted_lines.insert(sorted_lines.begin() + i, new_line);
            return;
        }
    }
    sorted_lines.push_back(new_line);
}

int find_sort_field_index(vector<string> &fields, string sort_field)
{
    for (int i = 0; i < fields.size(); i++)
        if (fields[i] == sort_field)
            return i;
}

void wait_for_balancer_data(ifstream &myfifo)
{
	int current_pos = myfifo.tellg();
	myfifo.seekg(0, ios_base::end);
	int end = myfifo.tellg();
	while(end - current_pos < 256)
	{
		myfifo.seekg(0, ios_base::end);
		end = myfifo.tellg();
	}
	myfifo.seekg(current_pos, ios_base::beg);
}

void read_file(stringstream &ss, ifstream &myfifo, int &readed_pos)
{
    myfifo.seekg(0, myfifo.end);
    int length = myfifo.tellg();
    int temp = length;
    length -= readed_pos;
    if (length == 0)
    {
        while(length - readed_pos <= 10)
        {
            myfifo.seekg(0, myfifo.end);
            length = myfifo.tellg();
        }
        temp = length;
        length -= readed_pos;
    }

    myfifo.seekg(readed_pos, myfifo.beg);
    readed_pos = temp;
    char * buffer = new char [length + 1];
    myfifo.read(buffer, length);
    buffer[length + 1] = '\0';
    ss << buffer;
    delete buffer;
}

void read_pipe(string pipe_name,
                vector<vector<string> > &lines,
                int &is_ascending)
{
    int readed_pos = 0;
	ifstream myfifo(pipe_name);
    wait_for_balancer_data(myfifo);
	stringstream ss;
    read_file(ss, myfifo, readed_pos);

    vector<string> sort_method;
	string sort_line;
    getline(ss, sort_line);
    get_fields(sort_line, sort_method, "=-");
    string sort_field = sort_method[0];
    string sort_value = sort_method[1];
    int number_of_processes = atoi(sort_method[3].c_str());
  
    string field_line;
    vector<string> fields;
    getline(ss, field_line);
    get_fields(field_line, fields, "#");

    if (sort_field == "None")
        sort_field = fields[1];

    if (sort_value == "ascending")
        is_ascending = 1;
    else
        is_ascending = 0;

    int sort_field_index = find_sort_field_index(fields, sort_field);
    int counted_process = 0;
    string line;
    int line_counter = 2;
    readed_pos = 0;
    while (counted_process != number_of_processes)
    {
        stringstream ss2;
        read_file(ss2, myfifo, readed_pos);
        if (line_counter == 2)
        {
            getline(ss2, line);
            getline(ss2, line);
        }

        for (string line; getline(ss2, line); line_counter++)
        {
            if (line == "**********")
                counted_process++;

            else
            {
                if (line.size() < 3)
                    continue;
                add_to_sort_list(line, sort_field_index, lines);
            }
        }
    }
}

void show_reslut(vector<vector<string> > &sorted_lines, int is_ascending)
{
    if (is_ascending == 0)
        reverse(sorted_lines.begin(), sorted_lines.end());
    for (auto line : sorted_lines)
    {
        for (int i = 0; i < line.size(); i++)
        {
            cout << line[i];
            if (i != line.size() - 1)
                cout << " - ";
        }
        cout << endl;
    }
}
int main(int argc, char* argv[])
{
    string pipe_name = argv[0];
    vector<vector<string> > sorted_lines;
    int is_ascending = 0;
    read_pipe(pipe_name, sorted_lines, is_ascending);
    show_reslut(sorted_lines, is_ascending);
    exit(EXIT_SUCCESS);
}
