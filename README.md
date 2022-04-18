# User-Level-Threads-Library
Implementation of User Level Threads, including switching between threads

### Thread States
-----------
![](https://www.d.umn.edu/~gshute/os/images/states.png)

### Algorithm
-----------
**The Round-Robin scheduling policy should work as follows:**

  1. Every time a thread is moved to RUNNING state, it is allocated a predefined number of microseconds to run. This time interval is called a **quantum**
  
  2. The RUNNING thread is preempted if any of the following occurs:  
    
    
     * `Its quantum expires.`  
     * `Changed its state to BLOCKED and is consequently waiting for an event (i.e. some other
    thread that will resume it or after some time has passed – more details below).`  
     * `It is terminated.`
     
 
 3. When the RUNNING thread is preempted, do the following:
 
 
    * `If it was preempted because its quantum has expired, move it to the end of the READY threads list.`\
    * `Move the next thread in the queue of READY threads to RUNNING state`\
    * `Every time a thread moves to the READY state from any other state, it is placed at the end of the list of READY threads`
    
    
    When a thread doesn't finish its quantum (as in the case of a thread that blocks itself), the next
    thread should start executing immediately as if the previous thread finished its quota.
    In the following illustration the quantum was set for 2 seconds, Thread 1 blocks itself after running
    only for 1 second and Thread 2 immediately starts its next quantum.

