#include <bits/stdc++.h>
using namespace std;
#include <bits/stdc++.h>
#include <pthread.h>
using namespace std;
void file_x()
{
    int block_size = 32;
    FILE *fp = fopen("test_file.txt", "r+");
    FILE *fp2 = fopen("test_file2.txt", "w+");
    for (int i = 0; i < 15; i++)
    {
        char buffer[block_size];
        bzero(buffer, 0);
        fread(buffer, block_size, block_size, fp);
        string x(buffer);
        fwrite(x.c_str(), sizeof(char), block_size, fp);
    }
    char buffer[25];
    bzero(buffer, 0);
    fread(buffer, block_size, 25, fp);
    string x(buffer);
    fwrite(x.c_str(), sizeof(char), 25, fp);
    int pos = ftell(fp);
    cout << pos << endl;
}
int main()
{
    pthread_t worker1, worker2;
    file_x();

    cout << "hello";
    return 0;
}