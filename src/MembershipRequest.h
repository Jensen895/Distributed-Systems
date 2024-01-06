#ifndef MEMBERSHIPREQUEST_H
#define MEMBERSHIPREQUEST_H
#include <string>
#include "Message.h"

class MembershipRequest : public Message
{
public:
    MESSAGE_TYPE message_type;
    string hostname;
    string port;
    string memberid;

    MembershipRequest(MESSAGE_TYPE);
    MembershipRequest(MESSAGE_TYPE message_type, string hostname, string port = "3490");
    virtual ~MembershipRequest(){};
    const string serialize();

    static MembershipRequest deserialize(const string &data);
};

#endif