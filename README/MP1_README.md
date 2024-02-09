# MP1



## Project Description
A project that allows the user to query distibuted log files on 10 given machines, from any one of the machine.

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
./server
```

2. Pick a client within one of the machines to query the pattern in every machine's respective log files. (vm1~10.log)
```
./client grep <-r regex> <search string / regex pattern>
```
example:
- Search for specific string, e.g. "Linux"
```
./client grep "Linux"
```
- Search for regular expression pattern, e.g. ".*.com"
```
./client grep -r ".*.com"
```

3. Output
> Result from (IP address):    
> log file from: (file name)       
> matching count: (# of matches)  
> .  
> .  
> .  
> Total Line Count: XXX 

If any of the machines fails or is disconnected, its output would just be left out.

## Unit Testing
```
./client unit_test
```
Generates distributed log files on each machine with frequent, somewhat infrequent and rare patterns.   
If the output from the servers matches the correct line count, the client would output:
> All servers are good!

If any of the servers is disconnected, or fails the unit_test, the client would output:
> (host name of the failed machine) failed!  
> .  
> .
> .
