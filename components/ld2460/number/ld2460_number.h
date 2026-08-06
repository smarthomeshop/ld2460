#pragma once

#include "esphome/components/number/number.h"
#include "../ld2460.h"

namespace esphome::ld2460 {

class LD2460InstallationParameterNumber : public number::Number, public Parented<LD2460Component> {
 public:
  explicit LD2460InstallationParameterNumber(InstallationParameterType type) : type_(type) {}

 protected:
  void control(float value) override;

  InstallationParameterType type_;
};

}  // namespace esphome::ld2460
