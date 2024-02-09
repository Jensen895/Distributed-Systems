# MP2



## Project Description
A gossip style failure detector that monitor distributed machines and checks if they are active or failed.  
### Normal Mode
Sets a parameter for Tfail, which is the time to determines if a node has failed if its heartbeat has not increase during the time interval.
### Suspicion Mode
Each node now gossips a new information called incarnation number associated with each process which can only be incremented by the node itself. When a node suspect some node to be failed, the node sets the process's state as Suspected, and increment its incarnation number. After a time interval, Tsus, if the node doesn't receive a greater incarnation number of the process with the state: Alive, it fails that process.

## Clone
```
git clone git@github.com:Jensen895/Distributed-Systems.git  
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
./mp2_demo <username> <machine number 00-10> <-j join|-p print_list|-s suspicion mode|-l leave>
```
example:
- VM1 wants to join (Note: VM1 is the introducer, must be the first to join)
```
./mp2_demo <username> 01 -j  
```
- Print VM10's membership list  
```
./mp2_demo <username> 10 -p  
```   
- VM3 wants to leave the cluster  
```  
./mp2_demo <username> 03 -l    
```  
- Change to suspicion mode
```
./mp2_demo <username> <any machine will do> -s
```
