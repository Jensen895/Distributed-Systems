#include "GossipMessage.h"
#include <stdexcept>
#include <iostream>

GossipMessage::GossipMessage(std::string mode, std::string id, std::string heartbeat, std::string clock)
    : mode(mode), id(id), heartbeat(heartbeat), clock(clock) {}

GossipMessage::GossipMessage(std::string mode, std::string id, std::string heartbeat, std::string clock, std::string sus, std::string incarnation)
    : mode(mode), id(id), heartbeat(heartbeat), clock(clock), sus(sus), incarnation(incarnation) {}

const std::string GossipMessage::membership_serialize()
{
    std::string serializedData;
    if (mode == std::to_string(Membershiplist::ServerMode::Gossip))
        serializedData = mode + CUSTOM_DELIMITER + id + CUSTOM_DELIMITER + heartbeat + CUSTOM_DELIMITER + clock;
    else if (mode == std::to_string(Membershiplist::ServerMode::GossipS))
        serializedData = mode + CUSTOM_DELIMITER + id + CUSTOM_DELIMITER + heartbeat + CUSTOM_DELIMITER + clock + CUSTOM_DELIMITER + sus + CUSTOM_DELIMITER + incarnation;
    return serializedData;
}

GossipMessage GossipMessage::deserialize(std::string serializedData)
{
    // cout << "serializedData: " << serializedData << endl;
    std::vector<std::string> parts;
    size_t pos = 0;
    while ((pos = serializedData.find(CUSTOM_DELIMITER)) != std::string::npos)
    {
        std::string part = serializedData.substr(0, pos);
        parts.push_back(part);
        serializedData.erase(0, pos + CUSTOM_DELIMITER.length());
    }
    parts.push_back(serializedData);
    if (parts.size() >= 6)
    {
        std::string mode = parts[0];
        std::string id = parts[1];
        std::string heartbeat = parts[2];
        std::string clock = parts[3];
        std::string sus = parts[4];
        std::string incar = parts[5];
        return GossipMessage(mode, id, heartbeat, clock, sus, incar);
    }
    else if (parts.size() >= 4)
    {
        std::string mode = parts[0];
        std::string id = parts[1];
        std::string heartbeat = parts[2];
        std::string clock = parts[3];
        return GossipMessage(mode, id, heartbeat, clock);
    }
    else
    {
        throw std::invalid_argument("Invalid input for deserialization");
    }
}