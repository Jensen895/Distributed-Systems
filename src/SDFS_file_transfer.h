#ifndef SDFS_FILE_TRANSFER
#define SDFS_FILE_TRANSFER

#include <iostream>
using namespace std;

class SDFSFileTransfer
{
public:
    static int64_t send_file(int sockfd, string filename);
    static int64_t recv_file(int sockfd, string filename);
    static string generate_temp_sdfsfilename(string filename);
    // static void client_file_transfer(int sockfd, string byte_request);
    // static void server_file_transfer(int sockfd, string byte_request);
};
#endif