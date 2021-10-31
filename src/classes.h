#include <bits/stdc++.h>
#include "constants.h"
using namespace std;
class User
{
public:
    string user_name;
    string password;
    string color_assignment;
    User()
    {
    }
    User(string user_name, string password, string color_assignment)
    {
        this->user_name = user_name;
        this->password = password;
        this->color_assignment = color_assignment;
    }
};
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
};
class File
{
    string file_name;
    string file_hash;
    string file_path;
    Peer *peer;
};
class Group
{
private:
    string name;
    string owner;
    unordered_set<string> members;
    vector<string> join_requests;

public:
    string create_group(string group_name, string username)
    {
        this->name = group_name;
        this->owner = username;
    }
    bool is_member(string username)
    {
        if (members.find(username) == members.end())
            return false;
        return true;
    }
    /**
     * @brief Approve join request Pre-req:- user has been authenticated
     * 
     * @param username 
     * @return string 
     */
    string approve_join_request(string username)
    {
        if (join_requests.empty())
            return reply_group_no_pending;
        if (username == owner)
            return reply_group_already_owner;
        if (is_member(username))
            return reply_group_already_member;
        auto request_position = find(join_requests.begin(), join_requests.end(), username);
        if (request_position == join_requests.end())
            return reply_group_no_user_pending;
        members.insert(username);
        join_requests.erase(request_position);
        return reply_group_new_member;
    }
    string add_join_request(string username)
    {
        auto request_position = find(join_requests.begin(), join_requests.end(), username);
        if (request_position != join_requests.end())
            return reply_group_already_join;
        join_requests.push_back(username);
        return reply_group_join_added;
    }
    vector<string> get_pending_requests()
    {
        return join_requests;
    }
};