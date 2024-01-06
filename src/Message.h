#ifndef MESSAGE
#define MESSAGE

#include <iostream>
#include <string>
#include <vector>
using namespace std;

const string CUSTOM_DELIMITER = "@@@chhuang5~~~yy63~~~@@@";
const string MEMBER_FILE_DELIMITER = "@@@member_file@@@";
const string FILE_TRANSFER_MSG = "FILE_TRANSFER_MSG";
const string START_RECV_FILE = "START_RECV_FILE";
const string FINISH_TRANSFER = "FINISH_TRANSFER";
const string SOMETHING_WRONG = "SOMETHING_WRONG";
const string NO_FILE_EXIST = "NO_FILE_EXIST";
const string IAMDONE = "IAMDONE";
const string TEMPFILENAME = "TEMPFILENAME@@";
const int SOCKET_BUFFER_SIZE = 262144;

enum MESSAGE_TYPE
{
    QUERY_LOG,
    MEMBERSHIP,
    JOIN,
    LEAVE,
    MBRLIST,
    MEMBERID,
    SUSPICION,
    SDFS,
    SDFS_PUT,
    SDFS_GET,
    SDFS_DELETE,
    SDFS_LS,
    SDFS_STORE,
    ASK_LEADER,
    SDFS_PRINT_GLOBAL,
    SDFS_PUT_LOCAL,
    SDFS_ASK_LOCAL_VERSION,
    SDFS_GET_LOCAL,
    SDFS_DELETE_LOCAL,
    SDFS_FR_GET,
    SDFS_FR_PUT,
    SDFS_FR_GET_LOCAL,
    SDFS_FR_PUT_LOCAL,
    SDFS_FR_INSERT_RECOVER,
    SDFS_PUT_LEADER,
    SDFS_GET_LEADER,
    SDFS_DELETE_LEADER,
    MAPLE,
    JUICE,
    MAPLE_LEADER,
    JUICE_LEADER,
    MAPLE_WORKER,
    JUICE_WORKER
};

// use this class to handle all command type: grep | membership
class Message
{
public:
    MESSAGE_TYPE message_type;
    Message(){};

    virtual ~Message(){};
    virtual const string serialize() = 0;
    static vector<string> parse_tokens(const string);
    static vector<string> parse_tokens_byte(const char *byte);
};
#endif