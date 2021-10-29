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
/**
 * @brief Will implement later -> command line interface inside tracker
 *  Main features: show tracker statistics
 */
void shell_setup()
{
}
void start_tracker()
{
    int tracker_socket_d = server_setup(tracker_1);
    log("hello");
}
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        exit(1);
    }
    string file_path(argv[1]);
    set_log_file("tracker_log_file.txt");
    read_tracker_file(file_path);
    start_tracker();
    return 0;
}