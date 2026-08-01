#pragma once

#include "esphome/components/select/select.h"
#include "../ld2460.h"

namespace esphome::ld2460 {

class LD2460InstallationModeSelect : public select::Select, public Parented<LD2460Component> {
 protected:
  void control(size_t index) override;
};

}  // namespace esphome::ld2460
