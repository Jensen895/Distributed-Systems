#include "SDFS_filelist.h"
using namespace std;

unordered_map<string, FileMetadata *> FileList::FileMetadataMap;

unordered_map<string, int> FileList::FileMap;
// unordered_map<string, FileVersionMetadata *> FileList::FileMap;

unordered_set<string> FileList::RecoveredFailureServers;
bool FileList::Recoverstart = false;
int FileList::VersionNumber = 1;
mutex FileList::GlobalMutex;
mutex FileList::LocalMutex;

void FileList::CreateContext()
{
    FileList::GlobalMutex.unlock();
    FileList::LocalMutex.unlock();

    FileList::GlobalMutex.lock();
    FileList::LocalMutex.lock();

    // reset file metadata
    FileList::FileMetadataMap = unordered_map<string, FileMetadata *>(); // global

    FileList::FileMap = unordered_map<string, int>(); // local
    // FileList::FileMap = unordered_map<string, FileVersionMetadata *>(); // local

    //   FileList::RecoveredFailureServers = unordered_set<string>();

    FileList::VersionNumber = 1;

    FileList::LocalMutex.unlock();
    FileList::GlobalMutex.unlock();
}

bool FileList::CheckFileExistGlobal(const string filename)
{
    FileList::GlobalMutex.lock();
    for (auto &file : FileList::FileMetadataMap)
    {
        const string &oldfilename = file.first;
        FileMetadata *newmetadata = new FileMetadata;
        newmetadata = file.second;
        if (oldfilename == filename)
        {
            vector<string> replicas;
            for (auto &entry : newmetadata->version_replicaids)
            {
                replicas = entry.second;
            }
            if (!replicas.empty())
            {
                FileList::GlobalMutex.unlock();
                return true;
            }
        }
    }
    FileList::GlobalMutex.unlock();
    return false;
}

bool FileList::CheckFileExistLocal(const string filename)
{
    FileList::LocalMutex.lock();
    for (auto &file : FileList::FileMap)
    {
        const string &oldfilename = file.first;
        if (oldfilename == filename)
        {
            FileList::LocalMutex.unlock();
            return true;
        }
    }
    FileList::LocalMutex.unlock();
    return false;
}

void FileList::InsertFileGlobal(const int version, const string filename, const vector<string> replicas)
{

    FileList::GlobalMutex.lock();
    FileMetadata *metadata = new FileMetadata;
    // Populate the FileMetadata object with version and replicas
    metadata->version_replicaids[version] = replicas;
    // Insert it into the map
    FileList::FileMetadataMap[filename] = metadata;
    FileList::GlobalMutex.unlock();

    return;
}

void FileList::UpdateFileGlobal(const int version, const string filename)
{

    FileList::GlobalMutex.lock();
    auto it = FileList::FileMetadataMap.find(filename);

    if (it != FileList::FileMetadataMap.end())
    {
        int oldKey;
        for (auto &entry : FileList::FileMetadataMap[filename]->version_replicaids)
        {
            oldKey = entry.first;
        }
        // Update the key in version_replicaids
        it->second->updateKey(oldKey, version);
    }
    FileList::GlobalMutex.unlock();

    return;
}

vector<string> FileList::GetFileReplicas(const string filename)
{
    vector<string> hostnames;

    auto it = FileList::FileMetadataMap.find(filename);
    if (it != FileList::FileMetadataMap.end())
    {
        for (const auto &entry : FileList::FileMetadataMap[filename]->version_replicaids)
        {
            for (const auto &replica : entry.second)
            {
                hostnames.push_back(replica);
            }
        }
    }

    return hostnames;
}

void FileList::PrintFileList()
{
    FileList::GlobalMutex.lock();
    int file_count = FileList::FileMetadataMap.size();
    cout << "=========== File list ===========" << endl;
    for (const auto &entry : FileList::FileMetadataMap)
    {
        const string &filename = entry.first;
        const FileMetadata *metadata = entry.second;
        // cout << "SDFSFilename: " << filename << endl;

        for (const auto &versionReplicas : metadata->version_replicaids)
        {
            int version = versionReplicas.first;
            const vector<string> &replicas = versionReplicas.second;
            if (replicas.empty())
            {
                file_count--;
                break;
            }
            cout << "SDFSFilename: " << filename << endl;
            cout << "Version: " << version << endl;
            cout << "Replicas: " << endl;

            for (const string &replica : replicas)
            {
                cout << "\t" << replica << endl;
            }
            cout << endl;
        }
    }
    cout << "Recovery List:" << endl;
    for (auto &recover : FileList::RecoveredFailureServers)
    {
        cout << "\t" << recover << endl;
    }
    cout << "============ Total File:" << file_count << "============" << endl;
    FileList::GlobalMutex.unlock();
    return;
}

