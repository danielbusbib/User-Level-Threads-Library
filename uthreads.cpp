#include "uthreads.h"
#include <queue>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>
#include <set>
#include <list>
#include <map>
#include <iostream>

#define PRINT_ERROR_MSG(s) fprintf(stderr,"thread library error: %s \n", s);
#define SYSTEM_ERROR_MSG(s) fprintf(stderr,"system error: %s \n", s);
#define TIMER_ERROR "Set timer error."
#define ACTION_ERROR "sigaction error."
#define FAILURE -1


//--------------------------------------------------------------------
#define SIGNAL_BLOCK if (sigprocmask(SIG_BLOCK, &set, NULL) < 0)\
{\
  SYSTEM_ERROR_MSG(ACTION_ERROR);\
  exit(1);\
}

#define SIGNAL_UNBLOCK if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0)\
{\
SYSTEM_ERROR_MSG(ACTION_ERROR);\
exit(1);\
}

#define TIMER_SET_UP if (setitimer(ITIMER_VIRTUAL, &timer, NULL)==-1)\
{\
SYSTEM_ERROR_MSG(TIMER_ERROR)\
  exit(1);\
}

using std::list;
enum STATE {
    READY = 0, RUNNING, BLOCKED
};
typedef unsigned long address_t;
sigset_t set;
int total_quantums = 0;
struct sigaction sa;
struct itimerval timer;

typedef struct Thread {
    STATE st;
    char stack[STACK_SIZE];
    int quantum;
    sigjmp_buf env;
} Thread;

std::map<size_t, Thread *> threads;
std::list<size_t> ready_list;
std::map<size_t, std::pair<int, STATE>> blocked_map;
int running_thread;


//--------------------------------------------------------------
/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address (address_t addr)
{
  address_t ret;
  asm volatile("xor    %%fs:0x30,%0\n"
               "rol    $0x11,%0\n"
  : "=g" (ret)
  : "0" (addr));
  return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
  address_t ret;
  asm volatile("xor    %%gs:0x18,%0\n"
               "rol    $0x9,%0\n"
               : "=g" (ret)
               : "0" (addr));
  return ret;
}


#endif

//--------------------------------------------------------------

Thread *new_thread (STATE st, address_t pc, size_t q)
{
  Thread *t = (Thread *) calloc (1, sizeof (Thread));
  if (!t)
    {
      SYSTEM_ERROR_MSG("ERROR CREATE NEW THREAD");
      exit (1);
    }
  t->st = st;
  t->quantum = q;
  // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
  // siglongjmp to jump into the thread.
  address_t sp = (address_t) t->stack + STACK_SIZE - sizeof (address_t);
  sigsetjmp(t->env, 1);

  (t->env->__jmpbuf)[JB_SP] = translate_address (sp);
  (t->env->__jmpbuf)[JB_PC] = translate_address (pc);
  if (sigemptyset (&t->env->__saved_mask) == -1)
    {
      SYSTEM_ERROR_MSG("SIG EMPTY SET ERROR");
      exit (1);
    }
  return t;

}

int find_pid ()
{
  for (auto &val: threads)
    {
      if (val.first != 0 && val.second == nullptr) return (int) val.first;
    }
  return -1;
}

void init_map ()
{
  for (size_t i = 0; i < MAX_THREAD_NUM; ++i) threads[i] = nullptr;
}

// update timer, wake up sleep threads
void timer_reset ()
{
  ++total_quantums;

  // sleep
  std::list<int> lst;
  for (auto &v : blocked_map)
    {
      int q_time = v.second.first;
      STATE from_state = v.second.second;
      if (q_time == -1) continue;

      if (q_time <= total_quantums)
        {
          if (from_state == READY)
            {
              threads[v.first]->st = READY;
              ready_list.push_back (v.first);
              lst.push_back (v.first);
            }
          else if (from_state == BLOCKED)
            {
              blocked_map[v.first].first = -1;
              blocked_map[v.first].second = READY;
            }
        }
    }
  for (auto &i:lst)
    {
      blocked_map.erase (i);
    }

  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr) < 0)
    {
      PRINT_ERROR_MSG(TIMER_ERROR)\
      exit (1);
    }
}

/**
 * delete all alloc on map threads
 */
void terminate_all ()
{
  for (auto &val: threads)
    {
      if (val.second)
        {
          free (val.second);
          val.second = nullptr;
        }
    }
  threads.clear ();
}

/**
 * set running thread
 */
void next_thread ()
{
  running_thread = ready_list.front ();
  ready_list.pop_front ();

  ++threads[running_thread]->quantum;
  threads[running_thread]->st = RUNNING;
  SIGNAL_UNBLOCK;
  // longjmp
  siglongjmp (threads[running_thread]->env, 1);
}

/**
 * The Round-Robin scheduling
 * @param sig
 */
void switch_threads (int sig)
{
  SIGNAL_BLOCK;
  timer_reset ();

  int ret_val = sigsetjmp(threads[running_thread]->env, 1);
  if (ret_val == 1)
    {
      return;
    }
  threads[running_thread]->st = READY;
  ready_list.push_back (running_thread);

  // while
  next_thread ();
}

//------------------------------------------------------

