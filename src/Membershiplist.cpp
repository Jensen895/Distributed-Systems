#include <iostream>
#include <unistd.h>
#include <set>
#include <random>
#include <vector>
#include <time.h>
#include <algorithm>
#include <random>
#include <regex>
#include "Membershiplist.h"
#include "Member.h"

using namespace std;

Membershiplist::ServerMode Membershiplist::servermode = Membershiplist::ServerMode::Gossip;
string Membershiplist::Introducer = "fa23-cs425-4101.cs.illinois.edu";
set<string> Membershiplist::Map;
string Membershiplist::SelfMember;
int Membershiplist::port = 8080;
mutex Membershiplist::ListMutex;
std::unordered_map<std::string, Membershiplist::MemberKeys> Membershiplist::allMembersList;
string Membershiplist::id = "";
bool Membershiplist::leave = false;
set<string> Membershiplist::membersToRemove;
vector<string> Membershiplist::replicaToRecovery;
int Membershiplist::Tfail = 8;
int Membershiplist::TSuspect = 2;
string Membershiplist::Leader;

void Membershiplist::CreateList(int port)
{
    // create_test_Membershiplistlist();
    Membershiplist::port = port;
    Membershiplist::CreateList();
}

void Membershiplist::CreateList()
{

    Membershiplist::Map.clear();
    Membershiplist::allMembersList.clear();

    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    string currentHostname(hostname);

    Membershiplist::servermode = ServerMode::Gossip;
    Membershiplist::Leader = Membershiplist::SelfMember;
}

void Membershiplist::DeleteList()
{
    // reset ring map
    Membershiplist::ListMutex.lock();
    Membershiplist::Map.clear();
    Membershiplist::allMembersList.clear();
    Membershiplist::SelfMember = "";
    Membershiplist::ListMutex.unlock();
}

void Membershiplist::clockInc()
{
    Membershiplist::ListMutex.lock();
    Membershiplist::allMembersList[Membershiplist::SelfMember].clock++;
    Membershiplist::ListMutex.unlock();
}

void Membershiplist::heartbeatInc()
{
    Membershiplist::ListMutex.lock();
    Membershiplist::allMembersList[Membershiplist::SelfMember].heartbeat++;
    Membershiplist::ListMutex.unlock();
}

string get_time()
{
    time_t currentTime;
    struct tm *timeInfo;
    char timeString[50];

    time(&currentTime);
    timeInfo = localtime(&currentTime);

    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeInfo);

    printf("Current time: %s\n", timeString);

    return timeString;
}

string get_leader(set<string> &Map)
{
    int minXX = 20;
    string Leader;
    for (auto &member : Membershiplist::Map)
    {
        size_t fPos = member.find('@');           // Find the position of the first underscore
        size_t sPos = member.find('@', fPos + 1); // Find the position of the second underscore
        if (fPos != std::string::npos && sPos != std::string::npos)
        {
            std::string host = member.substr(fPos + 1, sPos - fPos - 1);
            std::regex pattern("fa23-cs425-41(\\d{2}).cs.illinois.edu");
            std::smatch match;
            if (std::regex_search(host, match, pattern))
            {
                int currentXX = std::stoi(match[1].str());
                if (currentXX < minXX)
                {
                    minXX = currentXX;
                    Leader = host;
                }
            }
        }
    }
    return Leader;
}

void Membershiplist::print_membershiplist()
{
    cout << "=========== Membership list ===========" << endl;
    cout << "Member number: " << Membershiplist::Map.size() << endl;
    // for (auto &member : Membershiplist::allMembersList)
    // {
    //     string marker = (member.first == Membershiplist::SelfMember) ? " <-" : "";
    //     cout << "\tMember: " << member.first << " | Heartbeat: " << member.second.heartbeat << " | Clock: " << member.second.clock << marker << std::endl;
    // }

    cout << "Leader:" << Membershiplist::Leader << endl;
    cout << "Member:" << endl;
    for (auto &member : Membershiplist::Map)
    {
        string marker = (member == Membershiplist::SelfMember) ? " <-" : "";
        cout << "\t" << member << ", clock: " << Membershiplist::allMembersList[member].clock << marker << std::endl;
    }
    cout << "Fail list:" << endl;
    for (auto &member : Membershiplist::membersToRemove)
    {
        cout << "\t" << member << std::endl;
    }
    cout << "Recovery list:" << endl;
    for (auto &member : Membershiplist::replicaToRecovery)
    {
        cout << "\t" << member << std::endl;
    }
    cout << "=======================================" << endl;
}

