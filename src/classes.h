#include <bits/stdc++.h>
using namespace std;
class User
{
public:
    string user_name;
    string password;
    string color_assignment;
};
class Group
{
public:
    string name;
    string owner;
    unordered_set<string> members;
};
class Peer
{
public:
    string ip_address;
    string port;
    string user_name;
};