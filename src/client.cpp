#include "commons.h"
pair<string, string> client_socket_listener;
unordered_map<pthread_t, Peer> peer_list;
pthread_t listener_thread;
int client_fd;
bool user_logged_in = false;
class FileInfo
{
public:
    string file_name;
    string file_hash;
    string path;
    int file_descriptor;
    struct stat file_stat;
    unsigned long long size;
    vector<pair<bool, string>> integrity;
    int blocks;
    pthread_mutex_t file_sync;
    FileInfo(){};
    FileInfo(string path)
    {
        this->file_name = extract_file_name(path);
        this->path = path;
        file_hash = generate_SHA1(file_name);
    }
    FileInfo(string path, int blocks)
    {
        this->file_name = extract_file_name(path);
        this->path = path;
        integrity = vector<pair<bool, string>>(blocks, make_pair(0, ""));
        this->blocks = blocks;
    }
    int open_file(int flag)
    {
        file_descriptor = open(path.c_str(), flag);
        return file_descriptor;
    }
    int close_file()
    {
        close(file_descriptor);
        return 0;
    }
    void file_hash_generation()
    {
        integrity.clear();
        size = file_stat.st_size;
        int last_block_size = size;
        blocks = size / constants_file_block_size;
        if (size % constants_file_block_size)
        {
            blocks += 1;
            last_block_size = size % constants_file_block_size;
        }
        log("Generated hash:");
        for (int i = 1; i < blocks; i++)
        {
            char buffer[constants_file_block_size];
            read(file_descriptor, buffer, constants_file_block_size);
            string gen_hash = generate_SHA1(buffer, constants_file_block_size);
            log("block[" + to_string(i) + "] :" + gen_hash);
            integrity.push_back(make_pair(1, gen_hash));
        }
        char buffer[last_block_size];
        read(file_descriptor, buffer, last_block_size);
        string gen_hash = generate_SHA1(buffer, last_block_size);
        integrity.push_back(make_pair(1, gen_hash));
        log("block[" + to_string(blocks) + "] :" + gen_hash);
    }
    string get_bit_vector()
    {
        pthread_mutex_lock(&file_sync);
        string bit_vector = "";
        for (auto i : integrity)
        {
            bit_vector.append(to_string((int)i.first));
        }
        return bit_vector;
        pthread_mutex_unlock(&file_sync);
    }
    string get_hash(int block_id)
    {
        return integrity[block_id].second;
    }
    void set_hash(int block_id, char *buffer, int size)
    {
        pthread_mutex_lock(&file_sync);
        integrity[block_id].first = 1;
        integrity[block_id].second = generate_SHA1(buffer, size);
        pthread_mutex_unlock(&file_sync);
    }
};
/**
 * @brief <file_hash,FileInfo>
 * 
 */
unordered_map<string, FileInfo> hosted_files;
unordered_map<string, FileInfo> downloads;
bool file_uploader(vector<string> &tokens)
{
    string path = tokens[1];
    if (!file_query(path))
    {
        sync_print_ln("|| Incorrect file path provided");
        return false;
    }
    FileInfo file = FileInfo(path);
    int file_descriptor = file.open_file(O_RDONLY);
    if (file_descriptor == -1)
    {
        sync_print_ln("|| Error in opening file");
        log("Error in opening file");
        return false;
    }
    struct stat file_stats;
    if (fstat(file_descriptor, &file_stats) == -1)
    {
        sync_print_ln("|| Error fetching file stats");
        log("Error fetching file stats");
    }
    file.file_stat = file_stats;
    file.file_hash_generation();
    hosted_files[file.file_hash] = file;
    return true;
}
bool send_file_block_hash(int socket_fd, string file_hash)
{
    if (hosted_files.find(file_hash) == hosted_files.end())
        return false;
    FileInfo file = hosted_files[file_hash];
    socket_send(socket_fd, to_string(file.blocks));
    ack_recieve(socket_fd);
    socket_send(socket_fd, file.file_name);
    ack_recieve(socket_fd);
    for (auto i : file.integrity)
    {
        socket_send(socket_fd, i.second);
        ack_recieve(socket_fd);
    }
    ack_send(socket_fd);
}
void file_upload_send(int socket_fd, string file_hash)
{
    send_file_block_hash(socket_fd, file_hash);
    sync_print_ln(">>" + socket_recieve(client_fd));
}
void file_upload_verify_send(int socket_fd, string file_hash)
{
    send_file_block_hash(socket_fd,file_hash);
    if (ack_recieve(socket_fd)==reply_NACK)
    {
        hosted_files.erase(file_hash);
    }
    ack_send(socket_fd);
    sync_print_ln(">>" + socket_recieve(socket_fd));
}
void show_downloads()
{
    if (!user_logged_in)
    {
        sync_print_ln("|| User is not logged in.");
        return;
    }
    if (!downloads.empty() && !hosted_files.empty())
    {
        for (auto h : hosted_files)
        {
            sync_print_ln("");
        }
    }
}
bool file_download_pre_verification(vector<string> &tokens)
{
}
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
    else if (tokens[0] == command_upload_file && tokens.size() == 3)
        return file_uploader(tokens);
    else if (tokens[0] == command_list_files && tokens.size() == 2)
        return true;
    else if (tokens[0] == command_stop_share && tokens.size() == 3)
        return true;
    else if (tokens[0] == command_show_downloads && tokens.size() == 1)
        return true;
    else if (tokens[0] == command_download_file && tokens.size() == 4)
        return file_download_pre_verification(tokens);
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
    else if (tokens[0] == command_login && tokens.size() == 3)
    {
        sync_print(tokens[1]);
        sync_print_ln(">>" + tokens[2]);
        user_logged_in = true;
    }
    else if (tokens[0] == command_logout && tokens.size() == 3)
    {
        sync_print(tokens[1]);
        sync_print_ln(">>" + tokens[2]);
        user_logged_in = false;
    }
    else if (tokens[0] == command_upload_file)
    {
        if (tokens.size() == 3)
        {
            file_upload_send(client_fd, tokens[2]);
        }
        else if (tokens.size() == 4)
        {
            if (!stoi(tokens[1]))
            {
                hosted_files.erase(tokens[2]);
                sync_print_ln(">>" + tokens[3]);
            }
        }
    }
    else if (tokens[0] == command_upload_verify && tokens.size() == 3)
    {
        file_upload_verify_send(client_fd, tokens[2]);
    }
}
void client_startup()
{
    sleep(1);
    if ((client_fd = client_setup(tracker_1)) == -1)
    {
        log("Could not connect to Tracker [" + tracker_1.first + ":" + tracker_1.second + "]");
        exit(EXIT_FAILURE);
    }
    socket_send(client_fd, client_socket_listener.second);
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
void process(vector<string> &tokens, Peer &peer)
{
    pthread_t worker_thread;

    log("Connection to peer established :" + peer_list[worker_thread].ip_address + " " + peer_list[worker_thread].port);
}
void *listener_startup(void *)
{
    int listener_fd = server_setup(client_socket_listener);
    while (true)
    {
        struct sockaddr_storage peer_address;
        socklen_t peer_addr_size = sizeof(peer_address);
        Peer new_peer = Peer();
        new_peer.socket_fd = accept(listener_fd, (struct sockaddr *)&peer_address, &peer_addr_size);
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

        string command_message = socket_recieve(new_peer.socket_fd);
        vector<string> command_message_tokens = unpack_message(command_message);
        process(command_message_tokens, new_peer);
    }
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
    client_socket_listener = read_socket_input(socket_input);
    pthread_create(&listener_thread, NULL, listener_startup, NULL);
    client_startup();
    return 0;
}