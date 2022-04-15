//
// Created by eliasital
//

/**
 * boolean print (global var) :
 *     prints to screen if true, otherwise runs, but without any prints.
 *
 * printCount (function local variable) :
 *     Functions runs and writes 'in f(0)', 'in f(1)' and so on.
 *     The number in the brackets is the printCount var.
 *     A condition of 'if (printCount == x) then ...'
 *     states what will the function do when it will reach its 'x' cycle.
 *
 * Tests :
 *     1    Spawn functions
 *     2    Non-Main function terminates it self
 *     3    Non-Main function blocks Non-Main function
 *     4    Non-Main function goes to sleep
 *     5    Main terminates functions
 *     6    Main blocks function
 *     7    Main unblocks function
 *     8    Main removes blocked function
 *     9    Main terminates with functions that are alive
 *     10   Sanity check - Main blocks sleeping function (OK!)
 *          Sanity check - Main unblocks sleeping function (ERR!)
 *          Sanity check - Main blocks non-existent function (ERR!)
 *          Sanity check - Main asks to sleep (ERR!)
 *          Sanity check - Main asks to block himself (ERR!)
 *          Sanity check - Main asks to unblock sleeping function (ERR!)
 */

#include <array>
#include <iostream>
#include "../uthreads.h"

#define QUANT 1000000
#define PRINTER 100000000

bool print = true;

using namespace std;

/**
 * A Simple function - runs and terminates itself in the end.
 */
void f()
{
    int printCount = 0;
    for (int i = 0; ; i++)
    {
        if (i%PRINTER == 0)
        {
            if (print) {cout << "in f(" << printCount << ")" << endl;}
            if (printCount == 7)
            {
                break;
            }
            printCount++;
        }
    }
    if (print) {cout << "f terminates" << endl;}
    int me = uthread_get_tid();
    uthread_terminate(me);
}

/**
 * A Simple function - runs and terminates itself in the end.
 */
void g()
{
    int printCount = 0;
    for (int i = 0; ; i++)
    {
        if (i%PRINTER == 0)
        {
            if (print) {cout << "in g(" << printCount << ")" << endl;}
            if (printCount == 13)
            {
                break;
            }
            printCount++;
        }
    }
    if (print) {cout << "g terminates" << endl;}
    int me = uthread_get_tid();
    uthread_terminate(me);
}

/**
 * A Simple function - runs forever, until terminated.
 */
void neverEnding_h()
{
    int printCount = 0;
    for (int i = 0; ; i++)
    {
        if (i%PRINTER == 0)
        {
            if (print) {cout << "in h(" << printCount << ")" << endl;}
            printCount++;
        }
    }
}

/**
 * A Simple function - runs forever, until terminated.
 */
void neverEnding_i()
{
    int printCount = 0;
    for (int i = 0; ; i++)
    {
        if (i%PRINTER == 0)
        {
            if (print) {cout << "in i(" << printCount << ")" << endl;}
            printCount++;
        }
    }
}

/**
 * A Simple function - runs forever, until terminated.
 */
void neverEnding_j()
{
    int printCount = 0;
    for (int i = 0; ; i++)
    {
        if (i%PRINTER == 0)
        {
            if (print) {cout << "in j(" << printCount << ")" << endl;}
            printCount++;
        }
    }
}

///**
// * A simple function that sends it self to sleep.
// */
//void sleeper()
//{
//    int printCount = 0;
//    for (int i = 0; ; i++)
//    {
//        if (i%PRINTER == 0)
//        {
//            if (print) {cout << "in sleeper(" << printCount << ")" << endl;}
//
//            if (printCount == 15)
//            {
//                if (print) {cout << "sleeper went to sleep for 50 quants!" << endl;}
//                uthread_sleep(50);
//                if (print) {cout << "sleeper is awake again!" << endl;}
//            }
//
//            if (printCount == 57)
//            {
//                break;
//            }
//            printCount++;
//        }
//    }
//
//    if (print) {cout << "sleeper terminates" << endl;}
//    int me = uthread_get_tid();
//    uthread_terminate(me);
//}

