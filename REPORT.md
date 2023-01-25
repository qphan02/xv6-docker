# Project Report

Team Members:
 - Leo Phan
 - Selman Eris
 - Jiyan Ayhan
 - Hamza Khoja

## Module #1: Container Manager
Lead: Leo Phan
 
 - [ ] Level 0: Leo Phan
 - [ ] Level 1: Leo Phan
 - [ ] Level 2: Leo Phan
 - [X] Level 3: Leo Phan
 - [ ] Level 4
 - [ ] Extra Credit

## Module #2: Shared Memory
Lead: Selman Eris
 
 - [ ] Level 0: Selman Eris
 - [ ] Level 1: Selman Eris
 - [ ] Level 2: Selman Eris
 - [X] Level 3: Selman Eris
 - [ ] Extra Credit

## Module #3: Synchronization
Lead: Jiyan Ayhan
 
 - [ ] Level 0: Jiyan Ayhan
 - [ ] Level 1: Jiyan Ayhan
 - [ ] Level 2: Jiyan Ayhan
 - [X] Level 3: Jiyan Ayhan
 - [ ] Extra Credit

## Module #4: Scheduling
 Lead: Hamza Khoja
 
 - [ ] Level 0
 - [ ] Level 1
 - [ ] Level 2
 - [X] Level 3
 - [ ] Level 4

## Overall Project
 
 - [ ] Level 0: Level 1 in four modules.
 - [ ] Level 1: Level 2 in four modules.
 - [X] Level 2: Level 3 in four modules.  Two of the modules must be integrated together.  You must have tests to evaluate this integration.
 - [ ] Level 3: All four modules must be integrated together.  You must have tests to evaluate this integration.
 - [ ] Level 4: Highest level minus 1 in two modules, and highest level in two.
 - [ ] Level Super Saiyan: Highest level in all modules.

# Level Testing Documentation

 - Container Manager  
	- Level 0: just do make run and test like the example in the documentation
	- Level 1: just do make run and test like the example in the documentation
	- Level 2: just do make run and test like the example in the documentation
	- Level 3: just do make run and test like the example in the documentation
 - Shared Memory
 	- Level 0:  
	shm_testlvl1, shm_testlvl2, shm_testlvl3, shm_testfinal, shm_testexec  
	All the test files tests if system calls are implemented correctly and if the data structures are created correctly.
	- Level 1:  
	shm_testlvl3 tests this level. Parent forks a process and both processes call shm_get function with the same name. The child changes the value inside and parent is able to access the value inside after children exits.
	- Level 2:  
	shm_testlvl2 tests this level. The parent calls the shm_get function and puts a value before forking a new process. Then the parent forks a new process. The child is able to see the value inside. Before exiting the process, the child updates the value inside and exits. The parent is able to see the update.
	- Level 3:  
	shm_testlvl2, shm_testfinal, shm_testexec test this level. All the test files test if shared memory is remained or deallocated when the process exits. In exit, all the shared memories that are not removed, gets removed. shm_testfinal tests if correct memory address is deallocated or not. shm_testexec tests if shared memory is maintained across exec and fork. A new process gets forked and that child calls exec function and changes the value inside the shared memory. The parent is still able to access and see the update when child exits.
 - Synchronization
	- Level 0:
	0.c
	testing mutual exclusion
	- Level 1:
	1.c testing mutual exclusion among 3 processes
	- Level 2:
	4.c restaurant - customers analogy for testin condition variables
	- Level 3:
	5.c release mutexes and clear data structures after faulting
 - Scheduling
	- Level 0: 
	  testing that prio_set actually sets priority
	- Level 1:
	  checking if: priority is too high/low, process inherits parent's pid, within ancestry tree 
	- Level 2:
	  enqueue and dequeue are O(1), priority queue, with buckets
	- Level 3:
	  CM is highest priority, all other apps lower priority

