#pragma once

#include "bus.hpp"

using byte = uint8_t;

namespace iec {

enum class COMMAND_TYPE : byte
{
  LISTEN,
  UNLISTEN,
  TALK,
  UNTALK,
  REOPEN, // aka SECOND
  CLOSE,
  OPEN,
  ERROR,
};

struct command
{
  COMMAND_TYPE type;
  byte data;

  explicit command(byte b)
    : type(COMMAND_TYPE::ERROR)
    , data(0)
  {
    byte tp = (0xe0 & b) >> 5;
    byte rem = (0x1f & b);
    switch (tp) {
      case 1: // 20..3f Listen
        if (rem != 0x1f) {
          type = COMMAND_TYPE::LISTEN;
          data = rem;
        } else {
          type = COMMAND_TYPE::UNLISTEN;
        }
        break;

      case 2: // 40..5f Talk
        if (rem != 0x1f) {
          type = COMMAND_TYPE::TALK;
          data = rem;
        } else {
          type = COMMAND_TYPE::UNTALK;
        }
        break;

      case 3: // 60..6f reopen
        type = COMMAND_TYPE::REOPEN;
        data = rem & 0xf;
        break;

      case 7: // e0..ff clopen
        if (b & 0x10) {
          type = COMMAND_TYPE::OPEN;
          data = rem & 0xf;
        } else {
          type = COMMAND_TYPE::CLOSE;
          data = rem & 0xf;
        }
        break;

      default:
        break;
    }
  }

  void print();
};

enum class STATE {
  IDLE,
  TALK,
  LISTEN,
  ATTN,
  TURNAROUND,
};

inline const char* toChars(STATE state)
{
  switch (state) {
    case STATE::IDLE:
      return "IDLE";
    case STATE::TALK:
      return "TALK";
    case STATE::LISTEN:
      return "LISTEN";
    case STATE::ATTN:
      return "ATTN";
    case STATE::TURNAROUND:
      return "TURNAROUND";
  }
  return "BAD_STATE";
}

struct serial
{
  static constexpr uint64_t LISTENER_HOLD_OFF = 100;
  static constexpr uint64_t FRAME_HANDSHAKE = 100;
  static constexpr uint64_t EOI_TIME = 200;
  static constexpr uint64_t EOI_HOLD = 100;

  // Initial state : idle, not talking or listening
  bool listener = false;
  bool talker = false;
  STATE state = STATE::IDLE;
  bool eoi = false;
  uint8_t device_id;
  uint8_t channel_id;

  void tick();

private:
  void checkAttn();
  void processCommand(byte b);
  byte receive();

  void doIdle();
  void doListen();
  void doAttn();
  void doTurnaround();
  void doTalk();
};

} // namespace iec
