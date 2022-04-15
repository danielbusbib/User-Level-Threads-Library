//
// Created by ehudbartfeld on 05/04/2022.
//


#include "uthreads.h"
#include <stdio.h>
#define MAX_THREAD_TEST 10
void check_terminate();
void make_threads(int num,thread_entry_point entry_point ){
    int id;
    for (int i = 0; i < num; ++i) {
        id = uthread_spawn(entry_point);
        printf("make thread :  id = %d\n",id);
    }
}
int next_time(int runtime){
    if (runtime == uthread_get_quantums(uthread_get_tid())){
        return 1;
    }
    return 0;
}
void delete_and_make_check();

void test_deleting_threads();

/*
void yield(void)
{
  int ret_val = sigsetjmp(env[current_thread], 1);
  printf("yield: ret_val=%d\n", ret_val);
  bool did_just_save_bookmark = ret_val == 0;
  //    bool did_jump_from_another_thread = ret_val != 0;
  if (did_just_save_bookmark)
  {
    jump_to_thread(1 - current_thread);
  }

}
 */
void thread1_sleep_check(void)
{
  int j = 0;
  while (1)
  {
    ++j;
      if (j % 100 == 0)
      {
          j =0;
          printf("id = %d \n",uthread_get_tid());
          fflush (stdout);
          uthread_sleep (2);
      }
  }
}
void thread1_swap_check(void)
{
    int j = 0;
    int time =uthread_get_quantums(uthread_get_tid());
    while (1)
    {
        ++j;
        if (j % 100 == 0)
        {
            j =0;
            printf("id = %d \n",uthread_get_tid());
            fflush (stdout);
            while (next_time(time) == 1){}
            time++;
        }
    }
}
void thread2_swap_check(void)
{
    int i = 0;
    int time = uthread_get_quantums(uthread_get_tid());
    while (1)
    {
        ++i;
        if (i % 100 == 0)
        {
            printf("sleep check : id = %d \n",uthread_get_tid());
            fflush (stdout);
            while (next_time(time) == 1){}
        }
    }
}


void thread2_sleep_check(void)
{
  int i = 0;
  while (1)
  {
    ++i;
      if (i % 100 == 0)
      {
          printf("sleep check : id = %d \n",uthread_get_tid());
          fflush (stdout);
          uthread_sleep (2);
      }
  }
}

void thread1_block_check(void)
{
    int i = 0;
    while (1)
    {
        ++i;
        if (i % 100 == 0)
        {
            printf("id = %d \n",uthread_get_tid());
            fflush (stdout);
            uthread_resume(uthread_get_tid()+1);
            uthread_block(uthread_get_tid());
        }
    }
}

void thread2_block_check(void)
{
    int i = 0;
    while (1)
    {
        ++i;
        if (i % 100 == 0)
        {
            printf("id = %d \n",uthread_get_tid());
            fflush (stdout);
            uthread_resume(uthread_get_tid()-1);
            uthread_block(uthread_get_tid());
        }
    }
}
void thread1_block_check_time(void)
{
    int i = 0;
    while (1)
    {
        ++i;
        if (i % 100 == 0)
        {
            printf("id = %d ",uthread_get_tid());
            printf("and I am running for = %d  quantums \n",uthread_get_quantums(uthread_get_tid()));
            fflush (stdout);
            uthread_resume(uthread_get_tid()+1);
            uthread_block(uthread_get_tid());
        }
    }
}
void base_tarminate_itself(void)
{
            printf("id = %d ",uthread_get_tid());
            printf("and I am running for = %d  quantums \n",uthread_get_quantums(uthread_get_tid()));
            fflush (stdout);
            uthread_terminate(uthread_get_tid());
}
void thread2_block_check_time(void)
{
    int i = 0;
    while (1)
    {
        ++i;
        if (i % 100 == 0)
        {
            printf("id = %d ",uthread_get_tid());
            printf("and I am running for = %d  quantums \n",uthread_get_quantums(uthread_get_tid()));
            fflush (stdout);
            uthread_resume(uthread_get_tid()-1);
            uthread_block(uthread_get_tid());
        }
    }
}
void endless_run(){
    int time = 0 ;
    while (1){
        if(uthread_get_total_quantums() > time){
            printf("the current Quantum is : %d\n",uthread_get_total_quantums());
            time = uthread_get_total_quantums();
        }
    }
}
void quantums_to_run(int num){
    int time = 0 ;
    while (time < num){
        if(uthread_get_total_quantums() > time){
            printf("the current Quantum is : %d\n",uthread_get_total_quantums());
            time = uthread_get_total_quantums();
        }
    }
}
void endless_run_no_print(){
    while (1){}
    }
    /*
make thread :  id = 1
make thread :  id = 2
the current Quantum is : 1
id = 1
id = 2
the current Quantum is : 4
id = 1
the current Quantum is : 6
id = 2
the current Quantum is : 8
id = 1
the current Quantum is : 10
uthread_terminate id 0
Process finished with exit code 0
     */
