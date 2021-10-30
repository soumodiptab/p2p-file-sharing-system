#include "commons.h"
#include "classes.h"
/**
 * When message arrives from peer ->
 * 1. Process input and segregate into commands [common input parser]
 * 2. Validator to validate command [validator_api_name]
 * 3. Process command and generate follow up:
 * 4. Acknowledgement / Action
 */
unordered_map<string, group> group_list;
unordered_map<string, user> user_list;
void *thread_service(void *socket_fd)
{
    int thread_socket_fd = *((int *)socket_fd);
    delete socket_fd;
    while (true)
    {
        try
        {
            string client_message = socket_recieve(thread_socket_fd);
            cout << client_message << endl;
            string reply = client_message + " ACK";
            socket_send(thread_socket_fd, reply);
        }
        catch (string error)
        {
            log(error);
            break;
        }
    }
    close(thread_socket_fd);
}
/**
 * @brief Will implement later -> command line interface inside tracker[T.B.A.]
 *  Main features: show tracker statistics
 * This will be on another thread
 */
void shell_setup()
{
}
void start_tracker()
{
    int tracker_socket_fd = server_setup(tracker_1);
    log("Started listening on: [" + tracker_1.first + ":" + tracker_1.second + "]");
    while (true)
    {
        int *thread_socket_fd = new int;
        struct sockaddr_storage peer_address;
        socklen_t peer_addr_size = sizeof(peer_address);
        *thread_socket_fd = accept(tracker_socket_fd, (struct sockaddr *)&peer_address, &peer_addr_size);
        if (*thread_socket_fd == -1)
        {
            log("Unable to connect");
            continue;
        }
        pthread_t worker;
        pthread_create(&worker, NULL, thread_service, thread_socket_fd);
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