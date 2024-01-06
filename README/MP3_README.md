# MP3

## Project Description
A simple distributed file system that allow clients to write/read/update/delete files in the system.

## Clone
```
git clone git@gitlab.engr.illinois.edu:yy63/mp1.git  
cd mp1  
```

## Compile
Execute the Makefile on project parent directory
```
make
```

## Execute Code
1. Run the server on every machine
```
./server <Port for UDP socket 5000-5010>
```

2. Open another terminal window to enter the commands
```
./client sdfs put|get|delete|ls|store hostname port <necessary args>
```
example:
- VM1 wants to put a local text.txt to sdfs replica.txt 
```
./client sdfs put fa23-cs425-4101.cs.illinois.edu 5001 text.txt replica.txt  
```
- VM5 wants to get a sdfs replica.txt to local text.txt
```
./client sdfs get fa23-cs425-4105.cs.illinois.edu 5005 replica.txt text.txt   
```   
- VM3 wants to delete sdfs replica.txt from the system   
```  
./client sdfs delete fa23-cs425-4103.cs.illinois.edu 5003 replica.txt   
```  
- See the replicas stored on VM2
```
./client sdfs store fa23-cs425-4102.cs.illinois.edu 5002   
```
- See the where the replicas of replica.txt are located at VM7
```
./client sdfs ls fa23-cs425-4107.cs.illinois.edu 5007 replica.txt   
```
