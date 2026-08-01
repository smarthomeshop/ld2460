#include "ld2460.h"

#ifdef USE_SELECT
#include "select/ld2460_select.h"
#endif

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <cctype>
#include <cinttypes>
#include <cmath>
#include <cstdio>

namespace esphome {
namespace ld2460 {

static const char *const TAG = "ld2460";
static const uint32_t BAUD_RATES[] = {115200, 9600, 19200, 38400, 57600, 230400, 256000, 460800};
static const float RAD_TO_DEG = 57.2957795131f;

void LD2460Component::setup() { this->rx_buffer_.reserve(this->max_buffer_size_); }

void LD2460Component::dump_config() {
  ESP_LOGCONFIG(TAG, "HLK-LD2460 raw UART reader:");
  ESP_LOGCONFIG(TAG, "  Flush timeout: %" PRIu32 " ms", this->flush_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Max buffer size: %u byte(s)", this->max_buffer_size_);
  ESP_LOGCONFIG(TAG, "  Baud scan: %s", YESNO(this->baud_scan_));
  ESP_LOGCONFIG(TAG, "  Enable reporting on boot: %s", YESNO(this->enable_reporting_));
  ESP_LOGCONFIG(TAG, "  No-data log interval: %" PRIu32 " ms", this->no_data_log_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Publish interval: %" PRIu32 " ms", this->publish_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Report log interval: %" PRIu32 " ms", this->report_log_interval_ms_);
  this->check_uart_settings(115200, 1, uart::UART_CONFIG_PARITY_NONE, 8);
  LOG_TEXT_SENSOR("  ", "Raw UART", this->raw_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Summary", this->summary_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Firmware", this->firmware_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Installation Mode", this->installation_mode_text_sensor_);
  LOG_BINARY_SENSOR("  ", "Presence", this->presence_binary_sensor_);
  LOG_SENSOR("  ", "Target Count", this->target_count_sensor_);
  LOG_SENSOR("  ", "Byte Count", this->byte_count_sensor_);
  for (uint8_t i = 0; i < MAX_TARGETS; i++) {
    const auto target = this->target_sensors_[i];
    char prefix[24];
    std::snprintf(prefix, sizeof(prefix), "  Target %u ", static_cast<unsigned>(i + 1));
    LOG_SENSOR(prefix, "X", target.x);
    LOG_SENSOR(prefix, "Y", target.y);
    LOG_SENSOR(prefix, "Distance", target.distance);
    LOG_SENSOR(prefix, "Angle", target.angle);
  }
}

void LD2460Component::loop() {
  const uint32_t now = millis();

  if (this->enable_reporting_ && this->total_bytes_ == 0 &&
      (!this->startup_commands_sent_ || now - this->last_command_ms_ >= 10000) && now > 2000) {
    this->select_next_baud_rate_();
    this->send_startup_commands_();
    this->startup_commands_sent_ = true;
    this->last_command_ms_ = now;
  }

  uint8_t byte;
  while (this->available() > 0) {
    if (!this->read_byte(&byte))
      break;

    this->rx_buffer_.push_back(byte);
    this->total_bytes_++;
    this->last_byte_ms_ = now;

    this->process_rx_buffer_();

    if (this->rx_buffer_.size() >= this->max_buffer_size_) {
      this->flush_unparsed_buffer_();
    }
  }

  if (!this->rx_buffer_.empty() && now - this->last_byte_ms_ >= this->flush_timeout_ms_)
    this->flush_unparsed_buffer_();

  if (this->total_bytes_ == 0 && this->no_data_log_interval_ms_ > 0 &&
      now - this->last_no_data_log_ms_ >= this->no_data_log_interval_ms_) {
    ESP_LOGW(TAG, "No UART bytes received yet on RX. Check LD2460 TX -> ESP RX, common GND, power, and baud.");
    this->last_no_data_log_ms_ = now;
  }
}

void LD2460Component::send_startup_commands_() {
  this->send_enable_reporting_command_();
  this->send_query_installation_mode_command_();
  this->send_query_version_command_();
}

void LD2460Component::send_enable_reporting_command_() {
  static const uint8_t ENABLE_REPORTING[] = {
      0xFD, 0xFC, 0xFB, 0xFA,  // Frame header
      0x06,                    // Open/close reporting function
      0x0C, 0x00,              // Total frame length, little endian
      0x01,                    // Enable reporting
      0x04, 0x03, 0x02, 0x01   // Frame tail
  };
  this->write_array(ENABLE_REPORTING, sizeof(ENABLE_REPORTING));
  this->flush();
  ESP_LOGI(TAG, "Sent LD2460 enable-reporting command.");
}

void LD2460Component::send_query_version_command_() {
  static const uint8_t QUERY_VERSION[] = {
      0xFD, 0xFC, 0xFB, 0xFA,  // Frame header
      0x0B,                    // Query firmware version
      0x0C, 0x00,              // Total frame length, little endian
      0x01,                    // Query payload
      0x04, 0x03, 0x02, 0x01   // Frame tail
  };
  this->write_array(QUERY_VERSION, sizeof(QUERY_VERSION));
  this->flush();
  ESP_LOGI(TAG, "Sent LD2460 query-version command.");
}

void LD2460Component::send_query_installation_mode_command_() {
  static const uint8_t QUERY_INSTALLATION_MODE[] = {
      0xFD, 0xFC, 0xFB, 0xFA,
      0x0A,
      0x0C, 0x00,
      0x01,
      0x04, 0x03, 0x02, 0x01,
  };
  this->write_array(QUERY_INSTALLATION_MODE, sizeof(QUERY_INSTALLATION_MODE));
  this->flush();
  ESP_LOGI(TAG, "Sent LD2460 query-installation-mode command.");
}

void LD2460Component::set_installation_mode(uint8_t mode) {
  if (mode < 1 || mode > 2) {
    ESP_LOGW(TAG, "Invalid LD2460 installation mode: %u", mode);
    return;
  }

  const uint8_t command[] = {
      0xFD, 0xFC, 0xFB, 0xFA,
      0x09,
      0x0C, 0x00,
      mode,
      0x04, 0x03, 0x02, 0x01,
  };
  this->write_array(command, sizeof(command));
  this->flush();
  ESP_LOGI(TAG, "Sent LD2460 installation mode command: %s", installation_mode_to_string_(mode));
}

void LD2460Component::publish_installation_mode_(uint8_t mode) {
  if (mode < 1 || mode > 2)
    return;

  if (this->installation_mode_text_sensor_ != nullptr)
    this->installation_mode_text_sensor_->publish_state(installation_mode_to_string_(mode));
#ifdef USE_SELECT
  if (this->installation_mode_select_ != nullptr)
    this->installation_mode_select_->publish_state(mode - 1);
#endif
}

void LD2460Component::select_next_baud_rate_() {
  if (!this->baud_scan_ && this->startup_commands_sent_)
    return;

  const uint32_t baud_rate = BAUD_RATES[this->baud_index_];
  this->parent_->set_baud_rate(baud_rate);
  this->parent_->load_settings(false);
  ESP_LOGI(TAG, "Testing LD2460 UART baud rate: %" PRIu32, baud_rate);

  if (this->baud_scan_)
    this->baud_index_ = (this->baud_index_ + 1) % (sizeof(BAUD_RATES) / sizeof(BAUD_RATES[0]));
}

void LD2460Component::set_target_x_sensor(uint8_t index, sensor::Sensor *target_x_sensor) {
  if (index < MAX_TARGETS)
    this->target_sensors_[index].x = target_x_sensor;
}

void LD2460Component::set_target_y_sensor(uint8_t index, sensor::Sensor *target_y_sensor) {
  if (index < MAX_TARGETS)
    this->target_sensors_[index].y = target_y_sensor;
}

void LD2460Component::set_target_distance_sensor(uint8_t index, sensor::Sensor *target_distance_sensor) {
  if (index < MAX_TARGETS)
    this->target_sensors_[index].distance = target_distance_sensor;
}

void LD2460Component::set_target_angle_sensor(uint8_t index, sensor::Sensor *target_angle_sensor) {
  if (index < MAX_TARGETS)
    this->target_sensors_[index].angle = target_angle_sensor;
}

void LD2460Component::process_rx_buffer_() {
  while (this->rx_buffer_.size() >= 4) {
    if (!is_report_header_(this->rx_buffer_) && !is_command_header_(this->rx_buffer_)) {
      this->rx_buffer_.erase(this->rx_buffer_.begin());
      continue;
    }

    if (this->rx_buffer_.size() < 7)
      return;

    const uint16_t frame_length = read_u16_le_(this->rx_buffer_, 5);
    if (frame_length < 11 || frame_length > this->max_buffer_size_) {
      ESP_LOGW(TAG, "Invalid LD2460 frame length %u in: %s", frame_length,
               format_frame_(this->rx_buffer_).c_str());
      this->rx_buffer_.erase(this->rx_buffer_.begin());
      continue;
    }

    if (this->rx_buffer_.size() < frame_length)
      return;

    const std::vector<uint8_t> frame(this->rx_buffer_.begin(), this->rx_buffer_.begin() + frame_length);
    this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.begin() + frame_length);

    if (is_report_header_(frame)) {
      if (!has_report_footer_(frame)) {
        ESP_LOGW(TAG, "LD2460 report with invalid footer: %s", format_frame_(frame).c_str());
        continue;
      }
      this->process_report_frame_(frame);
      continue;
    }

    if (is_command_header_(frame)) {
      if (!has_command_footer_(frame)) {
        ESP_LOGW(TAG, "LD2460 command frame with invalid footer: %s", format_frame_(frame).c_str());
        continue;
      }
      this->process_command_frame_(frame);
    }
  }
}

void LD2460Component::process_report_frame_(const std::vector<uint8_t> &frame) {
  const uint8_t function_code = frame[4];
  if (function_code != 0x04) {
    ESP_LOGD(TAG, "LD2460 report-like frame function=0x%02X: %s", function_code, format_frame_(frame).c_str());
    return;
  }

  const uint16_t frame_length = read_u16_le_(frame, 5);
  if (frame_length < 11 || (frame_length - 11) % 4 != 0) {
    ESP_LOGW(TAG, "LD2460 target report has invalid length %u: %s", frame_length, format_frame_(frame).c_str());
    return;
  }

  uint8_t target_count = static_cast<uint8_t>((frame_length - 11) / 4);
  if (target_count > MAX_TARGETS) {
    ESP_LOGW(TAG, "LD2460 reported %u targets; only %u are exposed", target_count, MAX_TARGETS);
    target_count = MAX_TARGETS;
  }

  Target targets[MAX_TARGETS]{};
  std::string summary = "targets=" + std::to_string(target_count);

  for (uint8_t i = 0; i < target_count; i++) {
    const size_t offset = 7 + i * 4;
    const int16_t raw_x = read_i16_le_(frame, offset);
    const int16_t raw_y = read_i16_le_(frame, offset + 2);
    const float x_m = raw_x / 10.0f;
    const float y_m = raw_y / 10.0f;
    const float distance_m = std::sqrt(x_m * x_m + y_m * y_m);
    const float angle_deg = std::atan2(x_m, y_m) * RAD_TO_DEG;

    targets[i].raw_x = raw_x;
    targets[i].raw_y = raw_y;
    targets[i].x_m = x_m;
    targets[i].y_m = y_m;
    targets[i].distance_m = distance_m;
    targets[i].angle_deg = angle_deg;

    char target_summary[96];
    std::snprintf(target_summary, sizeof(target_summary), "; T%u x=%.1fm y=%.1fm d=%.1fm angle=%.1fdeg",
                  static_cast<unsigned>(i + 1), x_m, y_m, distance_m, angle_deg);
    summary += target_summary;
  }

  const uint32_t now = millis();
  if (now - this->last_report_log_ms_ >= this->report_log_interval_ms_) {
    ESP_LOGI(TAG, "LD2460 target report: %s", summary.c_str());
    this->last_report_log_ms_ = now;
  } else {
    ESP_LOGD(TAG, "LD2460 target report: %s", summary.c_str());
  }
  ESP_LOGD(TAG, "LD2460 report raw: %s", format_frame_(frame).c_str());

  if (this->target_state_changed_(targets, target_count) && now - this->last_publish_ms_ >= this->publish_interval_ms_) {
    this->publish_targets_(targets, target_count, summary);
    this->remember_published_targets_(targets, target_count);

    if (this->raw_text_sensor_ != nullptr)
      this->raw_text_sensor_->publish_state(format_frame_(frame));
    if (this->byte_count_sensor_ != nullptr)
      this->byte_count_sensor_->publish_state(this->total_bytes_);
    this->last_publish_ms_ = now;
  }
}

void LD2460Component::process_command_frame_(const std::vector<uint8_t> &frame) {
  const uint8_t function_code = frame[4];
  const uint16_t frame_length = read_u16_le_(frame, 5);
  const size_t payload_length = frame_length - 11;
  const size_t payload_offset = 7;

  switch (function_code) {
    case 0x06: {
      if (payload_length < 1)
        break;
      const uint8_t result = frame[payload_offset];
      const bool success = (result & 0x10) != 0;
      const bool enabled = (result & 0x01) != 0;
      ESP_LOGI(TAG, "LD2460 reporting %s: %s", enabled ? "enable" : "disable", success ? "success" : "failed");
      break;
    }
    case 0x09: {
      if (payload_length < 1)
        break;
      const uint8_t result = frame[payload_offset];
      const bool success = (result & 0x10) != 0;
      const uint8_t mode = result & 0x0F;
      ESP_LOGI(TAG, "LD2460 set installation mode %s: %s", installation_mode_to_string_(mode),
               success ? "success" : "failed");
      if (success)
        this->publish_installation_mode_(mode);
      break;
    }
    case 0x0A: {
      if (payload_length < 1)
        break;
      const char *mode = installation_mode_to_string_(frame[payload_offset]);
      ESP_LOGI(TAG, "LD2460 installation mode: %s", mode);
      this->publish_installation_mode_(frame[payload_offset]);
      break;
    }
    case 0x0B: {
      if (payload_length < 5)
        break;
      const char *mode = installation_mode_to_string_(frame[payload_offset]);
      const uint8_t year = frame[payload_offset + 1];
      const uint8_t month = frame[payload_offset + 2];
      const uint8_t major = frame[payload_offset + 3];
      const uint8_t minor = frame[payload_offset + 4];

      char firmware[48];
      std::snprintf(firmware, sizeof(firmware), "%s V%u.%u (20%02u-%02u)", mode, major, minor, year, month);
      ESP_LOGI(TAG, "LD2460 firmware: %s", firmware);
      if (this->firmware_text_sensor_ != nullptr)
        this->firmware_text_sensor_->publish_state(firmware);
      this->publish_installation_mode_(frame[payload_offset]);
      break;
    }
    default:
      ESP_LOGI(TAG, "LD2460 command/ack function=0x%02X payload=%u byte(s): %s", function_code,
               static_cast<unsigned>(payload_length), format_frame_(frame).c_str());
      break;
  }

  if (this->raw_text_sensor_ != nullptr)
    this->raw_text_sensor_->publish_state(format_frame_(frame));
  if (this->byte_count_sensor_ != nullptr)
    this->byte_count_sensor_->publish_state(this->total_bytes_);
}

void LD2460Component::publish_targets_(const Target *targets, uint8_t target_count, const std::string &summary) {
  if (this->presence_binary_sensor_ != nullptr)
    this->presence_binary_sensor_->publish_state(target_count > 0);

  if (this->target_count_sensor_ != nullptr)
    this->target_count_sensor_->publish_state(target_count);

  if (this->summary_text_sensor_ != nullptr)
    this->summary_text_sensor_->publish_state(summary);

  for (uint8_t i = 0; i < MAX_TARGETS; i++) {
    const bool present = i < target_count;
    const float x = present ? targets[i].x_m : NAN;
    const float y = present ? targets[i].y_m : NAN;
    const float distance = present ? targets[i].distance_m : NAN;
    const float angle = present ? targets[i].angle_deg : NAN;
    const auto target = this->target_sensors_[i];

    if (target.x != nullptr)
      target.x->publish_state(x);
    if (target.y != nullptr)
      target.y->publish_state(y);
    if (target.distance != nullptr)
      target.distance->publish_state(distance);
    if (target.angle != nullptr)
      target.angle->publish_state(angle);
  }
}

bool LD2460Component::target_state_changed_(const Target *targets, uint8_t target_count) const {
  if (!this->has_published_targets_)
    return true;

  if (target_count != this->last_published_target_count_)
    return true;

  for (uint8_t i = 0; i < target_count; i++) {
    if (targets[i].raw_x != this->last_published_targets_[i].raw_x ||
        targets[i].raw_y != this->last_published_targets_[i].raw_y) {
      return true;
    }
  }

  return false;
}

void LD2460Component::remember_published_targets_(const Target *targets, uint8_t target_count) {
  this->last_published_target_count_ = target_count;
  for (uint8_t i = 0; i < MAX_TARGETS; i++) {
    this->last_published_targets_[i] = i < target_count ? targets[i] : Target{};
  }
  this->has_published_targets_ = true;
}

void LD2460Component::flush_unparsed_buffer_() {
  if (this->rx_buffer_.empty())
    return;

  const std::string frame = this->format_frame_(this->rx_buffer_);
  ESP_LOGW(TAG, "Unparsed RX %u byte(s): %s", static_cast<unsigned>(this->rx_buffer_.size()), frame.c_str());

  if (this->raw_text_sensor_ != nullptr)
    this->raw_text_sensor_->publish_state(frame);

  if (this->byte_count_sensor_ != nullptr)
    this->byte_count_sensor_->publish_state(this->total_bytes_);

  this->rx_buffer_.clear();
}

bool LD2460Component::is_report_header_(const std::vector<uint8_t> &bytes) {
  return bytes.size() >= 4 && bytes[0] == 0xF4 && bytes[1] == 0xF3 && bytes[2] == 0xF2 && bytes[3] == 0xF1;
}

bool LD2460Component::is_command_header_(const std::vector<uint8_t> &bytes) {
  return bytes.size() >= 4 && bytes[0] == 0xFD && bytes[1] == 0xFC && bytes[2] == 0xFB && bytes[3] == 0xFA;
}

bool LD2460Component::has_report_footer_(const std::vector<uint8_t> &bytes) {
  return bytes.size() >= 4 && bytes[bytes.size() - 4] == 0xF8 && bytes[bytes.size() - 3] == 0xF7 &&
         bytes[bytes.size() - 2] == 0xF6 && bytes[bytes.size() - 1] == 0xF5;
}

bool LD2460Component::has_command_footer_(const std::vector<uint8_t> &bytes) {
  return bytes.size() >= 4 && bytes[bytes.size() - 4] == 0x04 && bytes[bytes.size() - 3] == 0x03 &&
         bytes[bytes.size() - 2] == 0x02 && bytes[bytes.size() - 1] == 0x01;
}

uint16_t LD2460Component::read_u16_le_(const std::vector<uint8_t> &bytes, size_t index) {
  return static_cast<uint16_t>(bytes[index]) | (static_cast<uint16_t>(bytes[index + 1]) << 8);
}

int16_t LD2460Component::read_i16_le_(const std::vector<uint8_t> &bytes, size_t index) {
  return static_cast<int16_t>(read_u16_le_(bytes, index));
}

const char *LD2460Component::installation_mode_to_string_(uint8_t mode) {
  switch (mode) {
    case 0x01:
      return "side";
    case 0x02:
      return "top";
    default:
      return "unknown";
  }
}

std::string LD2460Component::format_frame_(const std::vector<uint8_t> &bytes) {
  std::string hex;
  hex.reserve(bytes.size() * 3);

  char byte_hex[3];
  for (size_t i = 0; i < bytes.size(); i++) {
    if (i != 0)
      hex += ' ';
    std::snprintf(byte_hex, sizeof(byte_hex), "%02X", bytes[i]);
    hex += byte_hex;
  }

  std::string ascii;
  ascii.reserve(bytes.size());
  for (const auto byte : bytes) {
    if (std::isprint(static_cast<unsigned char>(byte)))
      ascii += static_cast<char>(byte);
    else
      ascii += '.';
  }

  return hex + " | " + ascii;
}

}  // namespace ld2460
}  // namespace esphome
