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

void read_pipe(int pipe_fd,
                vector<string> &field_name,
                vector<string> &field_value,
                vector<string> &files,
                string &pipe_name,
                string &dir)
{
    stringstream ss;
    int i;
    char temp[11];
    while (i = read(pipe_fd, temp, 10))
    {
        temp[i] = '\0';
        ss << temp;
        if (i != 10)
            break;
    }

    string temp1, temp2, input;
    int file_cnt;
    getline(ss, input);
    char* copy_input = strdup(input.c_str());
    temp1 = strtok(copy_input, "- =");
    while (true)
    {
        temp2 = strtok(NULL, "- =");
        if (temp1 == "file_cnt")
        {
            file_cnt = atoi(temp2.c_str());
            delete copy_input;
            break;
        }

        else if (temp1 == "pipe_name")
            pipe_name = temp2;

        else if (temp1 == "dir")
            dir = temp2;

        else
        {
            field_name.push_back(temp1);
            field_value.push_back(temp2);
        }
        temp1 = strtok(NULL, "- =");
    }

    getline(ss, input);
    
    stringstream ss2;
    ss2 << input;
    string file_name;
    while (ss2 >> file_name)
        files.push_back(file_name);
}

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

void find_target_field(vector<string> &field_name,
                        vector<string> &field_value,
                        vector<string> &fields,
                        vector<int> &target_field_indices,
                        vector<string> &target_field_value)
{
    for (int i = 0; i < field_name.size(); i++)
    {
        for (int j = 0; j < fields.size(); j++)
        {
            if (field_name[i] == fields[j])
            {
                target_field_indices.push_back(j);
                target_field_value.push_back(field_value[i]);
            }
        }
    }
}

void search(vector<string> &lines,
            vector<string> &files,
            vector<string> &field_name,
            vector<string> &field_value,
            const string &dir)
{
    string address, line;
    bool add = false;
    for (auto file : files)
    {
        address = dir + "/" + file;
        std::ifstream input(address.c_str());
        getline(input, line);
        vector<string> fields;
        get_fields(line, fields, "-");
        vector<int> target_field_indices;
        vector<string> target_field_value;
        find_target_field(field_name, field_value, fields, target_field_indices, target_field_value);
        vector<string> columns;
        for (std::string line; getline(input, line);)
        {
            get_fields(line, columns, "-");
            for (int index = 0; index < target_field_indices.size(); index++)
            {
                if (columns[target_field_indices[index]] != target_field_value[index])
                {
                    add = false;
                    break;
                }
            }
            if (add)
                lines.push_back(line);
            add = true;
            columns.clear();
        }
        fields.clear();
        target_field_value.clear();
        target_field_indices.clear();
    }
}

void write_for_presenter(string pipe_name, vector<string> &lines)
{
    int fd = open(pipe_name.c_str(), O_APPEND | O_WRONLY);
    for (auto line : lines)
    {
        write(fd, line.c_str(), line.length());
        write(fd, "\n", 1);
    }
    write(fd, "**********", 10);
    write(fd, "\n", 1);
    close(fd);
}

int main(int argc, char* argv[])
{
    string pipe_name;
    int pipe_fd = atoi(argv[0]);
    string dir;
    vector<string> field_name;
    vector<string> field_value;
    vector<string> files;
    vector<string> lines;

    read_pipe(pipe_fd, field_name, field_value, files, pipe_name, dir);
    search(lines, files, field_name, field_value, dir);
    write_for_presenter(pipe_name, lines);
    exit(EXIT_SUCCESS);
}
