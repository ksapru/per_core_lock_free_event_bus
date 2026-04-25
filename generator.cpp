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

#include "common.hpp"

uint64_t now_ns() {
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint64_t)ts.tv_sec * 1e9 + ts.tv_nsec;
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

  std::cout << "Starting generator, sending " << packet_limit << " packets..." << std::endl;

  auto start = std::chrono::high_resolution_clock::now();

  while (total_packets < packet_limit) {
    for (int burst = 0; burst < 10000 && total_packets < packet_limit; burst++) {
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

      total_packets++;
      if (total_packets % 1000000 == 0) {
        std::cout << "Sent " << total_packets << " packets. Last seq: " << global_seq - 1 << std::endl;
      }
    }
  }

  auto end = std::chrono::high_resolution_clock::now();

  double seconds = std::chrono::duration<double>(end - start).count();
  uint32_t total_messages = global_seq - 1;
  double throughput = total_messages / seconds;

  std::cout << "Throughput: " << throughput << " msgs/sec\n";

  close(fd);
  return 0;
}
