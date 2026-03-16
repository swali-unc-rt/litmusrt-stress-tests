/* smlpcorrectnesstest.cpp
 *
 * Test the correctness of the SMLP implementation by having multiple threads
 * lock and unlock the same SMLP semaphore in a specific order, and checking
 * that the assigned masks are correct.
 * 
 * In particular, this test does the following:
 * Initialize: 0xfff available
 * Worker 1 locks (TPCs allowed 1,2,4), expects to get 4 bits, 0x00f
 * Worker 2 locks (TPCs allowed 1,2,4), expects to get 4 bits, 0x0f0
 * Worker 3 locks (TPCs allowed 1,2,4), expects to get 4 bits, 0xf00
 * Worker 1 & 3 unlock, go from 0x000 -> 0xf0f available
 * Worker 4 locks (TPCs allowed 1,8), expects to get 8 bits, 0xf0f
 * Cleanup
 */

#include <stdio.h>
#include <stdlib.h>

#include <litmus.h>
#include <unistd.h>

#include "litmushelper.hpp"

#include <thread>
#include <barrier>

using std::thread;
using std::barrier;
using std::ref;

#define SMLP_OD_NAME "./smlp_lock_od"
#define SMLP_INITMASK 0xfff
#define SMLP_OD_ID 1

#define WORKERPERIOD_MS 15
#define WORKERCOST_MS 3

void smlpworker(barrier<>& job_sync_point, barrier<>& unlock_point, unsigned long requestMask, unsigned long expectedMask);

int main(int argc, char* argv[]) {
    auto _tid = litmus_gettid();
    system(LIBLITMUS_LIB_DIR "/setsched GSN-EDF");
    sleep(3);
    LITMUS_CALL_TID( init_litmus() );
    int smlpod = open_smlp_sem( SMLP_OD_ID, SMLP_OD_NAME, SMLP_INITMASK, 1 );

    // Lock once, Go from 0xfff -> 0xff0 available, obtain 0x00f
    barrier sync_point_1(2); barrier unlock_point_1(2);
    thread worker1(smlpworker, ref(sync_point_1), ref(unlock_point_1), (1<<0) | (1<<1) | (1<<3), 0xf);

    // Lock twice, Go from 0xff0 -> 0xf00 available, obtain 0x0f0
    barrier sync_point_2(2); barrier unlock_point_2(2);
    thread worker2(smlpworker, ref(sync_point_2), ref(unlock_point_2), (1<<0) | (1<<2) | (1<<3), 0xf0);

    // Lock again, Go from 0xf00 -> 0x000 available, obtain 0xf00
    barrier sync_point_3(2); barrier unlock_point_3(2);
    thread worker3(smlpworker, ref(sync_point_3), ref(unlock_point_3), (1<<0) | (1<<2) | (1<<3), 0xf00);

    // Unlock 1 & 3, go from 0x000 -> 0xf0f available
    // Lock again, go from 0xf0f -> 0x000 available, obtain 0xf0f
    barrier sync_point_4(2); barrier unlock_point_4(2);
    thread worker4(smlpworker, ref(sync_point_4), ref(unlock_point_4), (1<<0) | (1<<7), 0xf0f);

    // Begin!
    sync_point_1.arrive_and_wait();
    usleep(1000);
    sync_point_2.arrive_and_wait();
    usleep(1000);
    sync_point_3.arrive_and_wait();
    usleep(1000);

    // Now begin unlocks
    unlock_point_1.arrive_and_wait();
    unlock_point_3.arrive_and_wait();
    sleep(1);

    // Do the last lock
    sync_point_4.arrive_and_wait();

    // Wait
    sleep(1);

    // Now cleanup
    unlock_point_2.arrive_and_wait();
    unlock_point_4.arrive_and_wait();

    sleep(1);

    worker1.join();
    worker2.join();
    worker3.join();
    worker4.join();

    unlink(SMLP_OD_NAME);

    system(LIBLITMUS_LIB_DIR "/setsched Linux");
    return 0;
}

void smlpworker(barrier<>& job_sync_point, barrier<>& unlock_point, unsigned long requestMask, unsigned long expectedMask) {
    auto _tid = litmus_gettid();
    LITMUS_CALL_TID( init_rt_thread() );

    become_periodic(ms2ns(WORKERCOST_MS), ms2ns(WORKERPERIOD_MS));
    int smlpod = open_smlp_sem( SMLP_OD_ID, SMLP_OD_NAME, SMLP_INITMASK, 1 );

    job_sync_point.arrive_and_wait();

    uint64_t assigned_mask;
    LITMUS_CALL_TID( litmus_smlp_lock( smlpod, requestMask, &assigned_mask ) );
    if( assigned_mask != expectedMask ) {
        fprintf(stderr, "[ERROR] SMLP worker %lu: expected mask 0x%lx, got 0x%lx\n", std::this_thread::get_id(), expectedMask, assigned_mask);
    } else {
        printf("[SUCCESS] expected mask 0x%lx\n", assigned_mask);
    }
    unlock_point.arrive_and_wait();
    LITMUS_CALL_TID( litmus_smlp_gpu_done(smlpod) );
    LITMUS_CALL_TID( litmus_smlp_unlock(smlpod) );

    LITMUS_CALL_TID( task_mode(BACKGROUND_TASK) );
}