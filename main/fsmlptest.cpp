#include <stdio.h>
#include <stdlib.h>

#include "litmushelper.hpp"

#include "fsmlp.hpp"

#include <vector>
#include <thread>
#include <barrier>
#include <mutex>

using std::vector;
using std::thread;
using std::barrier;
using std::stop_token;
using std::stop_source;
using std::mutex;

#define WORKERPERIOD_MS 60
#define WORKERCOST_MS 3
#define DURATION_S 15
#define PROCESSORPERIOD_MS 3
#define PROCESSORCOST_MS 2
#define WORKERTHREADS 8

FSMLP fsmlp((uint64_t)-1, 10000);

void worker(stop_token stopper);
void queueprocessor(stop_token stopper);
vector<lt_t> lock_costs(10000); unsigned int lock_i = 0; mutex lock_m;
vector<lt_t> process_costs(10000); unsigned int process_i = 0; mutex process_m;

void record_lock_cost(lt_t cost);
void record_process_cost(lt_t cost);

int main(int argc, char* argv[]) {
    auto _tid = litmus_gettid();
    //system(LIBLITMUS_LIB_DIR "/setsched GSN-EDF");
    //sleep(3);
    LITMUS_CALL_TID( init_litmus() );
    
    vector<thread> threads;
    stop_source stopper;
    for( unsigned int i = 0; i < WORKERTHREADS; ++i ) {
        threads.emplace_back(worker, stopper.get_token());
    }
    threads.emplace_back(queueprocessor, stopper.get_token());

    printf("Waiting for threads to be ready...\n");
    while( get_nr_ts_release_waiters() < WORKERTHREADS + 1) {
        // Wait for threads
    }

    // release the task set
    if( release_taskset( ms2ns(100), ms2ns(100) ) ) {
        fprintf(stderr, "%d: Failed to release taskset\n", _tid);
        system(LIBLITMUS_LIB_DIR "/setsched Linux");
        return -1;
    }

    sleep(DURATION_S);

    printf("Winding down..\n");
    stopper.request_stop();
    for( auto& t : threads ) {
        t.join();
    }
    //system(LIBLITMUS_LIB_DIR "/setsched Linux");

    // printf("lock costs:\n");
    // for(auto& cost : lock_costs) {
    //     if( !cost ) continue;
    //     printf("%lu,", cost);
    // }
    // printf("process costs:\n");
    // for(auto& cost : process_costs) {
    //     if( !cost ) continue;
    //     printf("%lu,", cost);
    // }
    // printf("\n");
    return 0;
}

void queueprocessor(stop_token stopper) {
    auto _tid = litmus_gettid();
    LITMUS_CALL_TID( init_rt_thread() );

    printf("Queue processor tid: %d\n", _tid);

    become_periodic(ms2ns(PROCESSORCOST_MS), ms2ns(PROCESSORPERIOD_MS));
    wait_for_ts_release();

    while( !stopper.stop_requested() ) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto startclock = litmus_clock();
        fsmlp.processQueues();
        record_process_cost(litmus_clock() - startclock);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        sleep_next_period();
    }

    LITMUS_CALL_TID( task_mode(BACKGROUND_TASK) );
}

void worker(stop_token stopper) {
    auto _tid = litmus_gettid();
    LITMUS_CALL_TID( init_rt_thread() );

    printf("Worker tid: %d\n", _tid);

    GPURequest req {
        .abs_deadline = 0,
        .smctrl_tpcs_allowed = (1<<0)|(1<<1),
        .smctrl_mask_assigned = 0,
        //.stream = nullptr,
        .gpu_launch_fn = [=](){},
        .job_barrier = std::barrier(2),
        .lock_node = MCSNode{}
    };

    become_periodic(ms2ns(WORKERCOST_MS), ms2ns(WORKERPERIOD_MS));
    wait_for_ts_release();

    struct control_page* cpage = get_ctrl_page();

    while( !stopper.stop_requested() ) {
        req.abs_deadline = cpage->deadline;
        //std::atomic_thread_fence(std::memory_order_seq_cst);
        //auto startclock = litmus_clock();
        auto cost = fsmlp.submitRequest(
            &req
            //(1<<0)|(1<<1), [](){
            //printf("Hello from GPU on thread %d!\n", _tid);
            // Simulate some GPU work with a busy loop
            // volatile int x = 0;
            // for( int i = 0; i < 10000; ++i )
            //     x += i;
        );
        record_lock_cost(cost);
        //std::atomic_thread_fence(std::memory_order_seq_cst);
        sleep_next_period();
    }

     LITMUS_CALL_TID( task_mode(BACKGROUND_TASK) );
}

void record_lock_cost(lt_t cost) {
    //std::lock_guard<mutex> lock(lock_m);
    //lock_costs[lock_i++] = cost;
}

void record_process_cost(lt_t cost) {
    //std::lock_guard<mutex> lock(process_m);
    //process_costs[process_i++] = cost;
}