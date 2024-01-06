#ifndef MEMBERIDRESPONSE_H
#define MEMBERIDRESPONSE_H

#include <string>
#include "Message.h"
using namespace std;

class MemberIdResponse : public Message
{
public:
    MESSAGE_TYPE message_type;
    string memberId;
    string membershiplist;

    MemberIdResponse(MESSAGE_TYPE message_type = MEMBERID, string memberId = "", string membershiplist = "");
    virtual ~MemberIdResponse(){};
    const string serialize();

    static MemberIdResponse deserialize(const string &data);
};

#endif