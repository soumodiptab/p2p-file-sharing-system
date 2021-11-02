#include "commons.h"
/**
 * When message arrives from peer ->
 * 1. Process input and segregate into commands [common input parser]
 * 2. Validator to validate command [validator_api_name]
 * 3. Process command and generate follow up:
 * 4. Acknowledgement / Action
 */
class User
{
public:
    string user_name;
    string password;
    string color_assignment;
    User() {}
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
    string listener_port;
};
class FileInfo
{
public:
    FileInfo() {}
    FileInfo(string file_name, string file_hash)
    {
        this->file_hash = file_hash;
        this->file_name = file_name;
    }
    string file_name;
    string file_hash;
    bool status;
    vector<string> block_hashes;
    vector<Peer> peer_list;
};
class Group
{
private:
    string name;
    string owner;
    set<string> members;
    vector<string> join_requests;
    /**
     * @brief <filehash,Information of File>
     * 
     */
    unordered_map<string, FileInfo> file_list;

public:
    Group() {}
    Group(string group_name, string username)
    {
        this->name = group_name;
        this->owner = username;
        members.insert(owner);
    }
    string get_owner()
    {
        return owner;
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
        string reply;
        if (join_requests.empty())
            reply = reply_group_no_pending;
        else if (username == owner)
            reply = reply_group_already_owner;
        else if (is_member(username))
            reply = reply_group_already_member;
        else
        {
            auto request_position = find(join_requests.begin(), join_requests.end(), username);
            if (request_position == join_requests.end())
                reply = reply_group_no_user_pending;
            else
            {
                members.insert(username);
                join_requests.erase(request_position);
                reply = reply_group_new_member;
            }
        }
        return reply;
    }
    string add_join_request(string username)
    {
        string reply;
        auto request_position = find(join_requests.begin(), join_requests.end(), username);
        if (request_position != join_requests.end())
            reply = reply_group_already_join;
        else if (members.find(username) != members.end())
        {
            reply = reply_group_already_member;
        }
        else
        {
            join_requests.push_back(username);
            reply = reply_group_join_added;
        }
        return reply;
    }
    string remove_request(string username)
    {
        string reply;
        auto request_position = find(join_requests.begin(), join_requests.end(), username);
        if (request_position == join_requests.end())
            reply = reply_group_not_member;
        else if (owner == username) //dont allow owner to leave group for now
            reply = reply_group_owner_not_leave;
        else
        {
            members.erase(username);
            reply = reply_group_leave_group;
        }
        return reply;
    }
    vector<string> get_pending_requests()
    {
        return join_requests;
    }
    unordered_map<string, FileInfo> get_files()
    {
        return file_list;
    }
    void add_file(string &hash_file, FileInfo &file)
    {
        file_list[hash_file] = file;
    }
};
/**
 * @brief <groupname,Group>
 * 
 */
unordered_map<string, Group> group_list;
/**
 * @brief <username,User>
 * 
 */
unordered_map<string, User> user_list;
/**
 * @brief <username,Peer>
 * 
 */
unordered_map<string, Peer> logged_user_list;
/**
 * @brief <thread,username>
 * 
 */
unordered_map<pthread_t, string> logged_user_threads;
/**
 * @brief <thread,Peer>
 * 
 */
