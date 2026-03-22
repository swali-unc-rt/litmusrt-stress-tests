/* rgtest.cpp - test for release groups
 *
 * The key thing to remember about release groups is that
 * the id cannot be zero. This is because zero is used to indicate that a task is not in a release group.
 */

#include <stdio.h>
#include <stdlib.h>

#include <litmus.h>
#include <unistd.h>

#include "litmushelper.hpp"

#include <thread>
#include <barrier>
#include <vector>

using std::thread;
using std::barrier;
using std::stop_token;
using std::stop_source;
using std::vector;

// Parameters of a periodic task that releases the group
#define RGPERIOD ms2ns(100)
#define RGCOST ms2ns(1)

// Parameters for a task that is released as a part of the group
#define WORKERPERIOD ms2ns(10)
#define WORKERCOST ms2ns(1)

// The number if times a group is released
#define MAXITER 40

// Number of tasks in a group
#define RGTHREADS 100

// ID if the release group. Cannot be zero.
#define RGID 1U

// A releasegroup task that increments an integer pointed to by counter on every release. It stops when the stop_token is requested to stop.
void incrementer(unsigned int rgid, int* counter, stop_token stopper);

// A periodic task that releases the release group on every period. It stops when the stop_token is requested to stop.
void groupreleaser(stop_token stopper);

int main(int argc, char* argv[]) {
    // Set schedule to GSN-EDF
    system(LIBLITMUS_LIB_DIR "/setsched GSN-EDF");
    sleep(3);
    // Initialize the release group environment
    litmus_releasegroup_envinit();

    // Initialize litmus
    auto _tid = litmus_gettid();
    LITMUS_CALL_TID( init_litmus() );

    // Create our release group
    litmus_releasegroup_create(RGID);

    // Create our threads
    vector<thread> threads;
    stop_source stopper;
    int counters[RGTHREADS];
    for( unsigned int i = 0; i < RGTHREADS; ++i ) {
        counters[i] = 0;
        threads.emplace_back(incrementer, RGID, &counters[i], stopper.get_token());
    }

    // Create the periodic release group releaser thread.
    thread groupreleaser_thread(groupreleaser, stopper.get_token());

    printf("Waiting for threads to be ready...\n");
    while( get_nr_ts_release_waiters() < RGTHREADS + 1) {
        // Wait for threads
    }
    printf("Threads ready, releasing task set.\n");

    // release the task set
    if( release_taskset( ms2ns(100), ms2ns(100) ) ) {
        fprintf(stderr, "%d: Failed to release taskset\n", _tid);
        system(LIBLITMUS_LIB_DIR "/setsched Linux");
        return -1;
    }

    // Sleep for a while
    sleep( (RGPERIOD * MAXITER / 1000000000ULL)  + 5 );

    printf("Winding down..\n");

    // Stop the threads
    stopper.request_stop();
    sleep(1);

    // Do one final release to wake up the threads so they can cleanly exit.
    // If this is not done, the system will have zombie threads, so make sure to do this.
    litmus_releasegroup_release(RGID);
    for( auto& t : threads ) {
        t.join();
    }
    groupreleaser_thread.join();
    sleep(1);

    // Clean up the releasegroup environment
    litmus_releasegroup_envdestroy();
    // Switch back to Linux scheduler (will clean up GSN-EDF as well)
    system(LIBLITMUS_LIB_DIR "/setsched Linux");

    // Output the counters, they should be all equal to MAXITER
    printf("Counters:\n");
    for( int i = 0 ; i < RGTHREADS; ++i ) {
        printf("%s Thread %d: %d\n", counters[i] == MAXITER ? "[OK]" : "[FAIL]", i, counters[i]);
    }

    return 0;
}

void incrementer(unsigned int rgid, int* counter, stop_token stopper) {
    auto _tid = litmus_gettid();
    LITMUS_CALL_TID( init_rt_thread() );

    become_rgtask(WORKERCOST, WORKERPERIOD, rgid);
    wait_for_ts_release();

    while( !stopper.stop_requested()) {
        ++(*counter);
        sleep_next_period();
    }

    LITMUS_CALL_TID( litmus_releasegroup_remove() );
    LITMUS_CALL_TID( task_mode(BACKGROUND_TASK) );
}

void groupreleaser(stop_token stopper) {
    auto _tid = litmus_gettid();
    LITMUS_CALL_TID( init_rt_thread() );

    become_periodic(RGCOST, RGPERIOD);
    litmus_releasegroup_cache(RGID);
    wait_for_ts_release();

    for( int i = 0; i < MAXITER && !stopper.stop_requested(); ++i ) {
        //litmus_releasegroup_release(RGID);
        litmus_releasegroup_release_cached();
        sleep_next_period();
    }

    LITMUS_CALL_TID( task_mode(BACKGROUND_TASK) );
}