void Membershiplist::AddMember(const string memberid)
{
    Membershiplist::ListMutex.lock();
    Member new_member = Member::parse_member_id(memberid);
    for (auto &element : Membershiplist::Map)
    {
        Member old_member = Member::parse_member_id(element);
        if (new_member.hostname == old_member.hostname)
        {
            cout << new_member.hostname << " exist!" << endl;
            Membershiplist::ListMutex.unlock();
            return;
        }
    }
    string time = get_time();
    cout << "Time : [" << time << "]Insert a new member: " << memberid << " to Map!" << endl;
    Membershiplist::Map.insert(memberid);

    MemberKeys key;                                                               // Create a new key
    key.clock = Membershiplist::allMembersList[Membershiplist::SelfMember].clock; // change to current clock
    key.heartbeat = 0;
    key.incarnation = 0;
    key.sus = 0;
    key.mode = Membershiplist::servermode;
    Membershiplist::allMembersList[memberid] = key;
    // cout << "Adding a member: " << memberid << " when seif is : " << Membershiplist::SelfMember << endl;
    Membershiplist::ListMutex.unlock();
}

void set_map(set<string> &Map)
{
    Membershiplist::ListMutex.lock();
    Membershiplist::Map = Map;
    Membershiplist::ListMutex.unlock();
}

void Membershiplist::UpdateSelf(const string memberid)
{
    Membershiplist::ListMutex.lock();
    Membershiplist::SelfMember = memberid;
    Membershiplist::Leader = Membershiplist::SelfMember; // inthe begining, the leader will be itself
    cout << " Before receive the udp, the leader is:" << Membershiplist::Leader << endl;
    Membershiplist::ListMutex.unlock();
}

bool Membershiplist::CheckSelfExist(const string memberid)
{
    Membershiplist::ListMutex.lock();
    Member new_member = Member::parse_member_id(memberid);
    for (auto &element : Membershiplist::Map)
    {
        Member old_member = Member::parse_member_id(element);
        if (old_member.hostname == new_member.hostname)
        {
            cout << old_member.hostname << " exist!" << endl;
            Membershiplist::ListMutex.unlock();
            return true;
        }
    }
    Membershiplist::ListMutex.unlock();
    return false;
}

vector<Member> Membershiplist::ChooseBRandomMember(int b)
{
    vector<Member> random_members;
    vector<string> picked_elements;
    set<string> temp = Membershiplist::Map;
    temp.erase(Membershiplist::SelfMember);
    srand(static_cast<unsigned int>(time(nullptr)));
    if (temp.size() > 0)
    {
        // for (int i = 0; i < b; i++)
        // {
        //     int index = rand() % temp.size();
        //     auto it = temp.begin();
        //     std::advance(it, index);
        //     string random_member_str = *it;

        //     Member random_member = Member::parse_member_id(random_member_str);
        //     random_members.push_back(random_member);
        // }
        if (temp.size() > b)
        {
            std::vector<string> elements(temp.begin(), temp.end());

            // Shuffle the elements randomly
            std::random_device rd;
            std::mt19937 gen(rd());
            shuffle(elements.begin(), elements.end(), gen);

            // Pick the first k elements from the shuffled vector
            picked_elements.assign(elements.begin(), elements.begin() + b);
        }
        else
        {
            // Allow repetition when set size is less than k
            std::vector<string> elements(temp.begin(), temp.end());
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> distribution(0, elements.size() - 1);

            for (int i = 0; i < b; i++)
            {
                int random_index = distribution(gen);
                picked_elements.push_back(elements[random_index]);
            }
        }

        // Print the picked elements
        for (string element : picked_elements)
        {
            Member random_member = Member::parse_member_id(element);
            random_members.push_back(random_member);
        }
    }
    return random_members;
}

