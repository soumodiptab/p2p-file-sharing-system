#include <bits/stdc++.h>
using namespace std;
void file_x()
{
    int block_size = 32;
    FILE *fp = fopen("filetest.txt", "r+");
    FILE *fp2 = fopen("testfile2.txt", "w+");
    fseek(fp,0,SEEK_END);
    int pos = ftell(fp);
    fseek(fp,0,SEEK_SET);
    cout << pos << endl;
    for (int i = 0; i < 15; i++)
    {
        char buffer[block_size];
        bzero(buffer, 0);
        fread(buffer, block_size, 1, fp);
        string x(buffer);
        fwrite(x.c_str(), block_size, 1, fp2);
        fflush(fp2);
    }
    char buffer[25];
    bzero(buffer, 0);
    fread(buffer, 25, 1, fp);
    string x(buffer);
    fwrite(x.c_str(), 25, 1, fp2);
    fflush(fp2);
    pos = ftell(fp);
    cout << pos << endl;
}
void fstream_file()
{
    int block_size = 32;
    fstream file,file2;
    file.open("avid.mp3",ios::in|ios::binary);
    file2.open("new_avid.mp3",ios::in|ios::out|ios::trunc|ios::binary);
    file.seekg(0,ios::end);
    int pos = file.tellg();
    file.seekg(0,ios::beg);
    int size=pos;
    int no_of_blocks=size/block_size;
    int last_size=size%block_size;
    cout << pos << endl;
    for (int i = 0; i < no_of_blocks; i++)
    {
        char *buffer=new char[block_size];
        file.read(buffer,block_size);
        string x(buffer);
        cout<<x<<endl;
        file2.write(buffer,file.gcount());
    }
    char *buffer=new char[last_size];
    file.read(buffer,last_size);
    string x(buffer);
    cout<<x<<endl;
    file2.write(buffer,file.gcount());
    pos = file.tellg();
    cout << pos << endl;
    file.close();
    file2.close();
}
void fb()
{
    int block_size = 32;
    fstream file;
    file.open("filetest.txt",ios::in|ios::binary);
    file.seekg(0,ios::end);
    int pos=file.tellg();
    file.close();
    cout<<pos<<endl;

}
int main()
{
    pthread_t worker1, worker2;
    fstream_file();
    //fb();
    cout << "hello";
    return 0;
}