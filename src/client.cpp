#include "commons.h"
pair<string, string> client_socket;
void client_startup()
{
    int client_fd;
    if ((client_fd = client_setup(tracker_1)) == -1)
    {
        log("Could not connect to Tracker [" + tracker_1.first + ":" + tracker_1.second + "]");
        exit(EXIT_FAILURE);
    }
    while (true)
    {
        string echo;
        getline(cin, echo);
        socket_send(client_fd, echo);
        string reply = socket_recieve(client_fd);
        cout << reply << endl;
    }
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
    client_startup();
    return 0;
}