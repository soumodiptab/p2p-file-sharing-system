#include <bits/stdc++.h>
using namespace std;
string command_upload_file = "upload_file";
string command_download_file = "download_file";
string command_create_user = "create_user";
string command_login = "login";
string command_create_group = "create_group";
string command_join_group = "join_group";
string command_print = "print";
string command_change_color = "change_color";

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

const string reply_group_already_owner = "This user is already an owner";
const string reply_group_already_member = "This user is already a member";
const string reply_group_new_member = "User has been added to group";
const string reply_group_no_pending = "No Pending requests";
const string reply_group_no_user_pending = "There is no group join request from this user";
const string reply_group_already_join = "Join request already exits. Wait for owner approval";
const string reply_group_join_added = "Join request  has been sent";
const string reply_group_join_approve = "Join request has been approved";
const string reply_user_already_exists = "User already Exists";
const string reply_user_new_user = "New user has been created";
/**
 * @brief backlog connections:
 * maximum length to which the queue of pending connections for sockfd may grow
 */
const int constants_connection_backlog = 10;