#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include "constants.h"
using namespace std;
/**
 * @brief Header file to provide library for:
 * 1. socket connectivity using wrappers
 * 2. logging in console/logfile
 */
/**
 * @brief ip_addess | port | user_name | socket
 * 
 */
class Peer
{
public:
    string ip_address;
    string port;
    string user_name;
    int socket_fd;
    string listener_port;
};
/**
 * @brief ip | port
 * 
 */
pair<string, string> tracker_1;
pair<string, string> tracker_2;

//-----------------------------------------------------------------------------------

vector<string> colors = {
    "\033[34m",
    "\033[31m",
    "\033[36m",
    "\033[32m",
    "\033[33m",
    "\033[35m",
};
pthread_mutex_t log_mutex;
pthread_mutex_t console_mutex;
/**
 * @brief Print logs |
 *  0 - no output printing
 *  1 - only console
 *  2 - only logs
 *  3 - both
 */
int logging_level = 1;
string log_file;
/**
 * @brief Extracts the ip and port from the input string
 * 
 * @param ip_port 
 * @return pair<string, string> 
 */
pair<string, string> read_socket_input(string ip_port)
{
    pair<string, string> socket_pair;
    socket_pair.first = ip_port.substr(0, ip_port.find(":"));
    socket_pair.second = ip_port.substr(ip_port.find(":") + 1);
    return socket_pair;
}
/**
 * @brief Reads the tracker file and extracts the tracker ip_port
 * 
 * @param path 
 */
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
void sync_print_ln(string message)
{
    pthread_mutex_lock(&console_mutex);
    cout << message << endl;
    pthread_mutex_unlock(&console_mutex);
}
void sync_print(string message)
{
    pthread_mutex_lock(&console_mutex);
    cout << message;
    pthread_mutex_unlock(&console_mutex);
}
/**
 * @brief Writes to  a log file with timestamp and thread-id
 * 
 * @param message 
 */
void write_to_log(const std::string &message)
{
    pthread_mutex_lock(&log_mutex);
    ofstream file_out;
    file_out.open(log_file, std::ios_base::app);
    file_out << message << endl;
    pthread_mutex_unlock(&log_mutex);
}

/**
 * @brief Selects between displaying log on console/log file
 * 
 * @param message 
 */
void log(string message)
{
    pthread_t self = pthread_self();
    time_t now = time(0);
    string date_time = ctime(&now);
    date_time.pop_back();
    string final_message = "[" + date_time + "] " + "[" + to_string(self) + "][ " + message + " ]";
    switch (logging_level)
    {
    case 0:
        break;
    case 2:
        write_to_log(final_message);
        break;
    case 3:
        write_to_log(final_message);
    default:
        sync_print_ln(final_message);
    }
}
void set_log_file(string path)
{
    log_file = path;
}
/**
 * @brief  calling this api turns the tracker/peer into a server
 * @param pair<ip_address,port> : own ip and port
 * @return socket file descriptor
 */
int server_setup(pair<string, string> socket_pair)
{
    struct addrinfo query;
    struct addrinfo *socket_addr;
    memset(&query, 0, sizeof(query));
    query.ai_socktype = SOCK_STREAM; // Tcp connection protocol
    query.ai_family = AF_INET;       // Ipv4 address family
    //it queries the sockets and gets a list of socket address structures with protocol,family type specified and stores it in the socket address pointer
    if (getaddrinfo(socket_pair.first.c_str(), socket_pair.second.c_str(), &query, &socket_addr) != 0)
    {
        log(constants_socket_failure + " : " + socket_pair.first + " " + socket_pair.second);
        return -1;
    }
    int socket_file;
    if ((socket_file = socket(socket_addr->ai_family, socket_addr->ai_socktype, socket_addr->ai_protocol)) < 1)
    {
        log(constants_socket_failure + " : " + socket_pair.first + " " + socket_pair.second);
        return -1;
    }
    //Clear the pointer
    //returns -1 on error
    if (bind(socket_file, socket_addr->ai_addr, socket_addr->ai_addrlen) == -1)
    {
        log(constants_socket_binding_failure + " : " + socket_pair.first + " " + socket_pair.second);
        return -1;
    }
    //Marks the socket as a passive socket<<recieve incoming connections
    if (listen(socket_file, constants_connection_backlog) == -1)
    {
        log(constants_socket_listen_failure + " : " + socket_pair.first + " " + socket_pair.second);
        return -1;
    }
    log(constants_socket_listen_success + ":" + socket_pair.first + " " + socket_pair.second);
    freeaddrinfo(socket_addr);
    return socket_file;
}

/**
 * @brief calling this api turns the tracker/peer into a client
 * @param pair<ip_address,port> : ip  and port of reciever(server)
 * @return socket file descriptor
 */