void Membershiplist::compareGossip(unordered_map<string, MemberKeys> newList)
{
    // std::lock_guard<std::mutex> lock(ListMutex);
    Membershiplist::ListMutex.lock();
    for (const auto &newListMember : newList)
    {

        const string newListMemberId = newListMember.first;
        const MemberKeys newMemberKeys = newListMember.second;

        if (Membershiplist::servermode == Membershiplist::ServerMode::Gossip && newListMemberId == Membershiplist::SelfMember)
        {
            continue;
        }
        else if (Membershiplist::servermode == Membershiplist::ServerMode::GossipS && newListMemberId == Membershiplist::SelfMember)
        {
            // check the sus, if 1 then somebody suspect me, add incarnation number. (ignore sus cuz sus in this server always be 0)
            if (newListMemberId == Membershiplist::SelfMember && newMemberKeys.sus == 1)
            {
                if (newMemberKeys.incarnation == Membershiplist::allMembersList[Membershiplist::SelfMember].incarnation)
                {
                    Membershiplist::allMembersList[Membershiplist::SelfMember].incarnation += 1;
                }
                string time = get_time();
                cout << "[" << time << "]Receive suspicion. " << endl;
                // cout << "Someone suspect me! Add incarnation number to " << Membershiplist::allMembersList[Membershiplist::SelfMember].incarnation << endl;
            }
        }

        auto existingMember = Membershiplist::allMembersList.find(newListMemberId);

        // If I already have that member on my membershiplist
        if (existingMember != Membershiplist::allMembersList.end())
        {
            if (newMemberKeys.heartbeat > Membershiplist::allMembersList[newListMemberId].heartbeat)
            {
                Membershiplist::allMembersList[newListMemberId].heartbeat = newMemberKeys.heartbeat;
                Membershiplist::allMembersList[newListMemberId].clock = Membershiplist::allMembersList[SelfMember].clock;
            }

            // check this part if the mode is GossipS
            if (Membershiplist::servermode == Membershiplist::ServerMode::GossipS)
            {
                // somebody else suspect the newListMemberId, check my own allMembersList[newListMemberId].incarnation to see if i tun the sus to 1 too.
                if (newMemberKeys.sus == 1)
                {
                    // newer suspicion, should update allMembersList[newListMemberId].incarnation and allMembersList[newListMemberId].sus(no matter sus in list now)
                    if (allMembersList[newListMemberId].incarnation < newMemberKeys.incarnation)
                    {
                        allMembersList[newListMemberId].incarnation = newMemberKeys.incarnation;
                        allMembersList[newListMemberId].sus = newMemberKeys.sus;
                    }
                }
                // if the suspected server send or other server send me the alive, update my list.
                else if (newMemberKeys.sus == 0 && Membershiplist::allMembersList[newListMemberId].sus == 1 && newMemberKeys.incarnation > Membershiplist::allMembersList[newListMemberId].incarnation)
                {
                    string time = get_time();
                    // cout << "Cancel my suspect on member: " << newListMemberId << ", my inc num of it is: " << Membershiplist::allMembersList[newListMemberId].incarnation << ", and receiving larger inc num: " << newMemberKeys.incarnation << endl;
                    cout << "[" << time << "]Cancel my suspect on member: " << newListMemberId << endl;
                    Membershiplist::allMembersList[newListMemberId].sus = 0;
                    Membershiplist::allMembersList[newListMemberId].incarnation = newMemberKeys.incarnation;
                    Membershiplist::allMembersList[newListMemberId].clock = Membershiplist::allMembersList[Membershiplist::SelfMember].clock; //?? Should I do this?
                }
            }
        }
        else
        {
            // If other server gossip a new node
            if (Membershiplist::membersToRemove.find(newListMemberId) == Membershiplist::membersToRemove.end())
            {
                MemberKeys key = newMemberKeys;
                key.clock = Membershiplist::allMembersList[SelfMember].clock;
                if (Membershiplist::servermode == Membershiplist::ServerMode::GossipS)
                {
                    key.mode = Membershiplist::ServerMode::GossipS;
                }
                Membershiplist::allMembersList[newListMemberId] = key;
                Membershiplist::Map.insert(newListMemberId);
            }
        }
    }
    Membershiplist::Leader = get_leader(Membershiplist::Map);
    Membershiplist::ListMutex.unlock();
}