void basic_sleep_check(){
    uthread_init(1);
    make_threads(2 , thread1_sleep_check);
    quantums_to_run(10);
    printf("uthread_terminate id 0");
    uthread_terminate(uthread_get_tid());
//    int a=  uthread_spawn (thread1_sleep_check);
//    int b = uthread_spawn (thread2_sleep_check);
}
/*
the current Quantum is : 1
id = 1
id = 2
the current Quantum is : 4
id = 1
the current Quantum is : 6
id = 2
the current Quantum is : 8
id = 1
the current Quantum is : 10
uthread_terminate id 0
Process finished with exit code 0
 */
void basic_block_check(){
    uthread_init(1);
    int a=  uthread_spawn (thread1_block_check);
    int b = uthread_spawn (thread2_block_check);
    quantums_to_run(10);
    printf("uthread_terminate id 0");
    uthread_terminate(uthread_get_tid());
}
/*
 make thread :  id = 1
make thread :  id = 2
make thread :  id = 3
make thread :  id = 4
make thread :  id = 5
make thread :  id = 6
make thread :  id = 7
make thread :  id = 8
make thread :  id = 9
make thread :  id = 10
make thread :  id = 11
make thread :  id = 12
make thread :  id = 13
make thread :  id = 14
make thread :  id = 15
make thread :  id = 16
make thread :  id = 17
make thread :  id = 18
make thread :  id = 19
make thread :  id = 20
make thread :  id = 21
make thread :  id = 22
make thread :  id = 23
make thread :  id = 24
make thread :  id = 25
make thread :  id = 26
make thread :  id = 27
make thread :  id = 28
make thread :  id = 29
make thread :  id = 30
make thread :  id = 31
make thread :  id = 32
make thread :  id = 33
make thread :  id = 34
make thread :  id = 35
make thread :  id = 36
make thread :  id = 37
make thread :  id = 38
make thread :  id = 39
make thread :  id = 40
make thread :  id = 41
make thread :  id = 42
make thread :  id = 43
make thread :  id = 44
make thread :  id = 45
make thread :  id = 46
make thread :  id = 47
make thread :  id = 48
make thread :  id = 49
make thread :  id = 50
make thread :  id = 51
make thread :  id = 52
make thread :  id = 53
make thread :  id = 54
make thread :  id = 55
make thread :  id = 56
make thread :  id = 57
make thread :  id = 58
make thread :  id = 59
make thread :  id = 60
make thread :  id = 61
make thread :  id = 62
make thread :  id = 63
make thread :  id = 64
make thread :  id = 65
make thread :  id = 66
make thread :  id = 67
make thread :  id = 68
make thread :  id = 69
make thread :  id = 70
make thread :  id = 71
make thread :  id = 72
make thread :  id = 73
make thread :  id = 74
make thread :  id = 75
make thread :  id = 76
make thread :  id = 77
make thread :  id = 78
make thread :  id = 79
make thread :  id = 80
make thread :  id = 81
make thread :  id = 82
make thread :  id = 83
make thread :  id = 84
make thread :  id = 85
make thread :  id = 86
make thread :  id = 87
make thread :  id = 88
make thread :  id = 89
make thread :  id = 90
make thread :  id = 91
make thread :  id = 92
make thread :  id = 93
make thread :  id = 94
make thread :  id = 95
make thread :  id = 96
make thread :  id = 97
make thread :  id = 98
make thread :  id = 99
make thread :  id = -1
the current Quantum is : 1
sleep check : id = 1
sleep check : id = 2
sleep check : id = 3
sleep check : id = 4
sleep check : id = 5
sleep check : id = 6
sleep check : id = 7
sleep check : id = 8
sleep check : id = 9
sleep check : id = 10
sleep check : id = 11
sleep check : id = 12
sleep check : id = 13
sleep check : id = 14
sleep check : id = 15
sleep check : id = 16
sleep check : id = 17
sleep check : id = 18
sleep check : id = 19
sleep check : id = 20
sleep check : id = 21
sleep check : id = 22
sleep check : id = 23
sleep check : id = 24
sleep check : id = 25
sleep check : id = 26
sleep check : id = 27
sleep check : id = 28
sleep check : id = 29
sleep check : id = 30
sleep check : id = 31
sleep check : id = 32
sleep check : id = 33
sleep check : id = 34
sleep check : id = 35
sleep check : id = 36
sleep check : id = 37
sleep check : id = 38
sleep check : id = 39
sleep check : id = 40
sleep check : id = 41
sleep check : id = 42
sleep check : id = 43
sleep check : id = 44
sleep check : id = 45
sleep check : id = 46
sleep check : id = 47
sleep check : id = 48
sleep check : id = 49
sleep check : id = 50
sleep check : id = 51
sleep check : id = 52
sleep check : id = 53
sleep check : id = 54
sleep check : id = 55
sleep check : id = 56
sleep check : id = 57
sleep check : id = 58
sleep check : id = 59
sleep check : id = 60
sleep check : id = 61
sleep check : id = 62
sleep check : id = 63
sleep check : id = 64
sleep check : id = 65
sleep check : id = 66
sleep check : id = 67
sleep check : id = 68
sleep check : id = 69
sleep check : id = 70
sleep check : id = 71
sleep check : id = 72
sleep check : id = 73
sleep check : id = 74
sleep check : id = 75
sleep check : id = 76
sleep check : id = 77
sleep check : id = 78
sleep check : id = 79
sleep check : id = 80
sleep check : id = 81
sleep check : id = 82
sleep check : id = 83
sleep check : id = 84
sleep check : id = 85
sleep check : id = 86
sleep check : id = 87
sleep check : id = 88
sleep check : id = 89
sleep check : id = 90
sleep check : id = 91
sleep check : id = 92
sleep check : id = 93
sleep check : id = 94
sleep check : id = 95
sleep check : id = 96
sleep check : id = 97
sleep check : id = 98
sleep check : id = 99
the current Quantum is : 101
uthread_terminate id 0thread library error:  max amount of threads reached :(
 */
