#include "commons.h"
pair<string, string> client_socket_listener;
unordered_map<pthread_t, Peer> peer_list;
pthread_t listener_thread;
int client_fd;
bool user_logged_in = false;
typedef struct T
{
    Peer peer;
    vector<string> tokens;
} ThreadInfo;
pthread_mutex_t thread_info_maintainer;
class FileInfo
{
public:
    string file_name;
    string file_hash;
    string path;
    struct stat file_stat;
    unsigned int size;
    vector<pair<bool, string>> integrity;
    vector<Peer> peer_list;
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
    void file_hash_generation()
    {
        FILE *file_descriptor = fopen(path.c_str(), "r+");
        integrity.clear();
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
            fread(buffer, constants_file_block_size, constants_file_block_size, file_descriptor);
            string gen_hash = generate_SHA1(buffer, constants_file_block_size);
            log("block[" + to_string(i) + "] :" + gen_hash);
            integrity.push_back(make_pair(1, gen_hash));
        }
        char buffer[last_block_size];
        fread(buffer, constants_file_block_size, constants_file_block_size, file_descriptor);
        string gen_hash = generate_SHA1(buffer, last_block_size);
        integrity.push_back(make_pair(1, gen_hash));
        log("block[" + to_string(blocks) + "] :" + gen_hash);
        fclose(file_descriptor);
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
    int get_integrity()
    {
        pthread_mutex_lock(&file_sync);
        int value = 0;
        for (auto b : integrity)
        {
            if (b.first)
                value++;
        }
        pthread_mutex_unlock(&file_sync);
        return value;
    }
    bool check_integrity()
    {
        if (get_integrity() == blocks)
            return true;
        return false;
    }
    bool integrity_reconciliation(vector<string> hashes)
    {
        if (hashes.size() != blocks)
            return false;
        for (int i = 0; i < blocks; i++)
        {
            if (hashes[i] != integrity[i].second)
                return false;
        }
        return true;
    }
};
class Download
{
public:
    FileInfo file_downloading;
    FileInfo file_peer;
    vector<Peer> peer_list;
};
/**
 * @brief <file_hash,FileInfo>
 * 
 */
