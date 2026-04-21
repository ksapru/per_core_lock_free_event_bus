# Per-Core Lock-Free Event Bus

A high-performance, low-latency event distribution system designed for financial market data and real-time packet processing. This project leverages a **lock-free SPSC (Single Producer Single Consumer) architecture** and Linux-native optimizations to minimize contention and maximize throughput.

## Overview

The system handles millions of events per second by decoupling network I/O from data processing using a lock-free ring buffer. This architecture eliminates mutex contention and reduces context switches in the critical path.

### Key Components

- **`receiver.cpp`**: A multi-threaded Linux receiver utilizing `recvmmsg` for vectorized packet capture, significantly reducing system call overhead.
- **`generator.cpp`**: A UDP burst generator that simulates high-frequency market data feeds with sequence tracking and micro-benchmarking.
- **`ring_buffer.h`**: A thread-safe, lock-free SPSC ring buffer using `std::atomic` with `memory_order_acquire/release` and `alignas(64)` to prevent false sharing.
- **`common.hpp`**: Shared binary message definitions (`MarketDataMsg`) with `#pragma pack(1)` for zero-copy compatibility.

---

## Deployment: Google Compute Engine (GCE)

This system is optimized for high-performance Linux environments. We verified the implementation on a GCE instance with the following specs:
- **Instance Type**: `e2-standard-4` (4 vCPUs, 16GB RAM)
- **OS**: Debian 12 (Linux 6.1+)
- **Network**: Standard tiered networking with UDP optimization.

### Performance Milestone
Successfully deployed and ran the consumer-producer loop on GCE, achieving high-throughput event distribution between dedicated network capture and processing threads.

---

## Build Instructions

This project uses **CMake**. To build the components, run:

```bash
mkdir build && cd build
cmake ..
make
```

These commands will generate the `generator` and `receiver` binaries in the `build` directory.

---

## Usage

1. **Start the Receiver**:
   ```bash
   ./receiver
   ```

2. **Start the Generator**:
   ```bash
   ./generator
   ```

The receiver will bind to port `9000` and start processing packets pushed through the event bus.

---

## Roadmap

- [x] Implement SPSC lock-free ring buffer with cache-line alignment.
- [x] Transition to `recvmmsg` for batch packet reception.
- [x] Verify deployment on Linux GCE VM.
- [ ] Implement NUMA-aware core pinning (CPU affinity).
- [ ] Add DPDK kernel-bypass engine for <1μs latency.
- [ ] Implement zero-copy partial deserialization.

## ⚖ License
MIT License - see [LICENSE](LICENSE) for details.
