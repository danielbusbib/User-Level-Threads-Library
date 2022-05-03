#include "uthreads.h"
#include <iostream>
#include <gtest/gtest.h>
#include <random>
#include <algorithm>
#include <regex>

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *                        IMPORTANT
 * Each test, that is, whatever begins with TEST(XXX,XXX),
 * must be ran separately, that is, by clicking on the green button
 * and doing 'Run XXX.XXX'
 *
 * If you run all tests in a file/directory, e.g, by right clicking this .cpp
 * file and doing 'Run all in ...', THE TESTS WILL FAIL
 *
 * The reason is, each test expects a clean slate, that is, it expects that
 * the threads library wasn't initialized yet (and thus will begin with 1 quantum
 * once initialized).
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

//TEST(DO_NOT_RUN_ALL_TESTS_AT_ONCE, READ_THIS)
//{
//  FAIL() << "Do not run all tests at once via CLion, use the provided script" << std::endl;
//}

// conversions to microseconds:
static const int MILLISECOND = 1000;

/** Given a function, expects it to return -1 and that a thread library
 *  error message is printed to stderr
 */
template<class Function>
void expect_thread_library_error (Function function)
{
  testing::internal::CaptureStderr ();
  int retCode = function ();
  std::string errOut = testing::internal::GetCapturedStderr ();
  EXPECT_EQ(retCode, -1) << "failed thread library call should return -1";

  if (errOut.find ("thread library error: ") == std::string::npos)
    {
      ADD_FAILURE() << "no appropriate STDERR message was printed for failed thread library call";
    }
}

/**
 * This function initializes the library with given quantums_num.
 * If it fails, either you didn't implement these correctly,
 * or you ran multiple tests at the same time - see above note.
 */
void initializeLibrary (int lengths)
{
  ASSERT_EQ(uthread_init (lengths), 0);
  // First total number of quantums is always 1 (the main thread's run)
  ASSERT_EQ (uthread_get_total_quantums (), 1);

  if (STACK_SIZE < 8192)
    {
      std::cout << "(NOT AN ERROR) Your STACK_SIZE is " << STACK_SIZE << ", you might want to consider increasing "
                                                                         "                    it to at least 8192\n"
                << "        If you have no trouble with the current stack size, ignore this message. " << std::endl;
    }
}

/**
 * Causes the currently running thread to sleep for a given number
 * of thread-quantums, that is, by counting the thread's own quantums.
 * Unlike "block", this works for the main thread too.
 *
 * @param threadQuants Positive number of quantums to sleep for
 */
void threadQuantumSleep (int threadQuants)
{
  assert (threadQuants > 0);

  int myId = uthread_get_tid ();
  int start = uthread_get_quantums (myId);
  int end = start + threadQuants;
  /* Note, from the thread's standpoint, it is almost impossible for two consecutive calls to
   * 'uthread_get_quantum' to yield a difference larger than 1, therefore, at some point, uthread_get_quantums(myId)
   * must obtain 'end'.
   *
   * Theoretically, it's possible that the thread will be preempted before the condition check occurs, if this happens,
   * the above won't hold, and you'll get an infinite loop. But this is unlikely, as the following operation should
   * take much less than a microsecond
   */
  while (uthread_get_quantums (myId) != end)
    {} // Busy wait... Sleep (suspend) threadQuants quantums
}


/**
 * Testing basic thread library functionality and expected errors
 */
