#include <bits/stdc++.h>
using namespace std;
class user
{
public:
    string user_name;
    string password;
};
class group
{
public:
    string name;
    unordered_set<string> user_names;
};
class peer
{
public:
    string ip_address;
    string port;
    string user_name;
};