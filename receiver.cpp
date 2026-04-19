#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <cstdint>
#include <thread>
#include "common.hpp"
#include "ring_buffer.h"

int main() {
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

  printf("Receiver is active!\n");
  printf("  Thread 1: Catching packets from Network\n");
  printf("  Thread 2: Extracting and printing from RingBuffer\n\n");

  // 3. Start WORKER 2: The Processor Thread (Consumer)
  std::thread processor([&]() {
    MarketDataMsg msg;
    while (true) {
      if (rb.pop(msg)) {
        printf(" [POPPED FROM BUS] Seq: %u, Symbol: %u, Price: %u\n", 
               msg.seq, msg.symbol_id, msg.price);
      }
    }
  });

  // 4. Start WORKER 1: The Network Loop (Producer)
  char buffer[2048];
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  while (true) {
    int n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
    
    if (n > 0) {
      PacketHeader* header = (PacketHeader*)buffer;
      MarketDataMsg* msg_ptr = (MarketDataMsg*)(buffer + sizeof(PacketHeader));
      
      // Push each message from the packet into the Event Bus
      for (int i = 0; i < header->msg_count; i++) {
        // If the bus is full, we just keep trying until there's room
        while (!rb.push(msg_ptr[i])) { 
          std::this_thread::yield(); 
        }
      }
    }
  }

  processor.join();
  return 0;
}