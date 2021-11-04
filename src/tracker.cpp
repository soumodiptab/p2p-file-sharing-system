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
    unordered_set<string> group_memberships;
    User() {}
    User(string user_name, string password, string color_assignment)
    {
        this->user_name = user_name;
        this->password = password;
        this->color_assignment = color_assignment;
    }
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
    vector<string> block_hashes;
    vector<string> usernames;
};
class Download
{
public:
    string file_name;
    string file_hash;
    string group_name;
    string target_path;
    string master_user;
    vector<string> slave_users;
};
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
unordered_map<pthread_t, Download> ongoing_downloads;
pthread_mutex_t download_mutex;
bool is_user_logged_in(string username)
{
    if (logged_user_list.find(username) == logged_user_list.end())
    {
        return false;
    }
    return true;
}
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
    unordered_map<string, FileInfo> &get_files()
    {
        return file_list;
    }
    void add_file(string &hash_file, FileInfo &file)
    {
        file_list[hash_file] = file;
    }
    vector<string> find_uploaders_online(string file_hash)
    {
        vector<string> final_uploaders_list;
        for (auto user : file_list[file_hash].usernames)
        {
            if (is_user_logged_in(user))
                final_uploaders_list.push_back(user);
        }
        return final_uploaders_list;
    }
    bool is_uploader_online(string filehash)
    {
        vector<string> uploaders = find_uploaders_online(filehash);
        if (uploaders.empty())
            return false;
        return true;
    }
    string fetch_files()
    {
        string reply;
        if (file_list.empty())
        {
            reply = reply_group_file_list_empty;
        }
        else
        {
            reply = "Files Available:";
            int c = 0;
            for (auto f : file_list)
            {
                if (is_uploader_online(f.first))
                {
                    reply.append(" [" + f.second.file_name + "]");
                    c++;
                }
            }
            if (c == 0)
                reply = reply_group_file_list_empty;
        }
        return reply;
    }
};
/**
 * @brief <groupname,Group>
 * 
 */
