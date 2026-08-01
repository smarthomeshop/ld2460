#include "ld2460_select.h"

namespace esphome::ld2460 {

void LD2460InstallationModeSelect::control(size_t index) {
  if (index <= 1)
    this->parent_->set_installation_mode(static_cast<uint8_t>(index + 1));
}

}  // namespace esphome::ld2460