unordered_map<pthread_t, Peer> peer_list;
int get_current_socket()
{
    return logged_user_list[logged_user_threads[pthread_self()]].socket_fd;
}
string color_picker()
{
    if (user_list.size() >= colors.size())
        return "\033[0m";
    else
        return colors[user_list.size()];
}
vector<string> create_user(vector<string> &tokens)
{
    string user_name = tokens[1];
    string password = tokens[2];
    vector<string> reply_tokens = {command_print};
    if (user_list.find(user_name) != user_list.end())
    {
        reply_tokens.push_back(reply_user_already_exists);
    }
    else
    {
        user_list[user_name] = User(user_name, password, "\033[34m");
        reply_tokens.push_back(reply_user_new_user);
    }
    return reply_tokens;
}
vector<string> login_user(vector<string> &tokens)
{
    string user_name = tokens[1];
    string password = tokens[2];
    vector<string> reply_tokens = {command_print};
    if (user_list.find(user_name) == user_list.end())
    {

        reply_tokens.push_back(reply_user_not_exist);
    }
    else if (logged_user_list.find(user_name) != logged_user_list.end())
    {
        if (logged_user_threads[pthread_self()] == user_name)
            reply_tokens.push_back(reply_user_already_login);
        else
            reply_tokens.push_back(reply_user_already_login + " at : " + logged_user_list[user_name].ip_address + " " + logged_user_list[user_name].port);
    }
    else if (password != user_list[user_name].password)
    {
        reply_tokens.push_back(reply_user_login_incorrect);
    }
    else
    {
        reply_tokens = {command_login};
        reply_tokens.push_back(user_list[user_name].color_assignment);
        reply_tokens.push_back(reply_user_login);
        logged_user_list[user_name] = peer_list[pthread_self()];
        peer_list[pthread_self()].user_name = user_name;
        logged_user_threads[pthread_self()] = user_name;
    }
    return reply_tokens;
}
vector<string> logout_user(vector<string> &tokens)
{
    vector<string> reply_tokens = {command_logout};
    reply_tokens.push_back("\033[0m");
    reply_tokens.push_back(reply_user_logout);
    logged_user_list.erase(logged_user_threads[pthread_self()]);
    logged_user_threads.erase(pthread_self());
    peer_list[pthread_self()].user_name.clear();
    return reply_tokens;
}
vector<string> create_group(vector<string> &tokens)
{
    vector<string> reply_tokens = {command_print};
    string group_name = tokens[1];
    if (group_list.find(group_name) != group_list.end())
        reply_tokens.push_back(reply_group_already_exists);
    else
    {
        group_list[group_name] = Group(group_name, logged_user_threads[pthread_self()]);
        reply_tokens.push_back(reply_group_created);
    }
    return reply_tokens;
}
vector<string> join_group(vector<string> &tokens)
{
    string group_name = tokens[1];
    vector<string> reply_tokens = {command_print};
    if (group_list.find(group_name) == group_list.end())
    {
        reply_tokens.push_back(reply_group_not_exits);
    }
    else
    {
        reply_tokens.push_back(group_list[group_name].add_join_request(logged_user_threads[pthread_self()]));
    }
    return reply_tokens;
}
vector<string> list_requests(vector<string> &tokens)
{
    string group_name = tokens[1];
    vector<string> reply_tokens = {command_print};
    if (group_list.find(group_name) == group_list.end())
    {
        reply_tokens.push_back(reply_group_not_exits);
    }
    else if (group_list[group_name].get_owner() != logged_user_threads[pthread_self()])
    {
        reply_tokens.push_back(reply_group_not_owner);
    }
    else
    {
        if (group_list[group_name].get_pending_requests().empty())
            reply_tokens.push_back(reply_group_no_pending);
        else
        {
            string reply_message = "Pending Requests:";
            for (auto req : group_list[group_name].get_pending_requests())
            {
                reply_message.append(" [" + req + "]");
            }
            reply_tokens.push_back(reply_message);
        }
    }
    return reply_tokens;
}
vector<string> accept_request(vector<string> &tokens)
{
    string group_name = tokens[1];
    string member = tokens[2];
    vector<string> reply_tokens = {command_print};
    if (group_list.find(group_name) == group_list.end())
    {
        reply_tokens.push_back(reply_group_not_exits);
    }
    else
    {
        reply_tokens.push_back(group_list[group_name].approve_join_request(member));
    }
    return reply_tokens;
}
vector<string> leave_group(vector<string> &tokens)
{
    string group_name = tokens[1];
    vector<string> reply_tokens = {command_print};
    if (group_list.find(group_name) == group_list.end())
    {
        reply_tokens.push_back(reply_group_not_exits);
    }
    else
    {
        reply_tokens.push_back(group_list[group_name].remove_request(logged_user_threads[pthread_self()]));
    }
    return reply_tokens;
}
vector<string> list_groups(vector<string> &tokens)
{
    vector<string> reply_tokens = {command_print};
    if (group_list.empty())
    {
        reply_tokens.push_back(reply_group_no_group);
    }
    else
    {
        string reply_message = "Groups on network: ";
        for (auto group : group_list)
        {
            reply_message.append(" [" + group.first + "]");
        }
        reply_tokens.push_back(reply_message);
    }
    return reply_tokens;
}
vector<string> upload_file(vector<string> &tokens)
{
    string file_path = tokens[1];
    string file_name = extract_file_name(file_path);
    string file_hash = generate_SHA1(file_name);
    string group_name = tokens[2];
    vector<string> reply_tokens = {command_upload_file};
    if (group_list.find(group_name) == group_list.end())
    {
        reply_tokens.push_back(to_string(false));
        reply_tokens.push_back(file_hash);
        reply_tokens.push_back(reply_group_not_exits);
    }
    else if (!group_list[group_name].is_member(logged_user_threads[pthread_self()]))
    {
        reply_tokens.push_back(to_string(false));
        reply_tokens.push_back(file_hash);
        reply_tokens.push_back(reply_group_not_member);
    }
    else if (group_list[group_name].get_files().find(file_hash) != group_list[group_name].get_files().end()) //file already present check integrity
    {
        reply_tokens = {command_upload_verify};
    }
    else // new file
    {
        reply_tokens = {command_upload_file, group_name, file_hash};
    }
    return reply_tokens;
}
void store_file_block_hash(vector<string> &tokens)
{
    string group_name = tokens[1];
    string file_hash = tokens[2];
    int blocks = stoi(socket_recieve(get_current_socket()));
    ack_send(get_current_socket());
    string file_name = socket_recieve(get_current_socket());
    ack_send(get_current_socket());
    FileInfo file = FileInfo();
    file.file_name = file_name;
    file.file_hash = file_hash;
    file.peer_list.push_back(peer_list[pthread_self()]);
    log("File:" + file_name + " File hash: " + file_hash);
    for (int i = 1; i <= blocks; i++)
    {
        string hash_block = socket_recieve(get_current_socket());
        ack_send(get_current_socket());
        file.block_hashes.push_back(hash_block);
        log("Block[" + to_string(i) + "]:" + hash_block);
    }
    ack_recieve(get_current_socket());
    group_list[group_name].add_file(file_hash, file);
    socket_send(get_current_socket(), reply_file_upload_complete);
}
vector<string> process(vector<string> &tokens)
{
    vector<string> reply = {command_print, reply_default};
    if (tokens.size() == 0)
        throw constants_socket_empty_reply;
    if (tokens[0] == command_create_user)
        reply = create_user(tokens);
    else if (tokens[0] == command_login)
        reply = login_user(tokens);
    else if (logged_user_threads.find(pthread_self()) == logged_user_threads.end())
        reply = {command_print, reply_user_not_login};
    else if (tokens[0] == command_logout)
        reply = logout_user(tokens);
    else if (tokens[0] == command_create_group)
        reply = create_group(tokens);
    else if (tokens[0] == command_join_group)
        reply = join_group(tokens);
    else if (tokens[0] == command_list_groups)
        reply = list_groups(tokens);
    else if (tokens[0] == command_list_requests)
        reply = list_requests(tokens);
    else if (tokens[0] == command_accept_request)
        reply = accept_request(tokens);
    else if (tokens[0] == command_leave_group)
        reply = leave_group(tokens);
    else if (tokens[0] == command_upload_file)
        reply = upload_file(tokens);
    return reply;
}
void post_process(vector<string> &tokens)
{
    if (tokens.empty())
        return;
    if (tokens[0] == command_upload_file && tokens.size() == 3)
    {
        store_file_block_hash(tokens);
    }
}
void *thread_service(void *socket_fd)
{
    int thread_socket_fd = *((int *)socket_fd);
    delete socket_fd;
    while (true)
    {
        try
        {
            string client_message = socket_recieve(thread_socket_fd);
            vector<string> client_message_tokens = unpack_message(client_message);
            log("REQ: " + client_message);
            vector<string> reply_tokens = process(client_message_tokens);
            string reply = pack_message(reply_tokens);
            log("REPLY: " + reply);
            socket_send(thread_socket_fd, reply);
            post_process(reply_tokens);
        }
        catch (string error)
        {
            log(error);
            break;
        }
    }
    peer_list.erase(pthread_self());
    close(thread_socket_fd);
}
/**
 * @brief Will implement later -> command line interface inside tracker[T.B.A.]
 *  Main features: show tracker statistics
 * This will be on another thread
 */
