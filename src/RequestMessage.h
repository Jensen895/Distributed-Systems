#ifndef REQUESTMESSAGE_H
#define REQUESTMESSAGE_H

#include <string>
#include "Message.h"
using namespace std;

class RequestMessage : public Message
{
public:
    MESSAGE_TYPE message_type;
    string pattern;
    bool useRegex;
    bool unit_test;
    // RequestMessage(){};
    RequestMessage(MESSAGE_TYPE message_type = QUERY_LOG, string pattern = "", bool useRegex = false, bool unit_test = false);
    virtual ~RequestMessage(){};
    const string serialize();

    static RequestMessage deserialize(const string &data);
};

#endif // REQUESTMESSAGE_H
