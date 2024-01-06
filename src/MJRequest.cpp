#include "MJRequest.h"
#include <stdexcept>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <set>
#include <thread>
using namespace std;
MJRequest::MJRequest()
{
}
MJRequest::MJRequest(MESSAGE_TYPE message_type)
    : message_type(message_type)
{
}

MJRequest::MJRequest(MESSAGE_TYPE message_type, string exe_file, string num_workers, string sdfs_intermediate_filename, string src_dict)
    : message_type(message_type), exe_file(exe_file), num_workers(num_workers), sdfs_intermediate_filename(sdfs_intermediate_filename), src_dict(src_dict)
{
}

MJRequest::MJRequest(MESSAGE_TYPE message_type, string exe_file, string num_workers, string sdfs_intermediate_filename, string dest_file, string delete_input)
    : message_type(message_type), exe_file(exe_file), num_workers(num_workers), sdfs_intermediate_filename(sdfs_intermediate_filename), dest_file(dest_file), delete_input(delete_input)
{
}

MJRequest::MJRequest(MESSAGE_TYPE message_type, string exe_file, string sdfs_intermediate_filename, string dest_file, string maple_filename, fpos_t maple_fileposition)
    : message_type(message_type), exe_file(exe_file), sdfs_intermediate_filename(sdfs_intermediate_filename), dest_file(dest_file), maple_filename(maple_filename), maple_fileposition(maple_fileposition)
{
}

const string MJRequest::serialize()
{
    string serialized;
    // Serialize the struct into a string
    if (message_type == MAPLE || MAPLE_LEADER)
    {
        serialized = to_string(message_type) + CUSTOM_DELIMITER + exe_file + CUSTOM_DELIMITER + num_workers + CUSTOM_DELIMITER + sdfs_intermediate_filename + CUSTOM_DELIMITER + src_dict;
    }
    else if (message_type == JUICE || JUICE_LEADER)
    {
        serialized = to_string(message_type) + CUSTOM_DELIMITER + exe_file + CUSTOM_DELIMITER + num_workers + CUSTOM_DELIMITER + sdfs_intermediate_filename + CUSTOM_DELIMITER + dest_file + CUSTOM_DELIMITER + delete_input;
    }
    else if (message_type == MAPLE_WORKER)
    {
        stringstream ss;
        ss << static_cast<long long>(maple_fileposition);
        serialized = to_string(message_type) + CUSTOM_DELIMITER + exe_file + CUSTOM_DELIMITER + sdfs_intermediate_filename + CUSTOM_DELIMITER + src_dict + CUSTOM_DELIMITER + maple_filename + CUSTOM_DELIMITER + ss.str();
    }
    else if (message_type == JUICE_WORKER)
    {
        stringstream ss;
        ss << static_cast<long long>(juice_fileposition);
        serialized = to_string(message_type) + CUSTOM_DELIMITER + exe_file + CUSTOM_DELIMITER + sdfs_intermediate_filename + CUSTOM_DELIMITER + dest_file + CUSTOM_DELIMITER + juice_filename + CUSTOM_DELIMITER + ss.str();
    }
    return serialized;
}

MJRequest MJRequest::deserialize(string &request_string)
{
    vector<string> tokens = Message::parse_tokens(request_string);
    MESSAGE_TYPE message_type = (MESSAGE_TYPE)stoi(tokens[0]);
    if (message_type == MAPLE || message_type == MAPLE_LEADER)
    {
        string exe_file = tokens[1];
        string num_workers = tokens[2];
        string sdfs_intermediate_filename = tokens[3];
        string src_dict = tokens[4];
        return MJRequest(message_type, exe_file, num_workers, sdfs_intermediate_filename, src_dict);
    }
    else if (message_type == JUICE || message_type == JUICE_LEADER)
    {
        string exe_file = tokens[1];
        string num_workers = tokens[2];
        string sdfs_intermediate_filename = tokens[3];
        string dest_file = tokens[4];
        string delete_input = tokens[5];
        return MJRequest(message_type, exe_file, num_workers, sdfs_intermediate_filename, dest_file, delete_input);
    }
    else if (message_type == MAPLE_WORKER)
    {
        string exe_file = tokens[1];
        string sdfs_intermediate_filename = tokens[2];
        string src_dict = tokens[3];
        string maple_filename = tokens[4];
        string maple_fileposition = tokens[5];
        return MJRequest(message_type, exe_file, sdfs_intermediate_filename, src_dict, maple_filename, maple_fileposition);
    }
    else if (message_type == JUICE_WORKER)
    {
        string exe_file = tokens[1];
        string sdfs_intermediate_filename = tokens[2];
        string dest_file = tokens[3];
        string juice_filename = tokens[4];
        string juice_fileposition = tokens[5];
        return MJRequest(message_type, exe_file, sdfs_intermediate_filename, dest_file, juice_filename, juice_fileposition);
    }
}