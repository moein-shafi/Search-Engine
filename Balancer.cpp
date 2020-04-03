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

#define PIPE_NAME "pipe.txt"

using namespace std;

int get_input(vector<string> &field_name, vector<string> &field_value,
                string &sort_field, string &dir,
                int &is_ascending, int &number_of_processes)
{
    string input, temp1, temp2;
    getline(cin, input);
    if (input == "quit")
        exit(EXIT_SUCCESS);

    if (input.empty())
        return -1;

    char* input_copy = strdup(input.c_str());
    temp1 = strtok(input_copy, "- =");

    while (true)
    {
        temp2 = strtok(NULL, "- =");
        if (temp2 == "ascending" || temp2 == "descending")
        {
            sort_field = temp1;
            if (temp2 == "ascending")
                is_ascending = 1;
            else
                is_ascending = 0;
        }
        else if (temp1 == "processes")
            number_of_processes = atoi(temp2.c_str());

        else if (temp1 == "dir")
        {
            dir = temp2;
            break;
        }
        
        else
        {
            field_name.push_back(temp1);
            field_value.push_back(temp2);
        }

        temp1 = strtok(NULL, "- =");
    }
    delete input_copy;
    return 0;
}

void wait_cpids(vector<pid_t> &pids)
{
    int status;
    for (auto pid : pids)
    {
        while (waitpid(pid, &status, 0) == -1)
        {
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            {
                cout << "proccess with pid : " << pid << " failed.\n";
                return;
            }
        }
    }
}

void write_for_worker(int &devolved_files,
                        vector<string> &field_name,
                        vector<string> &field_value,
                        vector<string> &files,
                        int file_cnt, int fd, string dir)
{
	int piped_chars = 0;
	for (int i = 0; i < field_name.size(); i++)
	{
		piped_chars += write(fd, field_name[i].c_str(), field_name[i].size());
		piped_chars += write(fd, " = ", 3);
		piped_chars += write(fd, field_value[i].c_str(), field_value[i].size());
		piped_chars += write(fd, " - ", 3);
	}

    string pipe_name = PIPE_NAME;
    piped_chars += write(fd, "pipe_name = ", 12);
	piped_chars += write(fd, pipe_name.c_str(), pipe_name.size());
	piped_chars += write(fd, " - ", 3);

    piped_chars += write(fd, "dir = ", 6);
	piped_chars += write(fd, dir.c_str(), dir.size());
	piped_chars += write(fd, " - ", 3);

	piped_chars += write(fd, "file_cnt = ", 11);
	piped_chars += write(fd, to_string(file_cnt).c_str(), to_string(file_cnt).size());
	piped_chars += write(fd, "\n", 1);
    
	for (int i = 0; i < file_cnt; i++)
	{
		piped_chars += write(fd, files[devolved_files].c_str(), files[devolved_files].size());
		devolved_files++;
		piped_chars += write(fd, " ", 1);
	}

	if (piped_chars % 10 == 0)
		write(fd, " ", 1);
}

void write_for_presenter(string &sort_field,
                         int is_ascending,
                         int prc_cnt,
                         int fd,
                         vector<string> &fields,
                         int number_of_processes)
{
    string sorting_method;
    if (is_ascending == 1)
        sorting_method = "ascending";
    else
        sorting_method = "descending";

	if(!sort_field.size())
	    sort_field = "None";

    write(fd, sort_field.c_str(), sort_field.size());
    write(fd, " = ", 3);
    write(fd, sorting_method.c_str(), sorting_method.size());
    write(fd, " - ", 3);

    write(fd, "processes = ", 12);
    write(fd, to_string(number_of_processes).c_str(), to_string(number_of_processes).size());

    write(fd, "\n", 1);

    for (auto field : fields)
    {
        write(fd, field.c_str(), field.size());
        if (field != fields[fields.size() - 1])
            write(fd, " # ", 3);
    }
    write(fd, "\n", 1);
    close(fd);
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

void read_fields(vector<string> &fields, string dir, string file)
{
    string line;
    string address = dir + "/" + file;
    std::ifstream input(address.c_str());
    getline(input, line);
    get_fields(line, fields, "-");
}

void division_of_labor(vector<string> &field_name, vector<string> &field_value,
                string &sort_field, string &dir,
                int &is_ascending, int &number_of_processes,
                vector<string> files)
{
    vector<pid_t> pids;
    int ratio = files.size() / number_of_processes;
    int remainded = files.size() % number_of_processes;
    int p[2];
    int devolved_files = 0;
    pid_t pid;
    ofstream f(PIPE_NAME, std::ofstream::out | std::ofstream::trunc);
    f.close();
    for (int i = 0; i < number_of_processes; i++)
    {
        pipe(p);
        pid = fork();
        if (pid < 0)
        {
            cout << "fork problem!\n";
            return;
        }
        if (pid == 0)
            execl("Worker", to_string(p[0]).c_str(), NULL);

        if (pid > 0)
        {
            int file_cnt = ratio + (i < remainded);
            pids.push_back(pid);
            write_for_worker(devolved_files, field_name, field_value,
                            files, file_cnt, p[1], dir);
        }
    }

//  presenter
    int fd;
    const char* myfifo = PIPE_NAME;
    mkfifo(myfifo, 0666);
    fd = open(myfifo, O_WRONLY | O_APPEND);
    pid = fork();
    if (pid < 0)
    {
        cout << "fork problem!\n";
        return;
    }

    if (pid == 0)
        execl("Presenter", PIPE_NAME);

    if (pid > 0)
    {
        vector<string> fields;
        string file = files[0];
        read_fields(fields, dir, file);
        pids.push_back(pid);
        write_for_presenter(sort_field, is_ascending, number_of_processes, fd, fields, number_of_processes);
    }

    wait_cpids(pids);
}

void get_files(string dir, vector<string> &files)
{
    DIR *dirp;
    struct dirent *entry;
    dirp = opendir(dir.c_str());

    if (!dirp)
    {
        cout << "Directory not found!\n" << endl;
        return;
    }
    while ((entry = readdir(dirp)) != NULL)
    {
        if (string(entry->d_name)[0] != '.')
            files.push_back(string(entry->d_name));
    }
    closedir(dirp);
}

int main()
{
    vector<string> field_name, field_value;
    vector<string> files;
    string sort_field, dir;
    int is_ascending = 0;
    int number_of_processes = 1;

    vector<pid_t> pids;

    while (1)
    {
        if (get_input(field_name, field_value, sort_field, dir, is_ascending, number_of_processes) != 0)
            continue;
        get_files(dir, files);
        pid_t cpid = fork();
        if (cpid < 0)
            cout << "fork problem!\n";

        if (cpid == 0)
        {
            division_of_labor(field_name, field_value, sort_field, dir, is_ascending, number_of_processes, files);
            exit(EXIT_SUCCESS);
        }

        if (cpid > 0)
        {
            pids.push_back(cpid);
            field_name.clear();
            field_value.clear();
            files.clear();
        }
    }
    wait_cpids(pids);
    exit(EXIT_SUCCESS);
}