void Membershiplist::UpdateMembershiplist(unordered_map<string, MemberKeys> newList)
{
    Membershiplist::ListMutex.lock();
    for (const auto &newListMember : newList)
    {
        const string newListMemberId = newListMember.first;
        const MemberKeys newMemberKeys = newListMember.second;
        if (newListMemberId == Membershiplist::SelfMember)
            continue;
        MemberKeys key = newMemberKeys;
        key.clock = Membershiplist::allMembersList[SelfMember].clock;
        if (Membershiplist::servermode == Membershiplist::ServerMode::GossipS)
        {
            key.mode = Membershiplist::ServerMode::GossipS;
        }
        Membershiplist::allMembersList[newListMemberId] = key;
        Membershiplist::Map.insert(newListMemberId);
    }
    cout << "after update" << endl;
    for (const auto &newListMember : Membershiplist::Map)
    {
        cout << newListMember << endl;
    }

    Membershiplist::Leader = get_leader(Membershiplist::Map);
    Membershiplist::ListMutex.unlock();
}

void Membershiplist::detectFailure()
{
    // std::lock_guard<std::mutex> lock(ListMutex);
    Membershiplist::ListMutex.lock();

    vector<string> waitlist;
    int selfClock = Membershiplist::allMembersList[Membershiplist::SelfMember].clock;
    for (const auto &pair : Membershiplist::allMembersList)
    {
        const string memberId = pair.first;
        if (memberId == Membershiplist::SelfMember)
            continue;
        int memberClock = pair.second.clock;

        if (selfClock - memberClock > Membershiplist::Tfail)
        {
            waitlist.push_back(memberId);
            Membershiplist::membersToRemove.insert(memberId);
            // Membershiplist::replicaToRecovery.push_back(memberId);
            string time = get_time();
            cout << "[" << time << "]ID : " << memberId << " fail!" << endl;
        }
    }
    for (auto &mem : waitlist)
    {
        Membershiplist::allMembersList.erase(mem);
        Membershiplist::Map.erase(mem);
    }
    Membershiplist::Leader = get_leader(Membershiplist::Map);
    Membershiplist::ListMutex.unlock();
}

void Membershiplist::EnableGossipS()
{
    Membershiplist::ListMutex.lock();
    Membershiplist::servermode = Membershiplist::ServerMode::GossipS;
    Membershiplist::ListMutex.unlock();
}

void Membershiplist::detectFailureS()
{
    // std::lock_guard<std::mutex> lock(ListMutex);
    Membershiplist::ListMutex.lock();

    vector<string> waitlist;
    int selfClock = Membershiplist::allMembersList[Membershiplist::SelfMember].clock;
    for (const auto &pair : Membershiplist::allMembersList)
    {
        const string memberId = pair.first;
        if (memberId == Membershiplist::SelfMember)
            continue;
        int memberClock = pair.second.clock;

        if (selfClock - memberClock > Membershiplist::TSuspect && Membershiplist::allMembersList[memberId].sus == 0)
        {
            Membershiplist::allMembersList[memberId].sus = 1;
            string time = get_time();
            cout << "[" << time << "]ID : " << memberId << " be suspected! " << endl;
        }
        else if (selfClock - memberClock > Membershiplist::Tfail && Membershiplist::allMembersList[memberId].sus == 1)
        {
            waitlist.push_back(memberId);
            Membershiplist::membersToRemove.insert(memberId);
            string time = get_time();
            cout << "[" << time << "]ID : " << memberId << " fail!" << endl;
        }
    }

    for (auto &mem : waitlist)
    {
        Membershiplist::allMembersList.erase(mem);
        Membershiplist::Map.erase(mem);
    }
    Membershiplist::ListMutex.unlock();
}