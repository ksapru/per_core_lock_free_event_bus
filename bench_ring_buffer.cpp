#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <iomanip>
#include "ring_buffer.h"

#ifdef __linux__
#include <pthread.h>
#endif

// Portable CPU pause for busy-waiting
#if defined(__x86_64__) || defined(__i386__)
    #include <immintrin.h>
    #define CPU_PAUSE() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
    #define CPU_PAUSE() __asm__ volatile("yield")
#else
    #define CPU_PAUSE() std::this_thread::yield()
#endif

struct BenchMsg {
    uint64_t seq;
    uint64_t timestamp;
    uint64_t data[6]; // Total 64 bytes to fill a cache line
};

void set_affinity(int core_id) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "[WARNING] Failed to set CPU affinity for core " << core_id << ": " << rc << std::endl;
    }
#endif
}


int main() {
    const size_t num_msgs = 100'000'000;
    const size_t capacity = 65536;
    RingBuffer<BenchMsg> rb(capacity);

   
    std::cout << " Messages:      " << num_msgs << std::endl;
    std::cout << " Capacity:      " << capacity << std::endl;
    std::cout << " Message Size:  " << sizeof(BenchMsg) << " bytes" << std::endl;
    std::cout << " Architecture:  " << (sizeof(void*) == 8 ? "64-bit" : "32-bit") << std::endl;

    std::atomic<bool> start_flag{false};
    std::atomic<size_t> latencies_sum{0};
    const size_t sample_interval = 1000; // Measure latency every 1000 messages

    std::thread producer([&]() {
        set_affinity(1); // Pin to core 1
        while (!start_flag.load()) CPU_PAUSE();
        
        BenchMsg msg;
        for (size_t i = 0; i < num_msgs; ++i) {
            msg.seq = i;
            if (i % sample_interval == 0) {
                msg.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
            }
            while (!rb.push(msg)) {
                CPU_PAUSE(); 
            }
        }
    });

    std::thread consumer([&]() {
        set_affinity(2); // Pin to core 2
        while (!start_flag.load()) CPU_PAUSE();
        
        BenchMsg msg;
        size_t sampled_count = 0;
        for (size_t i = 0; i < num_msgs; ++i) {
            while (!rb.pop(msg)) {
                CPU_PAUSE();
            }
            if (msg.seq != i) {
                std::cerr << "\n[ERROR] Sequence mismatch at " << i << "! Expected " << i << " got " << msg.seq << std::endl;
                exit(1);
            }
            if (i % sample_interval == 0) {
                uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                latencies_sum += (now - msg.timestamp);
                sampled_count++;
            }
        }
    });

    auto start_time = std::chrono::steady_clock::now();
    start_flag.store(true);

    producer.join();
    consumer.join();

    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;

    double throughput = (num_msgs / diff.count()) / 1'000'000.0;
    double avg_sampled_latency = (double)latencies_sum / (num_msgs / sample_interval);

    std::cout << " RESULTS" << std::endl;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  Total Time:     " << diff.count() << " s" << std::endl;
    std::cout << "  Throughput:     " << throughput << " M msgs/sec" << std::endl;
    std::cout << "  Mean Latency:   " << avg_sampled_latency << " ns (sampled)" << std::endl;

    return 0;
}
