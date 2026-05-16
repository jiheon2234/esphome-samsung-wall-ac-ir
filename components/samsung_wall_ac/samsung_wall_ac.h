#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace samsung_wall_ac {

class SamsungWallAC : public climate_ir::ClimateIR {
public:
  SamsungWallAC()
      : climate_ir::ClimateIR(16.0f, 26.0f, 1.0f, false, false, {}, {}, {}) {}

protected:
  climate::ClimateTraits traits() override;
  void transmit_state() override;
  bool on_receive(remote_base::RemoteReceiveData data) override {
    return false;
  }
};

} // namespace samsung_wall_ac
} // namespace esphome