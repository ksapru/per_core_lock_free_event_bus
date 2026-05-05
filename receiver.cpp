#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <cstdint>
#include <thread>
#include "common.hpp"
#include "ring_buffer.h"

int main(int argc, char** argv) {
  bool quiet = false;
  if (argc > 1 && strcmp(argv[1], "--quiet") == 0) {
    quiet = true;
  }

  // 1. Setup UDP Socket
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("socket creation failed");
    return 1;
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9000);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind failed");
    return 1;
  }

  // 2. Setup the "Event Bus" (Ring Buffer)
  // We'll store up to 64k messages in the bus
  RingBuffer<MarketDataMsg> rb(65536);

  if (!quiet) {
    printf("Receiver is active!\n");
    printf("  Thread 1: Catching packets from Network\n");
    printf("  Thread 2: Extracting and processing from RingBuffer\n\n");
  }

  // 3. Start WORKER 2: The Processor Thread (Consumer)
  std::thread processor([&]() {
    MarketDataMsg msg;
    while (true) {
      if (rb.pop(msg)) {
        if (!quiet) {
          printf(" [POPPED FROM BUS] Seq: %u, Symbol: %u, Price: %u\n", 
                 msg.seq, msg.symbol_id, msg.price);
        }
      }
    }
  });

  // 4. Start WORKER 1: The Network Loop (Producer)
#ifdef __linux__
  const int VLEN = 64;
  struct mmsghdr msgvec[VLEN];
  struct iovec iov[VLEN];
  char bufs[VLEN][2048];
  
  memset(msgvec, 0, sizeof(msgvec));
  for (int i = 0; i < VLEN; i++) {
    iov[i].iov_base = bufs[i];
    iov[i].iov_len = sizeof(bufs[i]);
    msgvec[i].msg_hdr.msg_iov = &iov[i];
    msgvec[i].msg_hdr.msg_iovlen = 1;
  }

  while (true) {
    int num_msgs = recvmmsg(sock, msgvec, VLEN, 0, NULL);
    if (num_msgs < 0) {
      perror("recvmmsg failed");
      break;
    }

    for (int i = 0; i < num_msgs; i++) {
      char* packet_buf = (char*)msgvec[i].msg_hdr.msg_iov->iov_base;
      int packet_len = msgvec[i].msg_len;

      if (packet_len > 0) {
        PacketHeader* header = (PacketHeader*)packet_buf;
        MarketDataMsg* msg_ptr = (MarketDataMsg*)(packet_buf + sizeof(PacketHeader));
        
        for (int j = 0; j < header->msg_count; j++) {
          while (!rb.push(msg_ptr[j])) { 
            std::this_thread::yield(); 
          }
        }
      }
    }
  }
#else
  // Fallback for non-Linux platforms (e.g. macOS)
  printf("Note: recvmmsg not supported on this platform. Falling back to recvfrom.\n");
  char buffer[2048];
  while (true) {
    ssize_t packet_len = recv(sock, buffer, sizeof(buffer), 0);
    if (packet_len > 0) {
      PacketHeader* header = (PacketHeader*)buffer;
      MarketDataMsg* msg_ptr = (MarketDataMsg*)(buffer + sizeof(PacketHeader));
      for (int j = 0; j < header->msg_count; j++) {
        while (!rb.push(msg_ptr[j])) { 
          std::this_thread::yield(); 
        }
      }
    }
  }
#endif

  processor.join();
  return 0;
}