#include "commons.h"
pair<string, string> client_socket_listener;
pthread_t listener_thread;
class FileInfo
{
public:
    string file_hash;
    string path;
    int file_descriptor;
    struct stat file_stat;
    unsigned long long size;
    vector<pair<bool, string>> integrity;
    pthread_mutex_t file_sync;
    FileInfo(){};
    FileInfo(string path)
    {
        this->path = path;
        file_hash = generate_SHA1(path);
    }
    FileInfo(string path, int blocks)
    {
        FileInfo(path);
        integrity = vector<pair<bool, string>>(blocks, make_pair(0, ""));
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
        int blocks = size / constants_file_block_size;
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
        integrity.push_back(make_pair(1, generate_SHA1(buffer, last_block_size)));
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
unordered_map<string, FileInfo> hosted_files;
bool file_uploader(vector<string> &tokens)
{
    string path = tokens[0];
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
    tokens.push_back(file.file_hash);
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
void *listener_startup(void *)
{
    int listener_fd = server_setup(client_socket_listener);
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