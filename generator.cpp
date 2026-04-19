#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#pragma pack(push, 1)

struct PacketHeader {
    uint32_t seq_start;
    uint16_t msg_count;
};

struct MarketDataMsg {
    uint64_t timestamp;
    uint32_t seq;
    uint32_t symbol_id;
    uint32_t price;
    uint32_t qty;
    uint8_t side;
    uint8_t type;
};

#pragma pack(pop)

uint64_t now_ns() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main () {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    uint32_t global_seq = 1;

    while (true) {
        for (int i = 0; i < 10000; i++) {
            if (global_seq % 1000 == 0) {
                global_seq++;
                continue;
        }

        PacketHeader msg{global_seq++, rand() % 1000, rand() % 10};

        sendto(fd, &msg, sizeof(msg), 0, (sockaddr*)&addr, sizeof(addr));
    }
    usleep(1000);
    }
    close(sock);
    return 0;
}
    
