#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <hdr/hdr_histogram.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.hpp"

// Fast Xorshift PRNG
inline uint32_t xorshift32(uint32_t *state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}

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
  uint32_t rng_state = 123456789; // Seed for Xorshift
  char buffer[1500];

  // Initialize HdrHistogram: 1ns to 1s (1,000,000,000ns), 3 significant digits
  struct hdr_histogram *histogram;
  hdr_init(1, 1000000000, 3, &histogram);

  const uint64_t warmup_limit = 1000000;
  std::cout << "Warming up: sending " << warmup_limit << " packets..." << std::endl;

  for (uint64_t i = 0; i < warmup_limit; i++) {
    PacketHeader *header = (PacketHeader *)buffer;
    header->seq_start = global_seq;
    uint16_t msg_count = (xorshift32(&rng_state) % 10) + 1;
    header->msg_count = msg_count;

    char *ptr = buffer + sizeof(PacketHeader);
    for (int j = 0; j < msg_count; j++) {
      MarketDataMsg *msg = (MarketDataMsg *)ptr;
      msg->seq = global_seq++;
      ptr += sizeof(MarketDataMsg);
    }
    sendto(fd, buffer, ptr - buffer, 0, (sockaddr *)&addr, sizeof(addr));
  }

  uint64_t total_packets = 0;
  const uint64_t packet_limit = 10000000; // 10 million packets

  std::cout << "Starting benchmark: sending " << packet_limit << " packets..." << std::endl;

  uint64_t start_ns = now_ns();

  while (total_packets < packet_limit) {
    for (int burst = 0; burst < 10000 && total_packets < packet_limit; burst++) {
      uint64_t p_start = now_ns();

      PacketHeader *header = (PacketHeader *)buffer;
      header->seq_start = global_seq;
      uint16_t msg_count = (xorshift32(&rng_state) % 10) + 1;
      header->msg_count = msg_count;

      char *ptr = buffer + sizeof(PacketHeader);

      // Capture one timestamp to use for all messages in this packet
      uint64_t current_ts = p_start;

      for (int i = 0; i < msg_count; i++) {
        if (global_seq % 1000 == 0) {
          global_seq++;
          continue;
        }
        MarketDataMsg *msg = (MarketDataMsg *)ptr;
        msg->msg_size = sizeof(MarketDataMsg);
        msg->version = 1;
        msg->timestamp = current_ts;
        msg->seq = global_seq++;
        msg->msg_index = i;
        msg->symbol_id = xorshift32(&rng_state) % 1000;
        msg->price = xorshift32(&rng_state) % 1000;
        msg->qty = xorshift32(&rng_state) % 1000;
        msg->side = xorshift32(&rng_state) % 2;
        msg->type = xorshift32(&rng_state) % 2;
        msg->flags = xorshift32(&rng_state) % 2;
        ptr += sizeof(MarketDataMsg);
      }

      size_t packet_size = ptr - buffer;
      sendto(fd, buffer, packet_size, 0, (sockaddr *)&addr, sizeof(addr));

      uint64_t p_end = now_ns();
      hdr_record_value(histogram, p_end - p_start);

      total_packets++;
      if (total_packets % 1000000 == 0) {
        uint64_t now = now_ns();
        double elapsed = (double)(now - start_ns) / 1e9;
        uint32_t current_messages = global_seq - 1;
        std::cout << "Rate: " << (double)current_messages / elapsed << " msgs/sec (Sent " << total_packets << " packets)" << std::endl;
      }
    }
  }

  uint64_t end_ns = now_ns();
  double seconds = (double)(end_ns - start_ns) / 1e9;
  uint32_t total_messages = global_seq - 1;
  double throughput = (double)total_messages / seconds;

  std::cout << "\nFinal Results:" << std::endl;
  std::cout << "Throughput: " << throughput << " msgs/sec\n";

  std::cout << "Latency (ns):"
            << " P50: " << hdr_value_at_percentile(histogram, 50.0)
            << " P90: " << hdr_value_at_percentile(histogram, 90.0)
            << " P99: " << hdr_value_at_percentile(histogram, 99.0)
            << " P99.9: " << hdr_value_at_percentile(histogram, 99.9)
            << " P99.99: " << hdr_value_at_percentile(histogram, 99.99) << std::endl;

  hdr_close(histogram);
  close(fd);
  return 0;
}
