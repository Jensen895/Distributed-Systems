# MP4

## Project Description
A adapted version of MapReduce, MapleJuice, combined with SQL commands to run queries on external datasets

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
   
**Maple**  
maple_exe -> the executable for running maple phase.    
maple_num -> the number of maple tasks.   
sdfs_intermediate_prefix_filename -> the prefix of the output file from maple.   
src_directory -> the directory of the input files for maple.   
```
./client maple <maple_exe> <maple_num> <sdfs_intermediate_prefix_filename> <src_directory>  
```  
e.g.
```
./client maple maple_exe.py 10 sdfs_output_key maple_input
```
      
**Juice**  
juice_exe -> the executable for running juice phase.   
juice_num -> the number of juice tasks.    
sdfs_intermediate_prefix_filename -> the prefix for the output files from maple, also the input files for juice.   
dest_filename -> the final output filename.   
delete -> 1 to delete the intermediate files, 0 to not delete.    
```
./client juice <juice_exe> <juice_num> <sdfs_intermediate_prefix_filename> <dest_filename> <delete 0/1>
```
e.g.
```
./client juice juice_exe.py 10 sdfs_output_key MapleJuiceOutput.txt 1
```
