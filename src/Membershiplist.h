#ifndef MEMBERSHIPLIST_H
#define MEMBERSHIPLIST_H

#include <mutex>
#include <string>
#include <iostream>
#include <array>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <set>
#include <unordered_map>
#include "Member.h"

using namespace std;

class Membershiplist
{
public:
    enum ServerMode
    {
        Gossip,
        GossipS
    };
    struct MemberKeys
    {
        ServerMode mode = Gossip;
        int heartbeat = 0;
        int clock;
        int sus = 0;
        int incarnation = 0;
    };
    static int Tfail;
    static int TSuspect;
    static ServerMode servermode;
    static mutex ListMutex;
    static string ipAddress;
    static string Introducer;
    static set<string> Map;
    static string SelfMember;
    static unordered_map<string, MemberKeys> allMembersList;
    static set<string> membersToRemove;
    static vector<string> replicaToRecovery;
    static bool leave;
    static int port;
    static int timestamp;
    static string id;
    static string Leader;

    Membershiplist() = default;

    static void clockInc();
    static void heartbeatInc();
    static void CreateList(int port);
    static void CreateList();
    static void AddMember(const string memberid);
    static void print_membershiplist();
    static void UpdateSelf(const string memberid);
    static bool CheckSelfExist(const string memberid);
    static vector<Member> ChooseBRandomMember(int b);
    static void compareGossip(unordered_map<string, MemberKeys> newmem);
    static void detectFailure();
    static void DeleteList();
    static void EnableGossipS();
    static void detectFailureS();
    static void UpdateMembershiplist(unordered_map<string, MemberKeys> newmem);
};

#endif