int client_setup(pair<string, string> socket_pair)
{
    struct addrinfo query;
    struct addrinfo *socket_addr;
    memset(&query, 0, sizeof(query));
    query.ai_socktype = SOCK_STREAM; // Tcp connection protocol
    query.ai_family = AF_INET;       // Ipv4 address family
    if (getaddrinfo(socket_pair.first.c_str(), socket_pair.second.c_str(), &query, &socket_addr) != 0)
    {
        log(constants_socket_failure + " : " + socket_pair.first + " " + socket_pair.second);
        return -1;
    }
    int socket_file;
    socket_file = socket(socket_addr->ai_family, socket_addr->ai_socktype, socket_addr->ai_protocol);

    //Opens a connection to the socket
    if (connect(socket_file, socket_addr->ai_addr, socket_addr->ai_addrlen) == -1)
    {
        close(socket_file);
        log(constants_socket_conn_failure + " : " + socket_pair.first + " " + socket_pair.second);
        return -1;
    }
    log(constants_socket_connected_success + ":" + socket_pair.first + " " + socket_pair.second);
    freeaddrinfo(socket_addr);
    return socket_file;
}
/**
 * @brief Sends a message to the opposite end 
 * 
 * @param socket_fd
 * @param message 
 * @return status
 * @throws exception incase unable to send due to socket closure
 */
int socket_send(int socket_fd, const string &message)
{
    if (send(socket_fd, message.c_str(), message.size(), 0) == -1)
    {
        log(constants_socket_send_failure);
        throw(constants_socket_send_failure);
    }
    return 0;
}
/**
 * @brief Waits to recieve message from the socket
 * 
 * @param socket_fd 
 * @return string 
 * @throws exception incase client forcefully disconnects
 */
string socket_recieve(int socket_fd)
{
    char buff[constants_message_buffer_limit];
    int bytes_recieved;
    if ((bytes_recieved = recv(socket_fd, buff, constants_message_buffer_limit, 0)) == -1)
    {
        log(constants_socket_recv_failure);
        throw(constants_socket_recv_failure);
    }
    else if (bytes_recieved == 0)
    {
        throw(constants_client_disconnected);
    }
    string message(buff);
    string extract = message.substr(0, bytes_recieved);
    return extract;
}
/**
 * @brief ACKNOWLEDGEMENT Sending to synchronize socket transfer
 * 
 * @param socket_fd 
 * @return int 
 */
int ack_send(int socket_fd)
{
    string ack = reply_ACK;
    return socket_send(socket_fd, ack);
}
int nack_send(int socket_fd)
{
    string ack = reply_NACK;
    return socket_send(socket_fd, ack);
}
/**
 * @brief ACKNOWLEDGEMENT Recieve to synchronize socket transfer
 * 
 * @param socket_fd 
 * @return string 
 */
string ack_recieve(int socket_fd)
{
    return socket_recieve(socket_fd);
}
/**
 * @brief Parses command input and convert into tokens
 * 
 * @param input 
 * @return vector<string> 
 */
vector<string> input_parser(string &input)
{
    vector<string> tokens;
    string temp_str = "";
    for (char c : input)
    {
        if (c == ' ')
        {
            tokens.push_back(temp_str);
            temp_str = "";
        }
        else
        {
            temp_str += c;
        }
    }
    if (!temp_str.empty())
        tokens.push_back(temp_str);
    return tokens;
}
/**
 * @brief Packs message into a suitable format to be sent over socket
 * 
 * @param tokens 
 * @return string 
 */
string pack_message(vector<string> &tokens)
{
    string message = "";
    for (auto t : tokens)
    {
        message.append("|");
        message.append(t);
    }
    return message;
}
/**
 * @brief Unpacks message at reciever end
 * 
 * @param packed_message 
 * @return vector<string> 
 */
vector<string> unpack_message(string &packed_message)
{
    vector<string> tokens;
    string temp_str = "";
    for (char c : packed_message)
    {
        if (c == '|')
        {
            if (!temp_str.empty())
                tokens.push_back(temp_str);
            temp_str = "";
        }
        else
        {
            temp_str += c;
        }
    }
    if (!temp_str.empty())
        tokens.push_back(temp_str);
    return tokens;
}
bool file_query(string path)
{
    struct stat entity;
    if (stat(path.c_str(), &entity) == -1)
    {
        return false;
    }
    if (!S_ISREG(entity.st_mode))
    {
        return false;
    }
    return true;
}
int get_file_size(string path)
{
    struct stat entity;
    if (stat(path.c_str(), &entity) == -1)
    {
        return -1;
    }
    return true;
}
string generate_SHA1(const char *target, int size)
{
    unsigned char temp[SHA_DIGEST_LENGTH];
    char buffer[SHA_DIGEST_LENGTH * 2];
    memset(temp, 0x0, SHA_DIGEST_LENGTH);
    memset(buffer, 0x0, SHA_DIGEST_LENGTH * 2);
    SHA1((unsigned char *)target, size, temp);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        sprintf((char *)&(buffer[i * 2]), "%02x", temp[i]);
    }
    string temp_hash(buffer);
    return temp_hash;
}
string generate_SHA1(string message)
{
    return generate_SHA1(message.c_str(), message.size());
}
string extract_file_name(string &path)
{
    string file_name = path.substr(path.find_last_of('/') + 1, path.size() - path.find_last_of('/') + 1);
    return file_name;
}