void FileList::PrintFileListinClient(const string &serializedData) // TODO: fix the bug
{
    istringstream ss(serializedData);
    string token;
    string filename;
    string version;
    int count = 0;
    cout << "=========== File list ===========" << endl;
    if (serializedData == "None")
    {
        cout << endl;
    }
    else
    {
        while (getline(ss, token))
        {
            vector<string> replicas;
            size_t pos = token.find(CUSTOM_DELIMITER);
            if (pos != string::npos)
            {
                filename = token.substr(0, pos);
                token = token.substr(pos + CUSTOM_DELIMITER.length());
                pos = token.find(CUSTOM_DELIMITER);
                version = token.substr(0, pos);
                token = token.substr(pos + CUSTOM_DELIMITER.length());
                while ((pos = token.find(CUSTOM_DELIMITER)) != std::string::npos)
                {
                    std::string part = token.substr(0, pos);
                    replicas.push_back(part);
                    token.erase(0, pos + CUSTOM_DELIMITER.length());
                }
                replicas.push_back(token);
            }
            cout << "SDFSFilename: " << filename << endl;

            cout << "Version: " << version << endl;
            cout << "Replicas: " << endl;

            for (const string &replica : replicas)
            {
                cout << "\t" << replica << endl;
            }
            cout << endl;
            count++;
        }
    }
    cout << "============ Total File:" << count << "============" << endl;
    return;
}

void FileList::PrintFileStoreLocal()
{
    FileList::LocalMutex.lock();
    cout << "=========Local File list =========" << endl;
    for (const auto &entry : FileList::FileMap)
    {
        const string &filename = entry.first;
        const int version = entry.second;

        cout << "SDFSFilename: " << filename << ", Version: " << version << endl;
    }
    cout << "============ Total File:" << FileList::FileMap.size() << "============" << endl;
    FileList::LocalMutex.unlock();
    return;
}

void FileList::PrintFileLocation(const string filename)
{
    if (!CheckFileExistGlobal(filename))
    {
        cout << filename << " DOES NOT EXIST!" << endl;
    }
    else
    {
        FileList::GlobalMutex.lock();
        cout << filename << " STORE AT:" << endl;
        for (const auto &versionReplicas : FileList::FileMetadataMap[filename]->version_replicaids)
        {
            int version = versionReplicas.first;
            const vector<string> &replicas = versionReplicas.second;
            for (const string &replica : replicas)
            {
                cout << "\t" << replica << endl;
            }
            cout << endl;
        }
        FileList::GlobalMutex.unlock();
    }
}

string FileList::GlobalFile_serialize()
{
    string serializedData;
    FileList::GlobalMutex.lock();
    for (const auto &entry : FileMetadataMap)
    {
        const string &filename = entry.first;
        FileMetadata *metadata = entry.second;

        // Serialize the filename

        serializedData += to_string(FileList::VersionNumber) + CUSTOM_DELIMITER;
        serializedData += filename + CUSTOM_DELIMITER;

        // Serialize the metadata (version_replicaids)
        for (const auto &versionReplica : metadata->version_replicaids)
        {
            int version = versionReplica.first;
            const vector<string> &replicas = versionReplica.second;

            serializedData += to_string(version);

            for (const string &replica : replicas)
            {
                serializedData += CUSTOM_DELIMITER + replica;
            }
        }
        serializedData += "\n";
    }
    FileList::GlobalMutex.unlock();
    return serializedData;
}

bool areVectorsIdentical(const std::vector<std::string> &vec1, const std::vector<std::string> &vec2)
{
    if (vec1.size() != vec2.size())
    {
        return false;
    }
    for (size_t i = 0; i < vec1.size(); ++i)
    {
        if (vec1[i] != vec2[i])
        {
            return false;
        }
    }
    return true;
}

