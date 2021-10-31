#include "commons.h"
/**
 * When message arrives from peer ->
 * 1. Process input and segregate into commands [common input parser]
 * 2. Validator to validate command [validator_api_name]
 * 3. Process command and generate follow up:
 * 4. Acknowledgement / Action
 */

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
unordered_map<pthread_t, string> logged_user_threads;
/**
 * @brief <thread,Peer>
 * 
 */
unordered_map<pthread_t, Peer> peer_list;
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
        reply_tokens[0] = command_login;
        reply_tokens.push_back(user_list[user_name].color_assignment);
        reply_tokens.push_back(reply_user_login);
        logged_user_list[user_name] = peer_list[pthread_self()];
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
    return reply_tokens;
}
vector<string> create_group(vector<string> &tokens)
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
        reply_tokens.push_back(reply_user_already_login + ": " + logged_user_list[user_name].ip_address + " " + logged_user_list[user_name].port);
    }
    else
    {
        reply_tokens[0] = command_login;
        reply_tokens.push_back(user_list[user_name].color_assignment);
        reply_tokens.push_back(reply_user_login);
        logged_user_list[user_name] = peer_list[pthread_self()];
    }
    return reply_tokens;
}
vector<string> join_group(vector<string> &tokens)
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
        reply_tokens.push_back(reply_user_already_login + ": " + logged_user_list[user_name].ip_address + " " + logged_user_list[user_name].port);
    }
    else
    {
        reply_tokens[0] = command_login;
        reply_tokens.push_back(user_list[user_name].color_assignment);
        reply_tokens.push_back(reply_user_login);
        logged_user_list[user_name] = peer_list[pthread_self()];
    }
    return reply_tokens;
}
vector<string> process(vector<string> &tokens)
{
    vector<string> reply = {command_print, reply_default};
    if (tokens.size() == 0)
    {
        throw constants_socket_empty_reply;
    }
    if (tokens[0] == command_create_user)
    {
        reply = create_user(tokens);
    }
    else if (tokens[0] == command_login)
    {
        reply = login_user(tokens);
    }
    else if (logged_user_threads.find(pthread_self()) == logged_user_threads.end())
    {
        reply = {command_print, reply_user_not_login};
    }
    else if (tokens[0] == command_logout)
    {
        reply = logout_user(tokens);
    }
    return reply;
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
            log(client_message);
            vector<string> reply_tokens = process(client_message_tokens);
            string reply = pack_message(reply_tokens);
            socket_send(thread_socket_fd, reply);
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