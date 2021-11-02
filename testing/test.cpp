#include<bits/stdc++.h>
using namespace std;
string extract_file_name(string &path)
{
    string file_name=path.substr(path.find_last_of('/')+1,path.size()-path.find_last_of('/')+1);
    return file_name;
}
int main()
{
    string path;
    cin>>path;
    cout<<extract_file_name(path);
    return 0;
}