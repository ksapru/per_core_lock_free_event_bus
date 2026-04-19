# Per-Core Lock-Free Event Bus

A high-performance, low-latency event distribution system designed for financial market data and real-time packet processing. This project leverages **DPDK (Data Plane Development Kit)** for kernel-bypass networking and a **lock-free per-core architecture** to minimize contention and maximize throughput.

## 🚀 Overview

The system is designed to handle millions of events per second by assigning dedicated processing cores to specific event streams. By using lock-free ring buffers, we eliminate the overhead of mutexes and context switches in the critical path.

### Key Components

- **`generator.cpp`**: A high-speed UDP packet generator that simulates binary market data feeds (e.g., NASDAQ ITCH or binary JSON).
- **`dpdk_receiver.cpp`**: A DPDK-based ingress engine that pulls packets directly from the NIC into userspace.
- **`ring_buffer.h`**: A single-producer, single-consumer (SPSC) lock-free ring buffer implementation for inter-core communication.

---

## 🛠 Prerequisites

To build and run the receiver, you need:

- **Linux** (Recommended for DPDK support)
- **DPDK 22.11+**
- **g++ 11+** or **clang 14+**
- **Hugepages** configured on your system

---

## 🏗 Build Instructions

### Building the Generator
The generator is a standard C++ application:
```bash
g++ -O3 generator.cpp -o generator
```

### Building the DPDK Receiver
The receiver requires DPDK libraries and `pkg-config`:
```bash
g++ -O3 dpdk_receiver.cpp -o dpdk_receiver $(pkg-config --cflags --libs libdpdk)
```

---

## 🚦 Usage

1. **Start the Generator**:
   ```bash
   ./generator
   ```

2. **Start the Receiver**:
   Ensure you have bound your NIC to a DPDK-compatible driver (e.g., `uio_pci_generic` or `vfio-pci`).
   ```bash
   sudo ./dpdk_receiver -l 0-3 -n 4 -- -p 0x1
   ```

---

## 📈 Roadmap

- [ ] Implement SPSC lock-free ring buffer in `ring_buffer.h`.
- [ ] Complete DPDK EAL initialization in `dpdk_receiver.cpp`.
- [ ] Add support for multi-core distribution (RSS).
- [ ] Implement zero-copy deserialization for Market Data messages.

## ⚖ License
MIT License - see [LICENSE](LICENSE) for details.
