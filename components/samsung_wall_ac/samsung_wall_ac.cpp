#include "samsung_wall_ac.h"
#include "esphome/core/log.h"

namespace esphome {
namespace samsung_wall_ac {

static const char *const TAG = "climate.samsung_wall_ac";

namespace {

constexpr uint32_t CARRIER_FREQ = 38000;
constexpr uint32_t HEADER_MARK = 9132;
constexpr uint32_t HEADER_SPACE = 4579;
constexpr uint32_t BIT_MARK = 553;
constexpr uint32_t ZERO_SPACE = 579;
constexpr uint32_t ONE_SPACE = 1737;
constexpr uint32_t FINAL_SPACE = 25316;

uint8_t encode_temperature_byte(uint8_t temp) {
  static constexpr uint8_t table[11] = {
      0xE2, // 16
      0xF2, // 17
      0xEA, // 18
      0xFA, // 19
      0xE6, // 20
      0xF6, // 21
      0xEE, // 22
      0xFE, // 23
      0xE1, // 24
      0xF1, // 25
      0xE9, // 26
  };

  if (temp < 16)
    temp = 16;
  if (temp > 26)
    temp = 26;

  return table[temp - 16];
}

uint8_t encode_checksum_byte(uint8_t temp) {
  static constexpr uint8_t table[11] = {
      0xF1, // 16
      0xE9, // 17
      0xF9, // 18
      0xE5, // 19
      0xF5, // 20
      0xED, // 21
      0xFD, // 22
      0xE3, // 23
      0xF3, // 24
      0xEB, // 25
      0xFB, // 26
  };

  if (temp < 16)
    temp = 16;
  if (temp > 26)
    temp = 26;

  return table[temp - 16];
}

void fill_cool_packet(uint8_t temp, uint8_t packet[13]) {
  packet[0] = 0xC3;
  packet[1] = encode_temperature_byte(temp);
  packet[2] = 0x07;
  packet[3] = 0x00;
  packet[4] = 0x06;
  packet[5] = 0x00;
  packet[6] = 0x04;
  packet[7] = 0x00;
  packet[8] = 0x00;
  packet[9] = 0x04;
  packet[10] = 0x00;
  packet[11] = 0xA0;
  packet[12] = encode_checksum_byte(temp);
}

void fill_off_packet(uint8_t packet[13]) {
  packet[0] = 0xC3;
  packet[1] = 0xFE;
  packet[2] = 0x07;
  packet[3] = 0x00;
  packet[4] = 0x06;
  packet[5] = 0x00;
  packet[6] = 0x00;
  packet[7] = 0x00;
  packet[8] = 0x00;
  packet[9] = 0x00;
  packet[10] = 0x00;
  packet[11] = 0xA0;
  packet[12] = 0xE1;
}

} // namespace

climate::ClimateTraits SamsungWallAC::traits() {
  auto traits = climate::ClimateTraits();

  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_COOL,
  });

  traits.set_visual_min_temperature(16);
  traits.set_visual_max_temperature(26);
  traits.set_visual_temperature_step(1);

  return traits;
}

void SamsungWallAC::transmit_state() {
  uint8_t packet[13] = {0};

  if (this->mode == climate::CLIMATE_MODE_OFF) {
    ESP_LOGD(TAG, "TX OFF");
    fill_off_packet(packet);
  } else if (this->mode == climate::CLIMATE_MODE_COOL) {
    const uint8_t temp = this->target_temperature;
    ESP_LOGD(TAG, "TX COOL temp=%u", temp);
    fill_cool_packet(temp, packet);
  } else {
    ESP_LOGW(TAG, "Unsupported mode: %d", this->mode);
    return;
  }

  ESP_LOGD(TAG,
           "TX bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X "
           "%02X %02X",
           packet[0], packet[1], packet[2], packet[3], packet[4], packet[5],
           packet[6], packet[7], packet[8], packet[9], packet[10], packet[11],
           packet[12]);

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  data->set_carrier_frequency(CARRIER_FREQ); // 주파수
  data->item(HEADER_MARK, HEADER_SPACE);     // 패킷 헤더

  for (uint8_t byte : packet) { // MSB(왼쪽)부터 1비트씩 읽어서 마스킹함
    for (int bit = 7; bit >= 0; bit--) {
      const bool one = byte & (1 << bit);
      data->item(BIT_MARK,
                 one ? ONE_SPACE : ZERO_SPACE); // 553us 깜빡인 후, 1이면 1737us
                                                // 꺼짐, 0이면 579us 꺼짐
    }
  }

  data->item(BIT_MARK, FINAL_SPACE);
  transmit.perform();
}

} // namespace samsung_wall_ac
} // namespace esphome