unordered_map<string, FileInfo> hosted_files;
unordered_map<string, Download> downloads;
bool file_uploader(vector<string> &tokens)
{
    string path = tokens[1];
    if (!file_query(path))
    {
        sync_print_ln("|| Incorrect file path provided");
        return false;
    }
    FileInfo file = FileInfo(path);
    int file_descriptor = open(path.c_str(), O_RDONLY);
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
    file.size = file_stats.st_size;
    file.file_hash_generation();
    hosted_files[file.file_hash] = file;
    return true;
}
void send_file_block_hash(int socket_fd, string file_hash)
{
    FileInfo file = hosted_files[file_hash];
    socket_send(socket_fd, to_string(file.blocks)); //blocks
    ack_recieve(socket_fd);
    socket_send(socket_fd, file.file_name); //filename
    ack_recieve(socket_fd);
    for (auto i : file.integrity) //filehash
    {
        socket_send(socket_fd, i.second);
        ack_recieve(socket_fd);
    }
    ack_send(socket_fd);
}
void send_file_info(int socket_fd, FileInfo file)
{
    socket_send(socket_fd, file.file_name);
    ack_recieve(socket_fd);
    socket_send(socket_fd, file.file_hash);
    ack_recieve(socket_fd);
    socket_send(socket_fd, to_string(file.blocks));
    ack_recieve(socket_fd);
    socket_send(socket_fd, to_string(file.size));
    ack_recieve(socket_fd);
    for (int i = 0; i < file.blocks; i++)
    {
        socket_send(socket_fd, to_string((int)file.integrity[i].first));
        ack_recieve(socket_fd);
        socket_send(socket_fd, file.integrity[i].second);
        ack_recieve(socket_fd);
    }
}
FileInfo recieve_file_info(int socket_fd)
{
    FileInfo file = FileInfo();
    file.file_name = socket_recieve(socket_fd);
    ack_send(socket_fd);
    file.file_hash = socket_recieve(socket_fd);
    ack_send(socket_fd);
    file.blocks = stoi(socket_recieve(socket_fd));
    ack_send(socket_fd);
    file.size = stoi(socket_recieve(socket_fd));
    ack_send(socket_fd);
    for (int i = 0; i < file.blocks; i++)
    {
        bool flag = stoi(socket_recieve(socket_fd));
        ack_send(socket_fd);
        string hash = socket_recieve(socket_fd);
        ack_send(socket_fd);
        file.integrity.push_back(make_pair(flag, hash));
    }
    return file;
}
void file_upload_send(int socket_fd, string file_hash)
{
    send_file_block_hash(socket_fd, file_hash);
    sync_print_ln(">>" + socket_recieve(socket_fd));
}
void file_upload_verify_send(int socket_fd, string file_hash)
{
    send_file_block_hash(socket_fd, file_hash);
    if (ack_recieve(socket_fd) == reply_NACK)
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
/**
 * @brief <command><group_id><file_name><destination_path>
 * 
 * @param tokens 
 * @return true 
 * @return false 
 */
bool file_download_pre_verification(vector<string> &tokens)
{
    string destination_path = tokens[3];
    if (!directory_query(destination_path))
        return false;
    return true;
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
        sync_print_ln("||Invalid command/parameter");
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
    else if (tokens[0] == command_download_file && tokens.size() == 6)
    {
        sync_print_ln(">>" + tokens[5]);
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
void *download_start(void *arg)
{
    pthread_mutex_lock(&thread_info_maintainer);
    pthread_mutex_unlock(&thread_info_maintainer);
    ThreadInfo info = *((ThreadInfo *)arg);
    try
    {
        string file_name = info.tokens[1];
        string file_hash = info.tokens[2];
        string file_path = info.tokens[3];
        int number_of_peers = stoi(info.tokens[4]);
        Download new_download = Download();
        for (int i = 1; i <= number_of_peers; i++)
        {
            pair<string, string> socket = read_socket_input(info.tokens[3 + i]);
            Peer new_peer = Peer();
            new_peer.ip_address = socket.first;
            new_peer.listener_port = socket.second;
            new_download.peer_list.push_back(new_peer);
        }
        int socket_fd = client_setup(make_pair(new_download.peer_list[0].ip_address, new_download.peer_list[0].listener_port));
        vector<string> command_tokens = {command_fetch_file_info, file_hash};
        socket_send(socket_fd, pack_message(command_tokens));
        FileInfo to_download = recieve_file_info(socket_fd);
        close(socket_fd);
        unsigned int size = to_download.size;

        socket_send(info.peer.socket_fd, "Download Complete");
    }
    catch (string error)
    {
        log(error);
    }
    peer_list.erase(pthread_self());
}
void *send_blocks(void *arg)
{
    pthread_mutex_lock(&thread_info_maintainer);
    pthread_mutex_unlock(&thread_info_maintainer);
    ThreadInfo info = *((ThreadInfo *)arg);
    try
    {
        string file_name = info.tokens[1];
        string file_hash = info.tokens[2];
        int start_block_index = stoi(info.tokens[2]);
        int end_block_index = stoi(info.tokens[3]);
        FileInfo file = hosted_files[file_hash];
    }
    catch (string error)
    {
        log(error);
    }
    peer_list.erase(pthread_self());
}
void *fetch_file_info(void *arg)
{
    pthread_mutex_lock(&thread_info_maintainer);
    pthread_mutex_unlock(&thread_info_maintainer);
    ThreadInfo info = *((ThreadInfo *)arg);
    try
    {
        string file_hash = info.tokens[2];
        FileInfo file = hosted_files[file_hash];
        send_file_info(info.peer.socket_fd, file);
    }
    catch (string error)
    {
        log(error);
    }
    peer_list.erase(pthread_self());
}
void process(vector<string> tokens, Peer peer)
{
    ThreadInfo *info = (ThreadInfo *)malloc(sizeof(ThreadInfo));
    info->peer = peer;
    info->tokens = tokens;
    pthread_t worker_thread;
    pthread_mutex_lock(&thread_info_maintainer);
    if (tokens[0] == command_download_init)
    {
        pthread_create(&worker_thread, NULL, download_start, info);
    }
    else if (tokens[0] == command_send_block)
    {
        pthread_create(&worker_thread, NULL, send_blocks, info);
    }
    else if (tokens[0] == command_fetch_file_info)
    {
        pthread_create(&worker_thread, NULL, fetch_file_info, info);
    }
    log("Connection to peer established :" + peer_list[worker_thread].ip_address + " " + peer_list[worker_thread].port);
    peer_list[worker_thread] = peer;
    pthread_mutex_unlock(&thread_info_maintainer);
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