void shell_setup()
{
    cout << colors[0];
}
void start_tracker()
{
    int tracker_socket_fd = server_setup(tracker_1);
    while (true)
    {
        struct sockaddr_storage peer_address;
        socklen_t peer_addr_size = sizeof(peer_address);
        Peer new_peer = Peer();
        new_peer.socket_fd = accept(tracker_socket_fd, (struct sockaddr *)&peer_address, &peer_addr_size);
        if (new_peer.socket_fd == -1)
        {
            log("Unable to connect");
            continue;
        }
        struct sockaddr_in peer_address_cast = *((struct sockaddr_in *)&peer_address);
        new_peer.ip_address = inet_ntoa(peer_address_cast.sin_addr);
        new_peer.port = to_string(ntohs(peer_address_cast.sin_port));
        new_peer.listener_port = socket_recieve(new_peer.socket_fd);
        int *fd = new int;
        *fd = new_peer.socket_fd;
        pthread_t worker_thread;
        pthread_create(&worker_thread, NULL, thread_service, fd);
        peer_list[worker_thread] = new_peer;
        log("Connection to peer established :" + peer_list[worker_thread].ip_address + " " + peer_list[worker_thread].port);
    }
}
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        exit(1);
    }
    string file_path(argv[1]);
    logging_level = 3;
    set_log_file("tracker_log_file.txt");
    read_tracker_file(file_path);
    start_tracker();
    return 0;
}