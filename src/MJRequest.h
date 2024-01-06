#ifndef MJREQUEST_H
#define MJREQUEST_H
#include <string>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <queue>
#include <set>
#include <mutex>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include "Message.h"

struct MJRequest : public Message
{
    MESSAGE_TYPE message_type;
    string exe_file;
    string num_workers;
    string sdfs_intermediate_filename;
    string src_dict;
    string dest_file;
    string delete_input;
    string maple_filename;
    fpos_t maple_fileposition;
    string juice_filename;
    fpos_t juice_fileposition;

    MJRequest();
    MJRequest(MESSAGE_TYPE message_type);
    MJRequest(MESSAGE_TYPE message_type, string exe_file, string num_workers, string sdfs_intermediate_filename, string src_dict);
    MJRequest(MESSAGE_TYPE message_type, string exe_file, string num_workers, string sdfs_intermediate_filename, string dest_file, string delete_input);
    MJRequest(MESSAGE_TYPE message_type, string exe_file, string sdfs_intermediate_filename, string src_dict, string maple_filename, fpos_t maple_fileposition);
    MJRequest(MESSAGE_TYPE message_type, string exe_file, string sdfs_intermediate_filename, string dest_file, string juice_filename, fpos_t juice_fileposition);
    
    virtual ~MJRequest(){};
    const string serialize();

    static MJRequest deserialize(string &request_string);
};
#endif