/**
 * @brief initializes the thread library.
 *
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init (int quantum_usecs)
{
  if (quantum_usecs <= 0)
    {
      PRINT_ERROR_MSG("It is an error to call"
                      " this function with non-positive quantum_usecs.");
      return FAILURE;
    }
  init_map ();
  sa.sa_handler = switch_threads;

  if (sigaction (SIGVTALRM, &sa, nullptr) < 0)
    {
      SYSTEM_ERROR_MSG(ACTION_ERROR)
      exit (EXIT_FAILURE);
    }
  sigemptyset (&set);
  sigaddset (&set, SIGVTALRM);

  // INITIALIZE MAIN THREAD 0
  running_thread = 0;
  Thread *t = new_thread (RUNNING, (address_t) 0, 1);
  threads[0] = t;
  ++total_quantums;
  timer.it_value.tv_sec = quantum_usecs / 1000000;        // first time interval, seconds part
  timer.it_value.tv_usec = quantum_usecs % 1000000;        // first time interval, microseconds part

  timer.it_interval.tv_sec = quantum_usecs / 1000000;    // following time intervals, seconds part
  timer.it_interval.tv_usec = quantum_usecs % 1000000;
  TIMER_SET_UP;
  return 0;
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn (thread_entry_point entry_point)
{
  SIGNAL_BLOCK;
  if (!entry_point)
    {
      PRINT_ERROR_MSG("ENTRY POINT FUNC IS NULL.")
      return FAILURE;
    }
  int id = find_pid ();
  if (id == -1)
    {
      PRINT_ERROR_MSG("EXCEED MAX NUM OF THREADS")
      SIGNAL_UNBLOCK;
      return FAILURE;
    }
  Thread *t = new_thread (READY, (address_t) entry_point, 0);
  threads[id] = t;
  ready_list.push_back (id);
  SIGNAL_UNBLOCK;
  return id;
}

/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate (int tid)
{
  SIGNAL_BLOCK;
  if (tid == 0)
    {
      terminate_all ();
      exit (0);
    }

  if (tid < 0 || !threads[tid])
    {
      PRINT_ERROR_MSG("Can't terminate invalid id thread");
      SIGNAL_UNBLOCK;
      return FAILURE;
    }
  free (threads[tid]);
  threads[tid] = nullptr;
  ready_list.remove (tid);
  if (blocked_map.find (tid) != blocked_map.end ())
    {
      blocked_map.erase (tid);
    }
  if (running_thread == tid)
    {
      // self terminated
      timer_reset ();
      next_thread ();
    }
  SIGNAL_UNBLOCK;
  return 0;
}

/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is not considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block (int tid)
{
  SIGNAL_BLOCK;

  if (tid <= 0 || tid > MAX_THREAD_NUM || !threads[tid])
    {
      PRINT_ERROR_MSG("THREAD BLOCK - INVALID INPUT TID");
      SIGNAL_UNBLOCK;
      return FAILURE;
    }
  if (threads[tid]->st == BLOCKED)
    {
      SIGNAL_UNBLOCK;
      return 0;
    }
  else if (blocked_map.find (tid) != blocked_map.end () && blocked_map[tid].first != -1) // SLEEPING
    {
      blocked_map[tid].second = BLOCKED;
    }
  else
    {
      // remove from ready list
      ready_list.remove (tid);
      // set state

      threads[tid]->st = BLOCKED;
      if (blocked_map.find (tid) == blocked_map.end ())
        {
          blocked_map[tid].first = -1;
          blocked_map[tid].second = READY;
        }
      // case - block running thread
      if (tid == running_thread)
        {
          int ret_val = sigsetjmp(threads[running_thread]->env, 1);
          if (ret_val == 1)
            {
              SIGNAL_UNBLOCK;
              return 0;
            }
          timer_reset ();
          next_thread ();
        }

    }

  SIGNAL_UNBLOCK;
  return 0;

}

/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume (int tid)
{
  SIGNAL_BLOCK;
  if (!threads[tid])
    {
      PRINT_ERROR_MSG("RESUME THREAD INVALID TID");
      SIGNAL_UNBLOCK;
      return FAILURE;
    }
  // BLOCKED STATE
  if (blocked_map.find (tid) != blocked_map.end () && blocked_map[tid].first == -1)
    {
      threads[tid]->st = READY;
      ready_list.push_back (tid);
      blocked_map.erase (tid);
    }
  // SLEEPING STATE
  if (blocked_map.find (tid) != blocked_map.end () && blocked_map[tid].first != -1)
    {
      //sleep
      blocked_map[tid].second = READY;
    }
  SIGNAL_UNBLOCK;
  return 0;
}

/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * It is considered an error if the main thread (tid==0) calls this function.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep (int num_quantums)
{
  SIGNAL_BLOCK;
  if (num_quantums <= 0)
    {
      PRINT_ERROR_MSG("NUM QUANTUMS OF SLEEP MUST BE POSITIVE");
      SIGNAL_UNBLOCK;
      return FAILURE;
    }
  if (running_thread == 0)
    {
      PRINT_ERROR_MSG("Main Thread call sleep");
      SIGNAL_UNBLOCK;
      return FAILURE;
    }
  blocked_map[running_thread].first = total_quantums + num_quantums + 1;
  blocked_map[running_thread].second = READY;

  ready_list.remove (running_thread);
//  switch_threads (0);
  int ret_val = sigsetjmp(threads[running_thread]->env, 1);
  if (ret_val == 1)
    {
      SIGNAL_UNBLOCK;
      return 0;
    }
  timer_reset ();
  next_thread ();
  SIGNAL_UNBLOCK;
  return 0;
}

/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid ()
{
  return running_thread;
}

/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums ()
{
  return total_quantums;
}

/**
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums (int tid)
{
  SIGNAL_BLOCK;
  if (!threads[tid])
    {
      PRINT_ERROR_MSG("GET QUANTUM INVALID TID");
      SIGNAL_UNBLOCK;
      return FAILURE;
    }

  Thread *t = threads[tid];
  SIGNAL_UNBLOCK;
  return t->quantum;
}

