#include "samsung_wall_ac.h"

#include "esphome/core/log.h"

namespace esphome {
namespace samsung_wall_ac {

static const char *const TAG = "climate.samsung_wall_ac";

namespace {

constexpr uint8_t MIN_TEMP = 16;
constexpr uint8_t MAX_TEMP = 32;

// IR LED를 깜빡이는 반송파 주파수 (38kHz)
constexpr uint32_t CARRIER_FREQ = 38000;

// 시작 헤더 mark: 9132us 동안 IR 송신
constexpr uint32_t HEADER_MARK = 9132;

// 시작 헤더 space: 4579us 동안 IR 꺼짐
constexpr uint32_t HEADER_SPACE = 4579;

// 각 bit 앞에 붙는 공통 mark: 553us 동안 IR 송신 (0015)
constexpr uint32_t BIT_MARK = 553;

// bit 0의 space: 579us 동안 IR 꺼짐
constexpr uint32_t ZERO_SPACE = 579;

// bit 1의 space: 1737us 동안 IR 꺼짐
constexpr uint32_t ONE_SPACE = 1737;

// 패킷 종료 후 final space: 25316us 동안 IR 꺼짐
constexpr uint32_t FINAL_SPACE = 25316;

/*
 * float로 들어온 온도를 정수로 변환한 뒤
 * 지원하는 최소~최대 온도 범위 안의 uint8_t 값으로 제한한다.
 */
uint8_t normalize_temperature(float temperature) {
  int temp = static_cast<int>(temperature);
  if (temp < MIN_TEMP)
    temp = MIN_TEMP;
  if (temp > MAX_TEMP)
    temp = MAX_TEMP;
  return temp;
}

/**

 * 전달된 팬 모드를 삼성 에어컨 IR 패킷에 들어갈 fan byte로 변환
 * 비어있거나, 지원하지 않는 팬 모드가 들어오면 LOW로 보정한다.
 *
 * 반환값:
 *   HIGH   -> 0x20
 *   MEDIUM -> 0x40
 *   LOW    -> 0x60
 */
uint8_t current_fan_byte(const optional<climate::ClimateFanMode> &mode,
                         climate::ClimateFanMode &fan_mode) {
  fan_mode = mode.value_or(climate::CLIMATE_FAN_LOW);
  switch (fan_mode) {
  case climate::CLIMATE_FAN_HIGH:
    return 0x20;
  case climate::CLIMATE_FAN_MEDIUM:
    return 0x40;
  case climate::CLIMATE_FAN_LOW:
  default:
    fan_mode = climate::CLIMATE_FAN_LOW;
    return 0x60;
  }
}

/**
 * LSB-first이므로, 바이트를 오른쪽부터 읽어서 하나씩 보냄
 */
void emit_byte(remote_base::RemoteTransmitData *data, uint8_t byte_value) {
  data->item(BIT_MARK, (byte_value & 0x01) ? ONE_SPACE : ZERO_SPACE);
  data->item(BIT_MARK, (byte_value & 0x02) ? ONE_SPACE : ZERO_SPACE);
  data->item(BIT_MARK, (byte_value & 0x04) ? ONE_SPACE : ZERO_SPACE);
  data->item(BIT_MARK, (byte_value & 0x08) ? ONE_SPACE : ZERO_SPACE);
  data->item(BIT_MARK, (byte_value & 0x10) ? ONE_SPACE : ZERO_SPACE);
  data->item(BIT_MARK, (byte_value & 0x20) ? ONE_SPACE : ZERO_SPACE);
  data->item(BIT_MARK, (byte_value & 0x40) ? ONE_SPACE : ZERO_SPACE);
  data->item(BIT_MARK, (byte_value & 0x80) ? ONE_SPACE : ZERO_SPACE);
}

} // namespace

climate::ClimateTraits SamsungWallAC::traits() {
  auto traits = climate::ClimateTraits();

  traits.set_supported_modes({

      climate::CLIMATE_MODE_OFF,  // 끄기
      climate::CLIMATE_MODE_COOL, // 냉방
  });
  traits.set_supported_fan_modes({
      // 팬1,2,3
      climate::CLIMATE_FAN_LOW,    // 팬1
      climate::CLIMATE_FAN_MEDIUM, // 팬2
      climate::CLIMATE_FAN_HIGH,   // 팬3
  });

  traits.set_visual_min_temperature(MIN_TEMP);
  traits.set_visual_max_temperature(MAX_TEMP);
  traits.set_visual_temperature_step(1.0f);

  return traits;
}

void SamsungWallAC::transmit_state() {
  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  data->set_carrier_frequency(CARRIER_FREQ); // 주파수
  data->item(HEADER_MARK, HEADER_SPACE);     // 패킷 헤더

  if (this->mode == climate::CLIMATE_MODE_OFF) { // 종료일땐 그냥 하드코딩
    // ESP_LOGD(TAG, "TX OFF");
    // ESP_LOGD(TAG,
    //          "TX bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X
    //          %02X "
    //          "%02X %02X",
    //          0xC3, 0x7F, 0xE0, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
    //          0x00, 0x05, 0x87);

    emit_byte(data, 0xC3);
    emit_byte(data, 0x7F);
    emit_byte(data, 0xE0);
    emit_byte(data, 0x00);
    emit_byte(data, 0x60);
    emit_byte(data, 0x00);
    emit_byte(data, 0x00);
    emit_byte(data, 0x00);
    emit_byte(data, 0x00);
    emit_byte(data, 0x00);
    emit_byte(data, 0x00);
    emit_byte(data, 0x05);
    emit_byte(data, 0x87);
  } else if (this->mode == climate::CLIMATE_MODE_COOL) {
    const uint8_t temp = normalize_temperature(this->target_temperature);
    climate::ClimateFanMode fan_mode;
    const uint8_t b0 = 0xC3;
    const uint8_t b1 =
        static_cast<uint8_t>(0x47 + (temp - MIN_TEMP) * 0x08);     // 온도공식
    const uint8_t b2 = 0xE0;                                       // 고정
    const uint8_t b3 = 0x00;                                       // 고정
    const uint8_t b4 = current_fan_byte(this->fan_mode, fan_mode); // 팬공식
    const uint8_t b5 = 0x00;                                       // 고정
    const uint8_t b6 = 0x20;                                       // 고정
    const uint8_t b7 = 0x00;                                       // 고정
    const uint8_t b8 = 0x00;                                       // 고정
    const uint8_t b9 = 0x20;                                       // 고정
    const uint8_t b10 = 0x00;                                      // 고정
    const uint8_t b11 = 0x00; // 고정 (미확인)
    const uint8_t b12 =
        static_cast<uint8_t>(b0 + b1 + b2 + b3 + b4 + b5 + b6 + b7 + b8 + b9 +
                             b10 + b11); // 체크섬공식 ( ? & 0xFF)

    this->target_temperature = temp;
    this->fan_mode = fan_mode;

    // ESP_LOGD(TAG, "TX COOL temp=%u fan=%d", temp, fan_mode);
    // ESP_LOGD(TAG,
    //          "TX bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X
    //          %02X "
    //          "%02X %02X",
    //          b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12);

    emit_byte(data, b0);
    emit_byte(data, b1);
    emit_byte(data, b2);
    emit_byte(data, b3);
    emit_byte(data, b4);
    emit_byte(data, b5);
    emit_byte(data, b6);
    emit_byte(data, b7);
    emit_byte(data, b8);
    emit_byte(data, b9);
    emit_byte(data, b10);
    emit_byte(data, b11);
    emit_byte(data, b12);
  } else {
    // ESP_LOGW(TAG, "Unsupported mode: %d", this->mode);
    return;
  }

  data->item(BIT_MARK, FINAL_SPACE);
  transmit.perform();
}

} // namespace samsung_wall_ac
} // namespace esphome
