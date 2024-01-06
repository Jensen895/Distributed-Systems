#include "SDFS_file_transfer.h"
#include "Message.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>
using namespace std;
#define PORT 8088

int64_t get_filesize(string filename)
{
    ifstream file(filename, ifstream::ate | ifstream::binary);
    return file.tellg();
}

int64_t SDFSFileTransfer::send_file(int sockfd, string filename)
{
    int64_t fileSize;
    char buffer[SOCKET_BUFFER_SIZE] = {0};
    bool errored = false;
    std::ifstream file(filename, std::ifstream::binary);
    if (file.fail())
    {
        cout << filename << " cannot be opened" << endl;
        return -1;
    }

    // get file size
    fileSize = get_filesize(filename);

    // send file size first
    const char *file_size_str = to_string(fileSize).c_str();
    send(sockfd, file_size_str, strlen(file_size_str), 0);
    read(sockfd, buffer,
         SOCKET_BUFFER_SIZE); // receive message that the receiver got file size

    cout << "Start sending file: " << filename << ", file size: " << fileSize << endl;

    int64_t i = fileSize;
    while (i > 0)
    {
        const int64_t ssize = min(i, (int64_t)SOCKET_BUFFER_SIZE);
        file.read(buffer, ssize);
        int n = send(sockfd, buffer, ssize, MSG_NOSIGNAL);
        if (!file || n < 0 || n < ssize)
        {
            errored = true;
            break;
        }
        i -= n;
    }

    file.close();

    if (errored)
    {
        throw runtime_error("Broken pipe");
    }
    cout << "Finish sending file: " << filename << endl;
    return errored ? -2 : fileSize;
}

int64_t SDFSFileTransfer::recv_file(int sockfd, string filename)
{
    int64_t fileSize;
    char buffer[SOCKET_BUFFER_SIZE] = {0};
    bool errored = false;
    std::ofstream file(filename, ios::binary | ios::app);
    if (file.fail())
    {
        cout << "file: " << filename << " cannot be created" << endl;
        return -1;
    }

    // get file size first
    recv(sockfd, buffer, SOCKET_BUFFER_SIZE, 0);
    stringstream strstream(buffer);
    strstream >> fileSize;
    send(sockfd, FILE_TRANSFER_MSG.c_str(), sizeof(FILE_TRANSFER_MSG), 0); // tell the sender that the receiver got file size
    int64_t i = 0;

    cout << "Start receiving file: " << filename << ", file size: " << fileSize << endl;

    while (i < fileSize)
    {
        int n = recv(sockfd, buffer, SOCKET_BUFFER_SIZE, 0);
        if (n == 0)
            break;
        if (n < 0 || !file.write(buffer, n))
        {
            errored = true;
            break;
        }
        i += n;
    }

    file.close();
    cout << "Finish receiving file: " << filename << endl;
    return errored ? -2 : i;
}

string SDFSFileTransfer::generate_temp_sdfsfilename(string filename)
{
    return TEMPFILENAME + filename;
}