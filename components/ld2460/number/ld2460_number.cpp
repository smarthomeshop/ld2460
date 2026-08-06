#include "ld2460_number.h"

namespace esphome::ld2460 {

void LD2460InstallationParameterNumber::control(float value) {
  this->parent_->set_installation_parameter(this->type_, value);
}

}  // namespace esphome::ld2460