void get_limit_error(){
    uthread_init(1);
    make_threads(MAX_THREAD_NUM,thread2_sleep_check);
    quantums_to_run(10);
    printf("uthread_terminate id 0");
    uthread_terminate(uthread_get_tid());
}
/*
the current Quantum is : 1
id = 1 and I am running for = 1  quantums
id = 2 and I am running for = 1  quantums
the current Quantum is : 4
id = 1 and I am running for = 2  quantums
the current Quantum is : 6
id = 2 and I am running for = 2  quantums
the current Quantum is : 8
id = 1 and I am running for = 3  quantums
the current Quantum is : 10
uthread_terminate id 0
Process finished with exit code 0
 */
void check_uthread_get_quantums(){
    uthread_init(1);
    int a=  uthread_spawn (thread1_block_check_time);
    int b = uthread_spawn (thread2_block_check_time);
    quantums_to_run(10);
    printf("uthread_terminate id 0");
    uthread_terminate(uthread_get_tid());
}
/*
make thread :  id = 1
make thread :  id = 2
make thread :  id = 3
make thread :  id = 4
make thread :  id = 5
make thread :  id = 6
make thread :  id = 7
make thread :  id = 8
make thread :  id = 9
the current Quantum is : 1
id = 1 and I am running for = 1  quantums
id = 2 and I am running for = 1  quantums
id = 3 and I am running for = 1  quantums
id = 4 and I am running for = 1  quantums
id = 5 and I am running for = 1  quantums
id = 6 and I am running for = 1  quantums
id = 7 and I am running for = 1  quantums
id = 8 and I am running for = 1  quantums
id = 9 and I am running for = 1  quantums
the current Quantum is : 11
uthread_terminate id 0
Process finished with exit code 0
 */
void check_uthread_terminate(){
    uthread_init(1);
    make_threads(MAX_THREAD_TEST -1 , base_tarminate_itself);
    quantums_to_run(10);
    printf("uthread_terminate id 0");
    uthread_terminate(uthread_get_tid());
}

