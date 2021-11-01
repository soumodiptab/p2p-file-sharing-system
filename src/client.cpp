#include "commons.h"
pair<string, string> client_socket;
pthread_t listener_thread;
bool validator(vector<string> tokens)
{
    if (tokens.size() == 0)
        return false;
    if (tokens[0] == command_create_user && tokens.size() == 3)
        return true;
    else if (tokens[0] == command_login && tokens.size() == 3)
        return true;
    else if (tokens[0] == command_create_group && tokens.size() == 2)
        return true;
    else if (tokens[0] == command_join_group && tokens.size() == 2)
        return true;
    else if (tokens[0] == command_logout && tokens.size() == 1)
        return true;
    else if (tokens[0] == command_list_groups && tokens.size() == 1)
        return true;
    else if (tokens[0] == command_list_requests && tokens.size() == 2)
        return true;
    else if (tokens[0] == command_accept_request && tokens.size() == 3)
        return true;
    else if (tokens[0] == command_leave_group && tokens.size() == 2)
        return true;
    else
    {
        sync_print_ln("||Invalid command");
        return false;
    }
}
void action(vector<string> tokens)
{
    if (tokens.size() == 0)
    {
        throw constants_socket_empty_reply;
    }
    if (tokens[0] == command_print && tokens.size() == 2)
    {
        sync_print_ln(">>" + tokens[1]);
    }
    if ((tokens[0] == command_login || tokens[0] == command_logout) && tokens.size() == 3)
    {
        sync_print(tokens[1]);
        sync_print_ln(">>" + tokens[2]);
    }
}
void client_startup()
{
    int client_fd;
    if ((client_fd = client_setup(tracker_1)) == -1)
    {
        log("Could not connect to Tracker [" + tracker_1.first + ":" + tracker_1.second + "]");
        exit(EXIT_FAILURE);
    }
    socket_send(client_fd, client_socket.second);
    while (true)
    {
        try
        {
            string command_buffer;
            sync_print("<<");
            fflush(stdout);
            getline(cin, command_buffer);
            vector<string> tokens = input_parser(command_buffer);
            if (!validator(tokens))
            {
                continue;
            }
            string message = pack_message(tokens);
            socket_send(client_fd, message);
            string reply = socket_recieve(client_fd);
            vector<string> reply_tokens = unpack_message(reply);
            action(reply_tokens);
        }
        catch (string error)
        {
            sync_print_ln("Exiting client");
            log(error);
            break;
        }
    }
    close(client_fd);
}
void *listener_startup(void *)
{
    int listener_fd = server_setup(client_socket);
    close(listener_fd);
}
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        exit(1);
    }
    string file_path(argv[2]);
    string socket_input(argv[1]);
    logging_level = 3;
    set_log_file("client_log_file.txt");
    read_tracker_file(file_path);
    client_socket = read_socket_input(socket_input);
    //pthread_create(&listener_thread, NULL, listener_startup, NULL);
    client_startup();
    return 0;
}