/**
 * Blocks some thread and then unblocks its.
 * Blocks it self.
 */
void blocker()
{
    int printCount = 0;
    for (int i = 0; ; i++)
    {
        if (i%PRINTER == 0)
        {
            if (print) {cout << "in blocker(" << printCount << ")" << endl;}

            if (printCount == 3)
            {
                if (print) {cout << "blocker blocks f (tid = 1)!" << endl;}
                uthread_block(1);
            }

            if (printCount == 11)
            {
                if (print) {cout << "blocker unblocks f (tid = 1)!" << endl;}
                uthread_resume(1);
            }

            if (printCount == 40)
            {
                if (print) {cout << "blocker blocks himself!" << endl;}
                uthread_block(uthread_get_tid());
                if (print) {cout << "blocker was resumed!" << endl;}
                break;
            }
            printCount++;
        }
    }
    if (print) {cout << "blocker terminates" << endl;}
    int me = uthread_get_tid();
    uthread_terminate(me);
}


int main()
{

	int q[2] = {10, 20};
	int init = uthread_init(2);

    if (print) {cout << "MAIN INIT : " << init << endl << endl;}

    int fid = uthread_spawn(&f);
    if (print) {cout << "MAIN SPAWNS f : " << fid << endl;}

    int gid = uthread_spawn(&g);
    if (print) {cout << "MAIN SPAWNS g : " << gid << endl;}

    int hid = uthread_spawn(&neverEnding_h);
    if (print) {cout << "MAIN SPAWNS h : " << hid << endl;}

    int iid = uthread_spawn(&neverEnding_i);
    if (print) {cout << "MAIN SPAWNS i : " << iid << endl;}

    int jid = uthread_spawn(&neverEnding_j);
    if (print) {cout << "MAIN SPAWNS j : " << jid << endl;}

//    int sleeperid = uthread_spawn(&sleeper, 1);
//    if (print) {cout << "MAIN SPAWNS sleeper : " << sleeperid << endl;}

    int blockerid = uthread_spawn(&blocker);
    if (print) {cout << "MAIN SPAWNS blockerid : " << blockerid << endl;}

    if (print) {cout << endl << "MAIN RUNS!" << endl;}

    int printCount = 0;
    for (int i = 0; ; i++)
    {
        if (i%PRINTER == 0)
        {
            if (print) {cout << "IN MAIN(" << printCount << ")" << endl;}

            if (printCount == 20)
            {
//                if (print) {cout << "GOOD-ERR Main blocks sleeping sleeper!" << endl;}
//                uthread_block(sleeperid);
//                if (print) {cout << "ERR Main unblocks sleeping sleeper!" << endl;}
//                uthread_resume(sleeperid);
                if (print) {cout << "ERR Main blocks non-existent 99!" << endl;}
                uthread_block(99);
                if (print) {cout << "ERR Main unblocks non-existent 101!" << endl;}
                uthread_block(101);
                if (print) {cout << "ERR Main asks to block himself!" << endl;}
                uthread_block(uthread_get_tid());
            }

            if (printCount == 30)
            {
                if (print) {cout << "Main removes h,i!" << endl;}
                uthread_terminate(hid);
                uthread_terminate(iid);

//                if (print) {cout << "ERR Main unblocks sleeping sleeper!" << endl;}
//                uthread_resume(sleeperid);
            }


            if (printCount == 45)
            {
                if (print) {cout << "Main unblocks blocker (which blocked himself)!" << endl;}
                uthread_resume(blockerid);
            }

            if (printCount == 52)
            {
                if (print) {cout << "Main blocks blocker!" << endl;}
                uthread_block(blockerid);
            }

            if (printCount == 58)
            {
                if (print) {cout << "Main removes blocker!" << endl;}
                uthread_terminate(blockerid);
            }

            if (printCount == 65)
            {
                break;
            }
            printCount++;
        }
    }

    if (print) {cout << "MAIN TERMINATES WILE j IS ALIVE" << endl;}
    int me = uthread_get_tid();
    uthread_terminate(me);
}