void check_terminate(int from , int to) {
    int time = 0 ;
    for (int i = from; i <= to; ++i) {
        printf("about to delete id = %d \n", i);
        fflush (stdout);
        uthread_terminate(i);
        time = uthread_get_quantums(uthread_get_tid());
        while (next_time(time) == 1){}
    }
}
/*
make thread :  id = 1
make thread :  id = 2
make thread :  id = 3
make thread :  id = 4
make thread :  id = 5
make thread :  id = 6
make thread :  id = 7
make thread :  id = 8
make thread :  id = 9
about to delete id = 1
sleep check : id = 2
sleep check : id = 3
sleep check : id = 4
sleep check : id = 5
sleep check : id = 6
sleep check : id = 7
sleep check : id = 8
sleep check : id = 9
about to delete id = 2
sleep check : id = 3
sleep check : id = 4
sleep check : id = 5
sleep check : id = 6
sleep check : id = 7
sleep check : id = 8
about to delete id = 3
sleep check : id = 9
sleep check : id = 4
sleep check : id = 5
sleep check : id = 6
sleep check : id = 7
about to delete id = 4
sleep check : id = 8
sleep check : id = 9
sleep check : id = 5
sleep check : id = 6
about to delete id = 5
sleep check : id = 7
sleep check : id = 8
sleep check : id = 9
make thread :  id = 1
make thread :  id = 2
make thread :  id = 3
make thread :  id = 4
make thread :  id = 5
about to delete id = 1
sleep check : id = 6
sleep check : id = 7
sleep check : id = 2
sleep check : id = 3
sleep check : id = 4
sleep check : id = 5
sleep check : id = 8
about to delete id = 2
sleep check : id = 9
sleep check : id = 6
sleep check : id = 7
sleep check : id = 3
sleep check : id = 4
sleep check : id = 5
about to delete id = 3
sleep check : id = 8
sleep check : id = 9
sleep check : id = 6
sleep check : id = 7
sleep check : id = 4
about to delete id = 4
sleep check : id = 5
sleep check : id = 8
sleep check : id = 9
sleep check : id = 6
sleep check : id = 7
about to delete id = 5
sleep check : id = 8
sleep check : id = 9
sleep check : id = 6
make thread :  id = 1
make thread :  id = 2
make thread :  id = 3
make thread :  id = 4
make thread :  id = 5
uthread_terminate id 0
Process finished with exit code 0
 */
void delete_and_make_check() {
    uthread_init(1);
    make_threads(MAX_THREAD_TEST - 1, thread2_sleep_check);
    for (int i = 0; i < 2; ++i) {
        check_terminate(1,MAX_THREAD_TEST/2);
        make_threads(MAX_THREAD_TEST/2, thread2_sleep_check);
    }

    printf("uthread_terminate id 0");
    uthread_terminate(uthread_get_tid());
}
/*
make thread :  id = 1
make thread :  id = 2
the current Quantum is : 1
id = 1
id = 2
the current Quantum is : 4
id = 1
id = 2
the current Quantum is : 7
id = 1
id = 2
the current Quantum is : 10
uthread_terminate id 0
Process finished with exit code 0
 */
void check_swap(){
    uthread_init(1);
    make_threads(2 , thread1_swap_check);
    quantums_to_run(10);
    printf("uthread_terminate id 0");
    uthread_terminate(uthread_get_tid());
}
/*
make thread :  id = 1
make thread :  id = 2
make thread :  id = 3
make thread :  id = 4
make thread :  id = 5
make thread :  id = 6
make thread :  id = 7
make thread :  id = 8
make thread :  id = 9
about to delete id = 1
sleep check : id = 2
sleep check : id = 3
sleep check : id = 4
sleep check : id = 5
sleep check : id = 6
sleep check : id = 7
sleep check : id = 8
sleep check : id = 9
about to delete id = 2
sleep check : id = 3
sleep check : id = 4
sleep check : id = 5
sleep check : id = 6
sleep check : id = 7
sleep check : id = 8
about to delete id = 3
sleep check : id = 9
sleep check : id = 4
sleep check : id = 5
sleep check : id = 6
sleep check : id = 7
about to delete id = 4
sleep check : id = 8
sleep check : id = 9
sleep check : id = 5
sleep check : id = 6
about to delete id = 5
sleep check : id = 7
sleep check : id = 8
sleep check : id = 9
about to delete id = 6
sleep check : id = 7
sleep check : id = 8
about to delete id = 7
sleep check : id = 9
about to delete id = 8
about to delete id = 9
uthread_terminate id 0
Process finished with exit code 0
 */
void test_deleting_threads() {
    uthread_init(1);
    make_threads(MAX_THREAD_TEST -1 , thread2_sleep_check);
    check_terminate(1,MAX_THREAD_TEST-1);
    printf("uthread_terminate id 0");
    uthread_terminate(uthread_get_tid());
}

//how to use : uncomment each one of the tests and then make sure the output
// is the same as the comment above the function
// by Mor Nahum and Ehud Bartfeld 
int main()
{
//    check_swap();
//    basic_sleep_check();
//    basic_block_check();
    get_limit_error();
//    check_uthread_get_quantums();
//    check_uthread_terminate();
//    test_deleting_threads();
//    delete_and_make_check();
    return 0;
}