void FileList::CompareGlobalFile(const string &serializedData)
{

    FileList::GlobalMutex.lock();
    istringstream ss(serializedData);
    string token;
    string filename;
    int version_global;
    int version;
    while (getline(ss, token))
    {
        vector<string> replicas;
        size_t pos = token.find(CUSTOM_DELIMITER);
        if (pos != string::npos)
        {
            version_global = stoi(token.substr(0, pos));
            if (version_global > FileList::VersionNumber)
            {
                FileList::VersionNumber = version_global;
            }
            token = token.substr(pos + CUSTOM_DELIMITER.length());
            pos = token.find(CUSTOM_DELIMITER);
            filename = token.substr(0, pos);
            token = token.substr(pos + CUSTOM_DELIMITER.length());
            pos = token.find(CUSTOM_DELIMITER);
            version = stoi(token.substr(0, pos));
            if ((pos = token.find(CUSTOM_DELIMITER)) != std::string::npos)
            {
                token = token.substr(pos + CUSTOM_DELIMITER.length());
                while ((pos = token.find(CUSTOM_DELIMITER)) != std::string::npos)
                {
                    std::string part = token.substr(0, pos);
                    replicas.push_back(part);
                    token.erase(0, pos + CUSTOM_DELIMITER.length());
                }
                replicas.push_back(token);
            }
        }

        if (FileList::FileMetadataMap.count(filename) > 0)
        { // check the version
            int old_version;
            vector<string> old_replica;
            for (const auto &pair : FileList::FileMetadataMap[filename]->version_replicaids)
            {
                old_version = pair.first;
                old_replica = pair.second;
            }
            if (version > old_version)
            {
                FileList::FileMetadataMap[filename]->version_replicaids.erase(version);
                FileMetadata *newmetadata = new FileMetadata;
                newmetadata->version_replicaids[version] = replicas;
                FileList::FileMetadataMap[filename] = newmetadata;
            }
            else if (!areVectorsIdentical(old_replica, replicas))
            {
                FileList::FileMetadataMap[filename]->version_replicaids.erase(version);
                FileMetadata *newmetadata = new FileMetadata;
                newmetadata->version_replicaids[version] = replicas;
                FileList::FileMetadataMap[filename] = newmetadata;
            }
        }
        else
        { // directly insert it
            FileMetadata *newmetadata = new FileMetadata;
            newmetadata->version_replicaids[version] = replicas;
            FileList::FileMetadataMap[filename] = newmetadata;
        }
    }

    // check if there is repeated replica

    FileList::GlobalMutex.unlock();
}

void FileList::DeleteFileLocal(const string filename)
{
    FileList::LocalMutex.lock();
    FileList::FileMap.erase(filename);
    FileList::LocalMutex.unlock();
}

void FileList::DeleteFileGlobal(const string filename)
{
    FileList::GlobalMutex.lock();
    vector<string> replicas;
    int version;
    for (auto &entry : FileList::FileMetadataMap[filename]->version_replicaids)
    {
        version = entry.first;
    }
    FileList::FileMetadataMap[filename]->version_replicaids.erase(version);
    FileMetadata *newmetadata = new FileMetadata;
    newmetadata->version_replicaids[FileList::VersionNumber] = replicas;
    FileList::FileMetadataMap[filename] = newmetadata;
    // ssize_t erase = FileList::FileMetadataMap.erase(filename);
    // if (!erase)
    // {
    //     perror("File not found in File meta map");
    //     exit(1);
    // }
    FileList::GlobalMutex.unlock();
}

unordered_map<string, set<string>> FileList::CheckRecovery(const set<string> faillist) // output: a
{
    // get all the sdfsfile that store in the failed replica server
    unordered_map<string, set<string>> recover_replicas;
    FileList::GlobalMutex.lock();
    for (auto &failserver : faillist)
    {
        recover_replicas[failserver];
        if (FileList::RecoveredFailureServers.find(failserver) != FileList::RecoveredFailureServers.end())
        {
            continue;
        }
        string file_name;
        for (auto &file : FileList::FileMetadataMap)
        {
            file_name = file.first;
            vector<string> replicas;
            for (auto &entry : file.second->version_replicaids)
            {
                int version = entry.first;
                replicas = entry.second;
            }
            if (std::find(replicas.begin(), replicas.end(), failserver) != replicas.end())
            {
                recover_replicas[failserver].insert(file_name);
            }
        }
    }
    FileList::GlobalMutex.unlock();

    return recover_replicas;
}

void FileList::InsertRecover(string server)
{
    FileList::GlobalMutex.lock();
    FileList::RecoveredFailureServers.insert(server);
    FileList::GlobalMutex.unlock();
}

void FileList::DeleteServerGlobal(const string server)
{
    cout << "delete server: " << server << "in global list." << endl;
    FileList::GlobalMutex.lock();
    vector<string> replicas;
    int version;
    for (auto &replica : FileList::FileMetadataMap)
    {
        string filename = replica.first;
        for (auto &entry : FileList::FileMetadataMap[filename]->version_replicaids)
        {
            version = entry.first;
            replicas = entry.second;
            replicas.erase(std::remove(replicas.begin(), replicas.end(), server), replicas.end());
        }
        FileList::FileMetadataMap[filename]->version_replicaids[version] = replicas;
    }
    FileList::GlobalMutex.unlock();
}