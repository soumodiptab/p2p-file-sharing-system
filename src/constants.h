#include <bits/stdc++.h>
using namespace std;
const string command_upload_file = "upload_file";
const string command_download_file = "download_file";
const string command_create_user = "create_user";
const string command_login = "login";
const string command_logout = "logout";
const string command_create_group = "create_group";
const string command_join_group = "join_group";
const string command_leave_group = "leave_group";
const string command_list_requests = "list_requests";
const string command_accept_request = "accept_request";
const string command_list_groups = "list_groups";
const string command_list_files = "list_files";
const string command_show_downloads = "show_downloads";
const string command_stop_share = "stop_share";
const string command_print = "print";
const string command_change_color = "change_color";

const string constants_socket_failure = "Failed to open socket";
const string constants_socket_binding_failure = "Failed to bind socket";
const string constants_socket_conn_failure = "Failed to connect socket";
const string constants_socket_listen_failure = "Failed to listen to socket";
const string constants_socket_listen_success = "Listening to socket";
const string constants_socket_connected_success = "Connected to socket";
const string constants_socket_recv_failure = "Failed to recieve message";
const string constants_socket_send_failure = "Failed to send message";
const string constants_socket_empty_reply = "Incorrect reply";
const string constants_client_disconnected = "Opposite End has disconnected";
const string constants_client_connected = "Client has connected";
const int constants_message_buffer_limit = 1024;

const string reply_default = "Incorrect command/parameters";
const string reply_group_already_owner = "This user is already an owner";
const string reply_group_already_member = "This user is already a member";
const string reply_group_new_member = "User has been added to group";
const string reply_group_not_member = "User is not a part of the group";
const string reply_group_owner_not_leave = "Owner cannot leave before members";
const string reply_group_leave_group = "User has left the group";
const string reply_group_no_pending = "No Pending requests";
const string reply_group_already_exists = "Group already exists";
const string reply_group_created = "Group has been created";
const string reply_group_not_exits = "Group does not exist";
const string reply_group_no_group = "No Group Exists";
const string reply_group_not_owner = "You are not owner of this group";
const string reply_group_no_user_pending = "There is no group join request from this user";
const string reply_group_already_join = "Join request already exits. Wait for owner approval";
const string reply_group_join_added = "Join request  has been sent";
const string reply_group_join_approve = "Join request has been approved";
const string reply_user_already_exists = "User already Exists";
const string reply_user_new_user = "New user has been created";
const string reply_user_not_exist = "User does not exist";
const string reply_user_login = "User has been logged in";
const string reply_user_logout = "User has been logged out";
const string reply_user_login_incorrect = "Incorrect Password";
const string reply_user_not_login = "User is not logged";
const string reply_user_already_login = "User already logged in";
/**
 * @brief backlog connections:
 * maximum length to which the queue of pending connections for sockfd may grow
 */
const int constants_connection_backlog = 10;
/**
 * @brief Sets the block size of file
 * 
 */
const int constants_file_block_size = 32;