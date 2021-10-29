#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

const string command_upload_file = "upload_file";
const string command_download_file = "download_file";
/**
 * @brief Print logs |
 *  0 - no output printing
 *  1 - only console
 *  2 - only logs
 *  3 - both
 */
const int logging_level = 1;
string log_file;
/**
 * @brief backlog connections to check from
 * 
 */
const int constants_connection_backlog = 3;
const string constants_socket_failure = "Failed to open socket";
const string constants_socket_binding_failure = "Failed to bind socket";
/**
 * @brief ip | port
 * 
 */
pair<string, string> tracker_1;
pair<string, string> tracker_2;
void read_tracker_file(string path)
{
    string temp;
    ifstream file(path);
    getline(file, temp);
    tracker_1.first = temp.substr(0, temp.find(":"));
    tracker_1.second = temp.substr(temp.find(":") + 1);
}
string path_processor(string path)
{
    string x;
    return x;
}
void write_to_log(const std::string &message)
{
    ofstream file_out;
    file_out.open(log_file, std::ios_base::app);
    file_out << message << endl;
}
void log(string message)
{
    switch (logging_level)
    {
    case 0:
        break;
    case 2:
        write_to_log(message);
        break;
    case 3:
        write_to_log(message);
    default:
        cout << message << endl;
    }
}
void set_log_file(string path)
{
    log_file = path;
}
/**
 * @brief  calling this api turns the tracker/peer into a server
 * 
 * @return socket file descriptor
 */
int server_setup(pair<string, string>)
{
}
/**
 * @brief calling this api turns the tracker/peer into a client
 * 
 */
int client_setup(pair<string, string>)
{
}