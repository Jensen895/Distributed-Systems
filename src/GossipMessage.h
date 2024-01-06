#ifndef GOSSIPMESSAGE_H
#define GOSSIPMESSAGE_H

#include <string>
#include "Message.h"
#include "Membershiplist.h"

class GossipMessage
{
public:
    std::string mode;
    std::string id;
    std::string heartbeat;
    std::string clock;
    std::string sus;
    std::string incarnation;
    GossipMessage() = default;
    GossipMessage(std::string mode, std::string id, std::string heartbeat, std::string clock);
    GossipMessage(std::string mode, std::string id, std::string heartbeat, std::string clock, std::string sus, std::string incarnation);
    const std::string membership_serialize();
    static GossipMessage deserialize(string data);
};

#endif