TEST(Test1, BasicFunctionality)
{
  // first, initializing library with invalid parameters should fail
  int invalidQuantums = -1337;
  expect_thread_library_error ([&] ()
                               { return uthread_init (invalidQuantums); });

  // initialize correctly now.
  int quantums_num = 100 * MILLISECOND; // tenth-of-a-second - roughly a million and a half CPU cycles
  initializeLibrary (quantums_num);

  // can't block the main thread
  expect_thread_library_error ([] ()
                               { return uthread_block (0); });

  // main thread has only started one (the current) quantum
  EXPECT_EQ(uthread_get_quantums (0), 1);

  static bool ran = false;
  // most CPP compilers will translate this to a normal function (there's no closure)
  auto t1 = [] ()
  {
      EXPECT_EQ(uthread_get_tid (), 1);

      // this thread has just begun, thus exactly 1 quantum was started by this thread
      EXPECT_EQ(uthread_get_quantums (1), 1);

      // main thread's quantums are unchanged
      EXPECT_EQ(uthread_get_quantums (0), 1);

      // this is the 2nd quantum in the program entire run
      EXPECT_EQ(uthread_get_total_quantums (), 2);

      ran = true; // indicating this thread has been successfully run
      EXPECT_EQ (uthread_terminate (1), 0);
  };
  EXPECT_EQ(uthread_spawn (t1), 1);
  // spawning a thread shouldn't cause a switch
  // while theoretically it's possible that at the end of uthread_spawn
  // we get a preempt-signal, with the quantum length we've specified at the test initialization, it shouldn't happen.
  // (unless your uthread_spawn implementation is very slow, in which case you might want to check that out, or
  //  just increase the quantum length at test initialization)

  // Assuming thread-0 is still running at this phase
  EXPECT_EQ(uthread_get_total_quantums (), 1);
  EXPECT_EQ(uthread_get_quantums (0), 1);

  // see implementation of this function for explanation (basically, leading to a switch to thread-1)
  threadQuantumSleep (1);

  // Check thread-1 run's side-effects
  EXPECT_TRUE(ran);
  EXPECT_EQ(uthread_get_quantums (0), 2);
  EXPECT_EQ(uthread_get_total_quantums (), 3);

  // by now thread 1 was terminated, so operations on it should fail
  expect_thread_library_error ([] ()
                               { return uthread_get_quantums (1); });
  expect_thread_library_error ([] ()
                               { return uthread_block (1); });
  expect_thread_library_error ([] ()
                               { return uthread_resume (1); });
  expect_thread_library_error ([] ()
                               { return uthread_terminate (1); });

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

/** A slightly more complex test, involving two threads, blocking and resuming (jumping to each thread only once) */
TEST(Test2, ThreadSchedulingWithTermination)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  static bool reached_middle = false;
  static bool reached_f = false;

  auto f = [] ()
  {
      EXPECT_TRUE (reached_middle); // Ensure thread-2 has been already run
      reached_f = true;
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  auto g = [] ()
  {
      EXPECT_EQ (uthread_resume (1), 0); // Resume thread-1
      EXPECT_EQ (uthread_terminate (2), 0); // Terminate thread-2
  };

  EXPECT_EQ (uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ (uthread_spawn (g), 2); // Thread-2 is initialized, with g as its entry code
  EXPECT_EQ (uthread_block (1), 0); // Block thread-1

  threadQuantumSleep (1); // Let thread-0 sleep for a quantum
  // since thread-1 is blocked, we expect to go to thread-2, after which we'll go back to the main thread
  reached_middle = true; // Indicating thread-2 has successfully executed g, and is terminated

  // next, we'll go to f, and then get back here (since g was terminataed)
  threadQuantumSleep (1); // Let thread-0 sleep for a quantum
  EXPECT_TRUE (reached_f);

  // in total we had 5 quantums: 0->2->0->1->0
  EXPECT_EQ (uthread_get_total_quantums (), 5);

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}


/** In this test there's a total of 3 threads, including main one,
 *  and a more complicated flow - we alternate between the 3 threads, each one running several times, see
 *  comment below for expected execution order.
 */
TEST(Test3, ThreadExecutionOrder)
{
  int quantums_num = 100 * MILLISECOND;
  initializeLibrary (quantums_num);

  // maps number of passed quantums (ran in each of the loops) to a thread id
  static std::map<int, int> quantumsToTids;

  auto t = [] ()
  {
      /* every thread with this entry point, will block itself
         every iteration, up to 4 iterations, then terminate itself */
      int tid = uthread_get_tid ();
      for (int i = 1; i <= 4; ++i)
        {
          EXPECT_EQ(uthread_get_quantums (tid), i);
          quantumsToTids[uthread_get_total_quantums ()] = tid;
          EXPECT_EQ(uthread_block (tid), 0);
        }
      EXPECT_EQ(uthread_terminate (tid), 0);
  };

  EXPECT_EQ(uthread_spawn (t), 1);
  EXPECT_EQ(uthread_spawn (t), 2);

  // the main thread will also "block" itself every iteration,
  // but "blocking" is done by busy waiting, and it will also resume the other 2 threads
  for (int i = 1; i <= 4; ++i)
    {
      // sanity check
      EXPECT_EQ(uthread_get_tid (), 0);

      // the order in which we resume is only significant if
      // the threads were blocked, in the first iteration,
      // both threads aren't blocked, so this doesn't change
      // their order in the queue
      EXPECT_EQ(uthread_resume (2), 0);
      EXPECT_EQ(uthread_resume (1), 0);
      EXPECT_EQ(uthread_get_quantums (0), i);

      quantumsToTids[uthread_get_total_quantums ()] = 0;
      // during this call, the library should reschedule to thread 2 (if i >= 2) or thread 1 (if i == 1)
      threadQuantumSleep (1);
    }

  // we had a total of 13 quantums (1 at beginning, 4 during each iteration of the main loop, 4*2 for the iterations
  // in each of the two threads loops)
  EXPECT_EQ(uthread_get_total_quantums (), 13);
  quantumsToTids[uthread_get_total_quantums ()] = 0;

  // this illustrates the expected thread execution order
  // 0 -> 1 -> 2 -> 0 -> 2 -> 1 -> 0 -> 2 -> 1 -> 0 -> 2 -> 1 -> 0 -> exit
  //[..][...........][............][.............][...........][.........]
  //start i=1              i=2         i=3              i=4        after loop
  // (indices refer to main thread loop)

  std::map<int, int> expectedQuantumsToTids{
      {1,  0},
      {2,  1},
      {3,  2},
      {4,  0},
      {5,  2},
      {6,  1},
      {7,  0},
      {8,  2},
      {9,  1},
      {10, 0},
      {11, 2},
      {12, 1},
      {13, 0}
  };
  EXPECT_EQ(quantumsToTids, expectedQuantumsToTids);

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}


/** This test involves multiple aspects:
 * - Spawning the maximal amount of threads, all running at the same time
 * - Terminating some of them after they've all spawned and ran at least once
 * - Spawning some again, expecting them to get the lowest available IDs
 */
TEST(Test4, StressTestAndThreadCreationOrder)
{
  // you can increase the quantum length, but even the smallest quantum should work
  int priorities = 1;
  initializeLibrary (priorities);

  // this is volatile, otherwise when compiling in -O2, the compiler considers the waiting loop further below
  // as an infinite loop and optimizes it as such.
  static volatile int ranAtLeastOnce = 0;
  static auto f = [] ()
  {
      ++ranAtLeastOnce;
      while (true)
        {}
  };

  // you can lower this if you're trying to debug, but this should pass as is
  const int SPAWN_COUNT = MAX_THREAD_NUM - 1;
  std::vector<int> spawnedThreads;
  for (int i = 1; i <= SPAWN_COUNT; ++i)
    {
      EXPECT_EQ (uthread_spawn (f), i);
      spawnedThreads.push_back (i);
    }

  // wait for all spawned threads to run at least once
  while (ranAtLeastOnce != SPAWN_COUNT)
    {}

  if (SPAWN_COUNT == MAX_THREAD_NUM - 1)
    {
      // by now, including the 0 thread, we have MAX_THREAD_NUM threads
      // therefore further thread spawns should fail

      expect_thread_library_error ([] ()
                                   { return uthread_spawn (f); });
    }

  // now, terminate 1/3 of the spawned threads (selected randomly)
  std::random_device rd;
  std::shuffle (spawnedThreads.begin (),
                spawnedThreads.end (),
                rd);
  std::vector<int> threadsToRemove (spawnedThreads.begin (),
                                    spawnedThreads.begin () + SPAWN_COUNT * 1 / 3);
  for (const int &tid: threadsToRemove)
    {
      EXPECT_EQ (uthread_terminate (tid), 0);
    }

  // now, we'll spawn an amount of threads equal to those that were terminated
  // we expect the IDs of the spawned threads to equal those that were
  // terminated, but in sorted order from smallest to largest
  std::sort (threadsToRemove.begin (), threadsToRemove.end ());
  for (const int &expectedTid: threadsToRemove)
    {
      EXPECT_EQ (uthread_spawn (f), expectedTid);
    }

  ASSERT_EXIT (uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

class RandomThreadTesting {
  std::set<int> activeThreads;
  int quantums_num = 1;
  std::mt19937 rand;

 public:
  RandomThreadTesting ()
      : activeThreads (),
        rand{std::random_device{} ()}
  {
    initializeLibrary (quantums_num);
  }

  int getRandomActiveThread ()
  {
    auto it = activeThreads.begin ();
    auto dist = std::uniform_int_distribution<> (0, activeThreads.size () - 1);
    std::advance (it, dist (rand));
    assert (it != activeThreads.end ());

    return *it;
  }

  /**
   * Perform random thread library operation, except those involving thread 0
   * @param func Thread function if spawning
   */
  void doOperation (void (*func) ())
  {
    // 50% chance of doing a thread operation(1-5), 50% of not doing anything(6-10)
    // if there are no threads, create new one
    int option = std::uniform_int_distribution<> (1, 10) (rand);
    if (activeThreads.empty () || (option == 1 && activeThreads.size () < MAX_THREAD_NUM - 1))
      {
        int tid = uthread_spawn (func);
        EXPECT_GT(tid, 0);
        activeThreads.emplace (tid);
      }
    else
      {
        int tid = getRandomActiveThread ();

        if (option == 3)
          {
            // terminate thread
            EXPECT_EQ(uthread_terminate (tid), 0);
            activeThreads.erase (tid);
          }
        else if (option == 4)
          {
            // block thread
            EXPECT_EQ(uthread_block (tid), 0);
          }
        else if (option == 5)
          {
            // resume thread
            EXPECT_EQ(uthread_resume (tid), 0);
          }
        else
          {
            // don't perform a thread operation
          }
      } // OOP? open closed principle? ain't nobody got time for that
  }
};

/** This test performs a bunch of random thread library operations, it is used
 *  to detect crashes and other memory errors
 */
TEST(Test5, RandomThreadOperations)
{
  RandomThreadTesting rtt;

  const int ITER_COUNT = 1000;

  auto f = [] ()
  {
      while (true)
        {}
  }; // Busy wait

  for (int i = 0; i < ITER_COUNT; ++i)
    { rtt.doOperation (f); } // Perform ITER_COUNT random library's operations

  ASSERT_EXIT (uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test6, BasicSleep)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums = 5; // You might want to change this value for debugging purposes
  static bool auto_resumed_f = false;

  auto f = [] ()
  {
      EXPECT_EQ(uthread_get_total_quantums (), 2);

      uthread_sleep (sleep_quantums); // Sleep for the next `sleep_quantums` quantums

      EXPECT_EQ(uthread_get_total_quantums (), 2 + 1 + sleep_quantums);

      auto_resumed_f = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_get_total_quantums (), 1);

  threadQuantumSleep (1 + sleep_quantums); /* Let thread-0 busy-wait for a quantum to permit thread-1 to run,
                                                 where thread-1 falls asleep for other sleep_quantums quantums.
                                                 Hence, let the main thread busy-wait for other sleep_quantums quantums,
                                                 till thread-1 is auto-resumed */

  EXPECT_TRUE(auto_resumed_f);

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test7, SleepingCantBeResumed)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums = 5; // You might want to change this value for debugging purposes
  const int resume_after = 2; // You might want to change this value for debugging purposes
  assert (sleep_quantums > resume_after);

  static bool is_f_resumed = false;
  static bool reached_f = false;

  auto f = [] ()
  {
      reached_f = true;
      EXPECT_EQ(uthread_get_total_quantums (), 2);

      uthread_sleep (sleep_quantums); // Sleep for the next `sleep_quantums` quantums

      EXPECT_EQ(uthread_get_total_quantums (), 2 + 1 + sleep_quantums - resume_after);

      is_f_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_get_total_quantums (), 1);

  threadQuantumSleep (1 + sleep_quantums - resume_after);
  /* Let thread-0 busy-wait for a quantum to permit thread-1 to run,
       where thread-1 falls asleep for other sleep_quantums quantums.
       Hence, let the main thread busy-wait for other (sleep_quantums - resume_after) quantums,
       till thread-1 is manually-resumed */
  EXPECT_EQ(uthread_get_total_quantums (), 1 + 1 + 1 + sleep_quantums - resume_after);
  uthread_resume (1);

  EXPECT_TRUE(reached_f);
  EXPECT_FALSE(is_f_resumed);

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test8, SleepingBlocked)
{
  int quantums_num = 100 * MILLISECOND; // capable of roughly 160 hundreds CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums = 5; // You might want to change this value for debugging purposes
  assert(sleep_quantums > 2); // Must be greater than 2 for the test's purposes

  static bool is_f_resumed = false;
  static bool reached_f = false;

  static int offset = 0;

  auto f = [] ()
  {
      reached_f = true;
      EXPECT_EQ(uthread_get_total_quantums (), 2 + offset);

      uthread_sleep (sleep_quantums); // Sleep for the next `sleep_quantums` quantums

      EXPECT_EQ(uthread_get_total_quantums (), 2 + 1 + sleep_quantums + offset);

      is_f_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_get_total_quantums (), 1);

  threadQuantumSleep (1 + sleep_quantums - 2); /* Let thread-0 busy-wait for a quantum to permit thread-1 to run,
                                                 where thread-1 falls asleep for other (sleep_quantums - 2) quantums.
                                                 Hence, let the main thread busy-wait for other sleep_quantums quantums,
                                                 till thread-1 is auto-resumed */
  uthread_block (1); // Two quantums before thread-1 is to be auto-resumed, block it such that it can't wake up
  uthread_resume (1); // Try resuming thread-1, but it has to sleep for one more quantum, so it's expected remain blocked
  threadQuantumSleep (1);

  EXPECT_EQ(uthread_get_total_quantums (), 2 + 1 + sleep_quantums - 1);
  EXPECT_TRUE(reached_f);
  EXPECT_FALSE(is_f_resumed); // Thread-1 is supposed to remain slept, as one more quantum is left for wakening up

  threadQuantumSleep (1);
  EXPECT_TRUE(is_f_resumed);

  /* Another sub-test - sleeping + blocked thread, can't wake up as long as no one resumes it */
  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized again, with f as its entry code
  offset = uthread_get_total_quantums () - 1;
  is_f_resumed = reached_f = false;
  threadQuantumSleep (1 + sleep_quantums - 2); /* Let thread-0 busy-wait for a quantum to permit thread-1 to run,
                                                 where thread-1 falls asleep for other (sleep_quantums - 2) quantums.
                                                 Hence, let the main thread busy-wait for other sleep_quantums quantums,
                                                 till thread-1 is auto-resumed */
  EXPECT_TRUE(reached_f);

  uthread_block (1); // Two quantums before thread-1 is to be auto-resumed, block it such that it never wakes up

  threadQuantumSleep (10); // Let the main thread sleep for some long time, ensuring thread-1 never wakes up
  EXPECT_FALSE(is_f_resumed);

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test9, SleepingBlockedResumed)
{
  int quantums_num = 100 * MILLISECOND; // capable of roughly 160 hundreds CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums = 5; // You might want to change this value for debugging purposes
  const int block_before = 4; // Block thread-1 when this amount of quantums is left for thread-1 to wake up
  assert (sleep_quantums > block_before);

  static bool manually_resumed_f = false;
  static bool reached_f = false;

  static int offset = 0;

  auto f = [] ()
  {
      reached_f = true;
      EXPECT_EQ(uthread_get_total_quantums (), 2 + offset);

      uthread_sleep (sleep_quantums); // Sleep for the next `sleep_quantums` quantums

      EXPECT_EQ(uthread_get_total_quantums (), 2 + 1 + sleep_quantums + offset);

      manually_resumed_f = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_get_total_quantums (), 1);

  threadQuantumSleep (1 + sleep_quantums - block_before);
  /* Let thread-0 busy-wait for a quantum to permit thread-1 to run,
       where thread-1 falls asleep for other sleep_quantums quantums.
       Hence, let the main thread busy-wait for other (sleep_quantums - 2) quantums,
       till thread-1 is blocked and manually-resumed */
  EXPECT_TRUE(reached_f);

  uthread_block (1); // block_before quantums before thread-1 is to be auto-resumed, block it such that it never wakes up
  threadQuantumSleep (block_before);

  EXPECT_EQ(uthread_get_total_quantums (), 2 + 1 + sleep_quantums); // Thread-1's sleeping time expired
  threadQuantumSleep (5); // Let the main thread waste some more time to ensure thread-1 can't run
  EXPECT_FALSE(manually_resumed_f); // No one resumes thread-1, so it won't run

  uthread_resume (1); // Resume thread-1
  offset = 5 + 1; // Sum of previous and next threadQuantumSleep
  threadQuantumSleep (1); // Let the main thread busy-wait, to enable thread-1 to run after the preemption
  EXPECT_TRUE(manually_resumed_f);

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test10, BasicTwoSleep)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums = 5; // You might want to change this value for debugging purposes
  static bool is_f_resumed = false;
  static bool is_g_resumed = false;

  auto f = [] ()
  {
      uthread_sleep (sleep_quantums); // Sleep for the next `sleep_quantums` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums);

      is_f_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  auto g = [] ()
  {
      uthread_sleep (sleep_quantums); // Sleep for the next `sleep_quantums` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums);

      is_g_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (2), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_spawn (g), 2); // Thread-2 is initialized, with g as its entry code

  EXPECT_EQ(uthread_get_total_quantums (), 1);

  threadQuantumSleep (1 + sleep_quantums);
  EXPECT_TRUE(is_f_resumed || is_g_resumed); // At least one of them is supposed to be resumed after this delta

  threadQuantumSleep (1 + sleep_quantums); // Let's be sure they're both already resumed
  EXPECT_TRUE(is_f_resumed && is_g_resumed); // Both of them are supposed to be resumed after this entire delta

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test11, BasicTwoSleep2)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums_f = 5; // You might want to change this value for debugging purposes
  const int sleep_quantums_g = sleep_quantums_f * 2; // You might want to change this value for debugging purposes
  assert(sleep_quantums_f + 1 < sleep_quantums_g); // g may race-condition with the main thread, and win him - avoid it

  static bool is_f_resumed = false;
  static bool is_g_resumed = false;

  auto f = [] ()
  {
      uthread_sleep (sleep_quantums_f); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_f);

      is_f_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  auto g = [] ()
  {
      uthread_sleep (sleep_quantums_g); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_g);

      is_g_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (2), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_spawn (g), 2); // Thread-2 is initialized, with g as its entry code

  EXPECT_EQ(uthread_get_total_quantums (), 1);

  threadQuantumSleep (1 + sleep_quantums_f);
  EXPECT_TRUE(is_f_resumed && !is_g_resumed); // At least one of them is supposed to be resumed after this delta

  threadQuantumSleep (1 + sleep_quantums_g); // Let's be sure they're both already resumed
  EXPECT_TRUE(is_f_resumed && is_g_resumed); // Both of them are supposed to be resumed after this entire delta

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test12, TwoSleepOneBlocked)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums_f = 5; // You might want to change this value for debugging purposes
  const int sleep_quantums_g = sleep_quantums_f * 2; // You might want to change this value for debugging purposes
  assert(sleep_quantums_f + 1 < sleep_quantums_g); // g may race-condition with the main thread, and win him - avoid it

  static bool is_f_resumed = false;
  static bool is_g_resumed = false;

  auto f = [] ()
  {
      uthread_sleep (sleep_quantums_f); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_f);

      is_f_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  auto g = [] ()
  {
      uthread_sleep (sleep_quantums_g); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_g);

      is_g_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (2), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_spawn (g), 2); // Thread-2 is initialized, with g as its entry code

  EXPECT_EQ(uthread_get_total_quantums (), 1);
  uthread_block (2);

  threadQuantumSleep (1 + sleep_quantums_f);
  EXPECT_TRUE(is_f_resumed && !is_g_resumed); // At least one of them is supposed to be resumed after this delta

  threadQuantumSleep (1 + sleep_quantums_g); // Let's be sure they're both already resumed
  EXPECT_TRUE(is_f_resumed && !is_g_resumed); // Both of them are supposed to be resumed after this entire delta

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test13, TwoSleepTwoBlocked)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums_f = 5; // You might want to change this value for debugging purposes
  const int sleep_quantums_g = sleep_quantums_f * 2; // You might want to change this value for debugging purposes
  assert(sleep_quantums_f + 1 < sleep_quantums_g); // g may race-condition with the main thread, and win him - avoid it

  static bool is_f_resumed = false;
  static bool is_g_resumed = false;

  auto f = [] ()
  {
      uthread_sleep (sleep_quantums_f); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_f);

      is_f_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  auto g = [] ()
  {
      uthread_sleep (sleep_quantums_g); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_g);

      is_g_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (2), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_spawn (g), 2); // Thread-2 is initialized, with g as its entry code

  EXPECT_EQ(uthread_get_total_quantums (), 1);
  uthread_block (1);
  uthread_block (2);

  threadQuantumSleep (1 + sleep_quantums_f);
  EXPECT_TRUE(!is_f_resumed && !is_g_resumed); // At least one of them is supposed to be resumed after this delta

  threadQuantumSleep (1 + sleep_quantums_g); // Let's be sure they're both already resumed
  EXPECT_TRUE(!is_f_resumed && !is_g_resumed); // Both of them are supposed to be resumed after this entire delta

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test14, TwoSleepOneBlockedResumed)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums_f = 5; // You might want to change this value for debugging purposes
  const int sleep_quantums_g = sleep_quantums_f * 2; // You might want to change this value for debugging purposes
  assert(sleep_quantums_f + 1 < sleep_quantums_g); // g may race-condition with the main thread, and win him - avoid it

  static bool is_f_resumed = false;
  static bool is_g_resumed = false;

  auto f = [] ()
  {
      uthread_sleep (sleep_quantums_f); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_f);

      is_f_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  auto g = [] ()
  {
      uthread_sleep (sleep_quantums_g); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_g);

      is_g_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (2), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_spawn (g), 2); // Thread-2 is initialized, with g as its entry code

  EXPECT_EQ(uthread_get_total_quantums (), 1);
  uthread_block (2);

  threadQuantumSleep (1 + sleep_quantums_f);
  EXPECT_TRUE(is_f_resumed && !is_g_resumed); // At least one of them is supposed to be resumed after this delta

  uthread_resume (2);
  threadQuantumSleep (1 + sleep_quantums_g); // Let's be sure they're both already resumed
  EXPECT_TRUE(is_f_resumed && is_g_resumed); // Both of them are supposed to be resumed after this entire delta

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test15, TwoSleepTwoBlockedResumed)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const int sleep_quantums_f = 5; // You might want to change this value for debugging purposes
  const int sleep_quantums_g = sleep_quantums_f * 2; // You might want to change this value for debugging purposes
  assert(sleep_quantums_f + 1 < sleep_quantums_g); // g may race-condition with the main thread, and win him - avoid it

  static bool is_f_reached = false;
  static bool is_g_reached = false;

  static bool is_f_resumed = false;
  static bool is_g_resumed = false;

  auto f = [] ()
  {
      is_f_reached = true;

      uthread_sleep (sleep_quantums_f); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_f);

      is_f_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (1), 0);
  };

  auto g = [] ()
  {
      is_g_reached = true;

      uthread_sleep (sleep_quantums_g); // Sleep for the next `sleep_quantums_f` quantums

      EXPECT_TRUE(uthread_get_total_quantums () > sleep_quantums_g);

      is_g_resumed = true; // Indicating successful sleep and resume
      EXPECT_EQ (uthread_terminate (2), 0);
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_spawn (g), 2); // Thread-2 is initialized, with g as its entry code

  EXPECT_EQ(uthread_get_total_quantums (), 1);
  uthread_block (1);
  uthread_block (2);

  threadQuantumSleep (1);
  EXPECT_TRUE(!is_f_reached && !is_g_reached);

  uthread_resume (1);
  threadQuantumSleep (1);
  EXPECT_TRUE(is_f_reached && !is_g_reached); // At least one of them is supposed to be resumed after this delta

  uthread_resume (2);
  threadQuantumSleep (2 * (1 + sleep_quantums_g)); // Let's be sure they're both already resumed
  EXPECT_TRUE(is_f_resumed && is_g_resumed); // Both of them are supposed to be resumed after this entire delta

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}

