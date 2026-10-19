#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/log.h"

// This header is expected to be provided by the espp/rtsp ESP-IDF component.
#include "rtsp_client.hpp"

namespace esphome {
namespace esp_rtsp_stream {

class RtspMjpeg : public Component {
 public:
  void set_url(const std::string &url) { url_ = url; }
  void set_fps(int fps) { fps_ = fps; }
  void set_expected_size(int width, int height) {
    expected_width_ = width;
    expected_height_ = height;
  }
  void set_log_frame_info(bool enabled) { log_frame_info_ = enabled; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  // Copy the most recently received complete JPEG frame.
  bool copy_latest_frame(std::vector<uint8_t> &out, int &width, int &height);
  uint32_t get_frame_count() const { return frame_count_; }
  uint32_t get_dropped_frames() const { return dropped_frames_; }

 protected:
  bool parse_url_();
  bool start_client_();
  void stop_client_();

  std::string url_;
  std::string host_;
  std::string path_;
  int port_{554};
  int fps_{10};
  int expected_width_{704};
  int expected_height_{576};
  bool log_frame_info_{true};

  std::unique_ptr<espp::RtspClient> client_;
  mutable std::mutex frame_mutex_;
  std::vector<uint8_t> latest_frame_;
  int latest_width_{0};
  int latest_height_{0};
  uint32_t last_frame_ms_{0};
  uint32_t last_report_ms_{0};
  uint32_t frame_count_{0};
  uint32_t dropped_frames_{0};
  bool connected_{false};
  bool reconnect_pending_{false};
};

}  // namespace esp_rtsp_stream
}  // namespace esphome
