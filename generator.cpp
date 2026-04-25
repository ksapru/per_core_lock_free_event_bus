#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "common.hpp"

uint64_t now_ns() {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main() {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9000);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  uint32_t global_seq = 1;

  char buffer[1500];
  uint64_t total_packets = 0;
  const uint64_t packet_limit = 10000000; // 10 million packets

  std::vector<uint64_t> latencies;
  latencies.reserve(packet_limit);

  std::cout << "Starting generator, sending " << packet_limit << " packets..." << std::endl;

  uint64_t start_ns = now_ns();

  while (total_packets < packet_limit) {
    for (int burst = 0; burst < 10000 && total_packets < packet_limit; burst++) {
      uint64_t p_start = now_ns();

      PacketHeader *header = (PacketHeader *)buffer;
      header->seq_start = global_seq;
      uint16_t msg_count = rand() % 10 + 1;
      header->msg_count = msg_count;

      char *ptr = buffer + sizeof(PacketHeader);

      for (int i = 0; i < msg_count; i++) {
        if (global_seq % 1000 == 0) {
          global_seq++;
          continue;
        }
        MarketDataMsg *msg = (MarketDataMsg *)ptr;
        msg->msg_size = sizeof(MarketDataMsg);
        msg->version = 1;
        msg->timestamp = now_ns();
        msg->seq = global_seq++;
        msg->msg_index = i;
        msg->symbol_id = rand() % 1000;
        msg->price = rand() % 1000;
        msg->qty = rand() % 1000;
        msg->side = rand() % 2;
        msg->type = rand() % 2;
        msg->flags = rand() % 2;
        ptr += sizeof(MarketDataMsg);
      }

      size_t packet_size = ptr - buffer;

      sendto(fd, buffer, packet_size, 0, (sockaddr *)&addr, sizeof(addr));

      uint64_t p_end = now_ns();
      latencies.push_back(p_end - p_start);

      total_packets++;
      if (total_packets % 1000000 == 0) {
        uint64_t now = now_ns();
        double elapsed = (now - start_ns) / 1e9;
        uint32_t current_messages = global_seq - 1;
        std::cout << "Rate: " << current_messages / elapsed << " msgs/sec (Sent " << total_packets << " packets)" << std::endl;
      }
    }
  }

  uint64_t end_ns = now_ns();
  double seconds = (end_ns - start_ns) / 1e9;
  uint32_t total_messages = global_seq - 1;
  double throughput = total_messages / seconds;

  std::cout << "\nFinal Results:" << std::endl;
  std::cout << "Throughput: " << throughput << " msgs/sec\n";

  if (!latencies.empty()) {
    std::sort(latencies.begin(), latencies.end());
    std::cout << "Latency (ns):"
              << " P50: " << latencies[latencies.size() * 0.50]
              << " P90: " << latencies[latencies.size() * 0.90]
              << " P99: " << latencies[latencies.size() * 0.99]
              << " P99.9: " << latencies[latencies.size() * 0.999]
              << " P99.99: " << latencies[latencies.size() * 0.9999] << std::endl;
  }

  close(fd);
  return 0;
}
