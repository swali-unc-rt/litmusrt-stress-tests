/* smlpstresstest.cpp - Stress test for SMLP implementation in Litmus-RT
 *
 * All this test does is spam lock and unlock requests to the SMLP. If
 * everything goes well, there shouldn't be errors and all threads will be cleaned up.
 * There should not be any deadlocks/livelocks.
 */

#include <stdio.h>
#include <stdlib.h>

#include <litmus.h>
#include <unistd.h>

#include "litmushelper.hpp"

#include <thread>
#include <vector>
#include <barrier>

#define NUMTHREADS 100
#define SMLP_OD_NAME "./smlp_lock_od"

#define WORKERPERIOD_MS 15
#define WORKERCOST_MS 3
#define RELEASECOUNT 100
#define CSLEN_US 1000

using std::thread;
using std::vector;
using std::barrier;

void smlpworker();

barrier sync_point(NUMTHREADS + 1);

int main(int argc, char* argv[]) {
    system(LIBLITMUS_LIB_DIR "/setsched GSN-EDF");
    sleep(3);

    auto _tid = litmus_gettid();
    LITMUS_CALL_TID( init_litmus() );

    int smlpod = open_smlp_sem( 1, SMLP_OD_NAME, 0xFFF, 1 );
    printf("SMLP OD is %d\n", smlpod);

    vector<thread> threads;
    // Create our threads
    for( int i = 0; i < NUMTHREADS; ++i )
        threads.emplace_back(smlpworker);

    sync_point.arrive_and_wait();
    sleep(1);

    // Become real-time
    become_periodic(ms2ns(WORKERCOST_MS), ms2ns(WORKERPERIOD_MS));

    // release the task set
    if( release_taskset( ms2ns(100), ms2ns(100) ) ) {
        fprintf(stderr, "%d: Failed to release taskset\n", _tid);
        system(LIBLITMUS_LIB_DIR "/setsched Linux");
        return -1;
    }

    for( int i = 0; i < RELEASECOUNT; ++i ) {
        //if( i > 0 && ( i % 10 ) == 0 ) {
            uint64_t assigned_mask;
            litmus_smlp_lock( smlpod, (1<<0) | (1<<1) | (1<<3), &assigned_mask );
            //printf("%d: Acquired lock %d, assigned mask: %lx\n", _tid, smlpod, assigned_mask);
            usleep(CSLEN_US); // hold the lock for 1ms
            litmus_smlp_gpu_done( smlpod ); // This is a hack to force-move to the PIQ
            //printf("%d: Releasing lock %d\n", _tid, smlpod);
            litmus_smlp_unlock( smlpod );
        //} else usleep(1000);
        sleep_next_period();
    }

    LITMUS_CALL_TID( task_mode(BACKGROUND_TASK) );

    for( int i = 0; i < NUMTHREADS; ++i )
        threads[i].join();
    
    system(LIBLITMUS_LIB_DIR "/setsched Linux");
    unlink(SMLP_OD_NAME);
    return 0;
}

void smlpworker() {
    auto _tid = litmus_gettid();
    LITMUS_CALL_TID( init_rt_thread() );

    int smlpod = open_smlp_sem( 1, SMLP_OD_NAME, 0xFFF, 1 );

    become_periodic(ms2ns(WORKERCOST_MS), ms2ns(WORKERPERIOD_MS));
    sync_point.arrive_and_drop();
    wait_for_ts_release();

    for( int i = 0; i < RELEASECOUNT; ++i ) {
        //if( i > 0 && ( i % 10 ) == 0 ) {
            uint64_t assigned_mask;
            LITMUS_CALL_TID( litmus_smlp_lock( smlpod, (1<<0) | (1<<1) | (1<<3), &assigned_mask ) );
            //printf("%d: Acquired lock %d, assigned mask: %lx\n", _tid, smlpod, assigned_mask);
            usleep(CSLEN_US); // hold the lock for 1ms
            LITMUS_CALL_TID( litmus_smlp_gpu_done( smlpod ) );
            //printf("%d: Releasing lock %d\n", _tid, smlpod);
            LITMUS_CALL_TID( litmus_smlp_unlock( smlpod ) );
        //} else usleep(1000);
        sleep_next_period();
    }

    LITMUS_CALL_TID( task_mode(BACKGROUND_TASK) );
}
