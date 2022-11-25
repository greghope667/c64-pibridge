#include "iec_protocol.hpp"

#include "stream.hpp"

using byte = uint8_t;

/* Logging configuration */
[[maybe_unused]] static void discard(...) {}

#define LOG_STATE stream::logf
#define LOG_COMMAND stream::logf
#define LOG_ERROR stream::logf

#define LOG_DETAILED discard
#define LOG_BYTES discard

namespace iec {

static byte readByte()
{
  byte b = 0;

  for (int i=0; i<8; i++)
  {
    untilRisingEdge(clk_bus);
    b |= ((byte)data_bus.isHigh() << i);
  }

  untilLow(clk_bus);
  LOG_BYTES("Received: %02x", b);
  return b;
}

static void sendByte(byte b)
{
  for (int i=0; i<8; i++)
  {
    clk_bus.hold();
    data_bus.release();
    busy_wait_us(100);
    data_bus.write(not (b & (1 << i)));
    busy_wait_us(100);
    clk_bus.release();
    busy_wait_us(100);
  }
  clk_bus.hold();
  untilLow(data_bus);
  LOG_BYTES("Sent: %02x", b);
  busy_wait_us(200);
}

void command::print()
{
  switch (type) {
    case COMMAND_TYPE::LISTEN:
      LOG_COMMAND("Listen: %x", data);
      break;
    case COMMAND_TYPE::UNLISTEN:
      LOG_COMMAND("Unlisten");
      break;
    case COMMAND_TYPE::TALK:
      LOG_COMMAND("Talk: %x", data);
      break;
    case COMMAND_TYPE::UNTALK:
      LOG_COMMAND("Untalk");
      break;
    case COMMAND_TYPE::REOPEN:
      LOG_COMMAND("Reopen: %x", data);
      break;
    case COMMAND_TYPE::CLOSE:
      LOG_COMMAND("Close: %x", data);
      break;
    case COMMAND_TYPE::OPEN:
      LOG_COMMAND("Open: %x", data);
      break;
    default:
      LOG_COMMAND("Unknown: %x", data);
      break;
  }
}

void serial::tick()
{
  auto prev = state;
  checkAttn();

  switch (state) {
    case STATE::IDLE:
      doIdle();
      break;

    case STATE::TALK:
      doTalk();
      break;

    case STATE::LISTEN:
      doListen();
      break;

    case STATE::ATTN:
      doAttn();
      break;

    case STATE::TURNAROUND:
      doTurnaround();
      break;
  }

  if (state != prev) {
    LOG_STATE("State changed: %s -> %s", toChars(prev), toChars(state));
  }
}

void serial::checkAttn() {
  bool atn = atn_bus.read();
  bool clk = clk_bus.read();

  if (atn && clk &&
      state != STATE::ATTN) {

    LOG_DETAILED("Atn Received");
    listener = false;
    talker = false;
    state = STATE::ATTN;
  } else

  if ((not atn) &&
      state == STATE::ATTN) {
    LOG_DETAILED("Atn Exited");
    state = STATE::TURNAROUND;
  }
}

void serial::processCommand(byte b) {
  command cmd(b);
  cmd.print();
  switch (cmd.type) {
    case COMMAND_TYPE::LISTEN:
      listener = true;
      device_id = cmd.data;
      stream::setDevice(device_id);
      break;
    case COMMAND_TYPE::UNLISTEN:
      listener = false;
      break;
    case COMMAND_TYPE::TALK:
      talker = true;
      device_id = cmd.data;
      break;
    case COMMAND_TYPE::UNTALK:
      talker = false;
      break;
    case COMMAND_TYPE::REOPEN:
      channel_id = cmd.data;
      stream::setChannel(channel_id);
      stream::set(FRAME_REOPEN);
      stream::flush();
      break;
    case COMMAND_TYPE::CLOSE:
      channel_id = cmd.data;
      stream::setChannel(channel_id);
      stream::set(FRAME_CLOSE);
      stream::flush();
      break;
    case COMMAND_TYPE::OPEN:
      channel_id = cmd.data;
      stream::setChannel(channel_id);
      stream::set(FRAME_OPEN);
      stream::flush();
      break;
    default:
      break;
  }
}

byte serial::receive() {
  LOG_DETAILED("Receiving data");
  busy_wait_us(LISTENER_HOLD_OFF);

  untilHigh(data_bus);
  eoi = false;

  if (untilLow(clk_bus, EOI_TIME)) {
  } else {
    LOG_DETAILED("EOI");
    data_bus.hold();
    busy_wait_us(EOI_HOLD);
    data_bus.release();
    untilLow(clk_bus);
    eoi = true;
  }

  bus::timeout_occured = false;

  auto b = readByte();

  if (bus::timeout_occured) {
    LOG_ERROR("Timeout during read");
    state = STATE::IDLE;
    return 0;
  } else {
    busy_wait_us(FRAME_HANDSHAKE);
    data_bus.hold();
    return b;
  }
}

void serial::doIdle() {
  clk_bus.release();
  data_bus.release();
}

void serial::doListen() {
  clk_bus.release();
  data_bus.hold();

  if (clk_bus.isHigh()) {
    auto b = receive();
    if (not bus::timeout_occured) {
      stream::write(b);
      if (eoi) {
        stream::set(FRAME_EOI);
        stream::flush();
      }
    }
  }
}

void serial::doAttn() {
  clk_bus.release();
  data_bus.hold();

  if (clk_bus.isHigh()) {
    auto b = receive();
    if (not bus::timeout_occured) {
      processCommand(b);
    }
  }
}

void serial::doTurnaround() {
  if (listener) {
    state = STATE::LISTEN;
  } else if (talker) {
    // Expect turn around
    bus::timeout_occured = false;
    if (untilHigh(clk_bus, 100)) {
      state = STATE::TALK;
    } else {
      LOG_ERROR("Error during turnaround");
    }
    clk_bus.hold();
    busy_wait_us(200);
  } else {
    state = STATE::IDLE;
  }
}

void serial::doTalk() {
  data_bus.release();

  if (stream::available()) {
    clk_bus.release();
  } else {
    clk_bus.hold();
  }

  if (data_bus.isHigh()) {
    auto b = stream::read();

    bus::timeout_occured = false;
    LOG_DETAILED("Talking");
    if (stream::eoi()) {
      LOG_DETAILED("Sending EOI");
      untilLow(data_bus);
      untilHigh(data_bus);
      busy_wait_us(20);
    }
    sendByte(b);
    busy_wait_us(200);

    if (bus::timeout_occured) {
      LOG_ERROR("Timout during talk");
      state = STATE::IDLE;
    }
  }
}
} // namespace iec
