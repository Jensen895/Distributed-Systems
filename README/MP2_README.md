# MP2



## Project Description
A gossip style failure detector that monitor distributed machines and checks if they are active or failed.

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
- VM3 wants to leave   
```  
./mp2_demo <username> 03 -l    
```  
- Change to suspicion mode
```
./mp2_demo <username> <any machine will do> -s
```
