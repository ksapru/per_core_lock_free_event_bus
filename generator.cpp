#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

#pragma pack(push, 1)

struct PacketHeader {
  uint32_t seq_start;
  uint16_t msg_count;
};

struct MarketDataMsg {
  uint16_t msg_size;
  uint8_t version;

  uint64_t timestamp;
  uint32_t seq;
  uint16_t msg_index;

  uint32_t symbol_id;

  uint32_t price;
  uint32_t qty;

  uint8_t side;
  uint8_t type;
  uint8_t flags;
};

#pragma pack(pop)

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

  while (true) {
    for (int burst = 0; burst < 10000; burst++) {
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
  close(fd);
  return 0;
}
