#ifndef MEMBERIDREQUEST_H
#define MEMBERIDREQUEST_H

#include <string>
#include "Message.h"
using namespace std;

class MemberIdRequest : public Message
{
public:
    MESSAGE_TYPE message_type;
    string memberId;
    string membershiplist;

    MemberIdRequest(MESSAGE_TYPE message_type = MEMBERID, string memberId = "", string membershiplist = "");
    virtual ~MemberIdRequest(){};
    const string serialize();

    static MemberIdRequest deserialize(const string &data);
};

#endif