map<string, Group> group_list;

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
vector<string> list_files(vector<string> &tokens)
{
    string group_name = tokens[1];
    vector<string> reply_tokens = {command_print};
    if (group_list.empty())
    {
        reply_tokens.push_back(reply_group_no_group);
    }
    if (group_list.find(group_name) == group_list.end())
    {
        reply_tokens.push_back(reply_group_not_exits);
    }
    else
    {
        reply_tokens.push_back(group_list[group_name].fetch_files());
    }
    return reply_tokens;
}
vector<string> stop_share(vector<string> &tokens)
{
    string group_name = tokens[1];
    string file_name = tokens[2];
    string file_hash = generate_SHA1(file_name);
    vector<string> reply_tokens = {command_print};
    if (group_list.empty())
    {
        reply_tokens.push_back(reply_group_no_group);
    }
    if (group_list.find(group_name) == group_list.end())
    {
        reply_tokens.push_back(reply_group_not_exits);
    }
    else
    {
        reply_tokens.push_back(group_list[group_name].fetch_files());
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
        reply_tokens = {command_upload_verify, group_name, file_hash};
    }
    else // new file
    {
        reply_tokens = {command_upload_file, group_name, file_hash};
    }
    return reply_tokens;
}
vector<string> download_file_verification(vector<string> &tokens)
{
    string group_name = tokens[1];
    string file_name = tokens[2];
    string path = tokens[3];
    string file_hash = generate_SHA1(file_name);
    vector<string> reply_tokens;
    if (group_list.find(group_name) == group_list.end())
    {
        reply_tokens = {command_print, reply_group_not_exits};
    }
    else if (!group_list[group_name].is_member(logged_user_threads[pthread_self()]))
    {
        reply_tokens = {command_print, reply_group_not_member};
    }
    else if (group_list[group_name].get_files().find(file_hash) == group_list[group_name].get_files().end()) //file does not exist
    {
        reply_tokens = {command_print, reply_file_download_file_not_exists};
    }
    else if (!group_list[group_name].is_uploader_online(file_hash)) //file is offline(user logged out)
    {
        reply_tokens = {command_print, reply_file_download_file_uploader_offline};
    }
    else
    {
        reply_tokens = {command_download_file, file_name, group_name, file_hash, path, reply_file_download_started};
    }
    return reply_tokens;
}
FileInfo store_file_block_hash(vector<string> &tokens)
{
    string file_hash = tokens[2];
    int blocks = stoi(socket_recieve(get_current_socket()));
    ack_send(get_current_socket());
    string file_name = socket_recieve(get_current_socket());
    ack_send(get_current_socket());
    FileInfo file = FileInfo();
    file.file_name = file_name;
    file.file_hash = file_hash;
    file.usernames.push_back(logged_user_threads[pthread_self()]);
    log("File:" + file_name + " File hash: " + file_hash);
    for (int i = 1; i <= blocks; i++)
    {
        string hash_block = socket_recieve(get_current_socket());
        ack_send(get_current_socket());
        file.block_hashes.push_back(hash_block);
        log("Block[" + to_string(i) + "]:" + hash_block);
    }
    ack_recieve(get_current_socket());
    return file;
}
void upload_verify(vector<string> &tokens)
{
    string group_name = tokens[1];
    FileInfo file = store_file_block_hash(tokens);
    FileInfo file_stored = group_list[group_name].get_files()[file.file_hash];
    bool flag = true;
    if (file.block_hashes.size() == file_stored.block_hashes.size())
    {
        for (int i = 0; i < file.block_hashes.size(); i++)
        {
            if (file.block_hashes[i] != file_stored.block_hashes[i])
            {
                flag = false;
                break;
            }
        }
    }
    else
    {
        flag = false;
    }
    string reply;
    if (flag)
    {
        group_list[group_name].get_files()[file.file_hash].usernames.push_back(logged_user_threads[pthread_self()]);
        reply = reply_file_upload_recon_success;
        ack_send(get_current_socket());
    }
    else
    {
        reply = reply_file_upload_recon_error;
        nack_send(get_current_socket());
    }
    ack_recieve(get_current_socket());
    socket_send(get_current_socket(), reply);
}
void upload_process(vector<string> &tokens)
{
    string group_name = tokens[1];
    FileInfo file = store_file_block_hash(tokens);
    group_list[group_name].add_file(file.file_hash, file);
    socket_send(get_current_socket(), reply_file_upload_complete);
}
void *download_service(void *)
{
    pthread_mutex_lock(&download_mutex);
    pthread_mutex_unlock(&download_mutex);
    Download &download = ongoing_downloads[pthread_self()];
    Peer target = logged_user_list[download.master_user];
    int target_user_fd = client_setup(make_pair(target.ip_address, target.listener_port));
    vector<string> message_tokens = {command_download_init};
    message_tokens.push_back(download.file_name);
    message_tokens.push_back(download.file_hash);
    message_tokens.push_back(download.target_path);
    message_tokens.push_back(to_string(download.slave_users.size()));
    for (auto u : download.slave_users)
    {
        Peer other_peer = logged_user_list[u];
        message_tokens.push_back(other_peer.ip_address + ":" + other_peer.listener_port);
    }
    string message = pack_message(message_tokens);
    socket_send(target_user_fd, message);
    log("Sent DOWNLOAD REQ: " + message);
    string reply = socket_recieve(target_user_fd);
    log(reply);
    if (reply == reply_download_status_SUCCESS)
    {
        string group_name = download.group_name;
        group_list[group_name].get_files()[download.file_hash].usernames.push_back(download.master_user);
    }
    ongoing_downloads.erase(pthread_self());
    close(target_user_fd);
}
void download_process(vector<string> &tokens)
{
    string group_name = tokens[2];
    string file_hash = tokens[3];
    vector<string> users_with_file = group_list[group_name].find_uploaders_online(file_hash);
    string user_download = logged_user_threads[pthread_self()];
    Download new_download = Download();
    new_download.file_hash = file_hash;
    new_download.file_name = tokens[1];
    new_download.group_name = group_name;
    new_download.master_user = user_download;
    new_download.target_path = tokens[4];
    new_download.slave_users = users_with_file;
    pthread_t new_download_thread;
    pthread_mutex_lock(&download_mutex);
    pthread_create(&new_download_thread, NULL, download_service, NULL);
    log("Download request  has been sent for file : " + new_download.file_hash + "and user : " + new_download.master_user);
    ongoing_downloads[new_download_thread] = new_download;
    pthread_mutex_unlock(&download_mutex);
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
    else if (tokens[0] == command_list_files)
        reply = list_files(tokens);
    else if (tokens[0] == command_stop_share)
        reply = stop_share(tokens);
    else if (tokens[0] == command_download_file)
        reply = download_file_verification(tokens);
    return reply;
}
void post_process(vector<string> &tokens)
{
    if (tokens.empty())
        return;
    if (tokens[0] == command_upload_file && tokens.size() == 3)
    {
        upload_process(tokens);
    }
    if (tokens[0] == command_upload_verify && tokens.size() == 3)
    {
        upload_verify(tokens);
    }
    if (tokens[0] == command_download_file && tokens.size() == 6)
    {
        download_process(tokens);
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