TEST(Test16, BasicSleepTerminated)
{
  int quantums_num = MILLISECOND; // capable of roughly 1600 CPU-cycles per-thread
  initializeLibrary (quantums_num); // Thread-0 is initialized

  const static int sleep_quantums = 5; // You might want to change this value for debugging purposes
  static bool is_reached_f = false;
  static bool auto_resumed_f = false;

  auto f = [] ()
  {
      EXPECT_EQ(uthread_get_total_quantums (), 2);
      is_reached_f = true;

      uthread_sleep (sleep_quantums); // Sleep for the next `sleep_quantums` quantums

      EXPECT_EQ(uthread_get_total_quantums (), sleep_quantums);

      auto_resumed_f = true; // Indicating successful sleep and resume
  };

  EXPECT_EQ(uthread_spawn (f), 1); // Thread-1 is initialized, with f as its entry code
  EXPECT_EQ(uthread_get_total_quantums (), 1);

  threadQuantumSleep (1); /* Let thread-0 busy-wait for a quantum to permit thread-1 to run,
                             where thread-1 falls asleep for other sleep_quantums quantums. */
  EXPECT_TRUE(is_reached_f);

  EXPECT_EQ(uthread_terminate (1), 0); // Kill him in his sleep
  threadQuantumSleep (sleep_quantums + 1); // Let's enable him to wake up...

  EXPECT_FALSE(auto_resumed_f);

  ASSERT_EXIT(uthread_terminate (0), ::testing::ExitedWithCode (0), "");
}