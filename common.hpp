#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>

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

#endif // COMMON_HPP
