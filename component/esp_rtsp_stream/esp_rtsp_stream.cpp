#include "esp_rtsp_stream.h"

#include <system_error>

namespace esphome {
namespace esp_rtsp_stream {

static const char *const TAG = "esp_rtsp_stream";

bool RtspMjpeg::parse_url_() {
  constexpr const char *prefix = "rtsp://";
  if (url_.rfind(prefix, 0) != 0) {
    ESP_LOGE(TAG, "URL must start with rtsp://");
    return false;
  }

  std::string rest = url_.substr(7);
  // Authentication is deliberately not implied by this prototype.
  if (rest.find('@') != std::string::npos) {
    ESP_LOGE(TAG, "RTSP user-info credentials are not supported by this prototype");
    return false;
  }

  const auto slash = rest.find('/');
  const std::string authority = slash == std::string::npos ? rest : rest.substr(0, slash);
  path_ = slash == std::string::npos ? "/" : rest.substr(slash);

  if (authority.empty()) {
    ESP_LOGE(TAG, "Missing RTSP host");
    return false;
  }

  const auto colon = authority.rfind(':');
  if (colon == std::string::npos) {
    host_ = authority;
    port_ = 554;
  } else {
    host_ = authority.substr(0, colon);
    try {
      port_ = std::stoi(authority.substr(colon + 1));
    } catch (...) {
      ESP_LOGE(TAG, "Invalid RTSP port");
      return false;
    }
  }

  if (host_.empty() || port_ < 1 || port_ > 65535) {
    ESP_LOGE(TAG, "Invalid RTSP host or port");
    return false;
  }
  return true;
}

void RtspMjpeg::setup() {
  if (fps_ < 1 || fps_ > 30) {
    ESP_LOGE(TAG, "fps must be in range 1..30");
    mark_failed();
    return;
  }
  if (!parse_url_()) {
    mark_failed();
    return;
  }
  reconnect_pending_ = !start_client_();
  last_report_ms_ = millis();
}

bool RtspMjpeg::start_client_() {
  // The upstream espp/rtsp v1.3.8 API must be checked against the installed
  // component headers; this is the intended callback/configuration shape.
  espp::RtspClient::Config cfg{
      .server_address = host_,
      .rtsp_port = port_,
      .path = path_,
      .on_jpeg_frame = [this](std::shared_ptr<espp::JpegFrame> frame) {
        if (!frame || !frame->is_complete()) return;

        const uint32_t now = millis();
        const uint32_t interval = 1000U / static_cast<uint32_t>(fps_);
        if (last_frame_ms_ != 0 && now - last_frame_ms_ < interval) {
          ++dropped_frames_;
          return;
        }

        const auto bytes = frame->get_data();
        if (bytes.empty()) return;

        {
          std::lock_guard<std::mutex> lock(frame_mutex_);
          latest_frame_.assign(bytes.begin(), bytes.end());
          latest_width_ = frame->get_width();
          latest_height_ = frame->get_height();
        }
        last_frame_ms_ = now;
        ++frame_count_;

        if (log_frame_info_ &&
            (frame_count_ == 1 || now - last_report_ms_ >= 5000)) {
          last_report_ms_ = now;
          ESP_LOGI(TAG, "Frame %u: %dx%d, %u bytes; target %dx%d @ %d fps",
                   static_cast<unsigned>(frame_count_),
                   frame->get_width(), frame->get_height(),
                   static_cast<unsigned>(bytes.size()),
                   expected_width_, expected_height_, fps_);
          if (frame->get_width() != expected_width_ ||
              frame->get_height() != expected_height_) {
            ESP_LOGW(TAG, "Received dimensions differ from expected dimensions");
          }
        }
      },
      .on_connection_lost = [this]() {
        connected_ = false;
        reconnect_pending_ = true;
        last_report_ms_ = millis();
        ESP_LOGW(TAG, "RTSP connection lost");
      },
      .log_level = espp::Logger::Verbosity::ERROR,
  };

  client_.reset(new espp::RtspClient(cfg));
  std::error_code ec;
  client_->connect(ec);
  if (!ec) client_->describe(ec);
  if (!ec) client_->setup(ec);
  if (!ec) client_->play(ec);

  if (ec) {
    ESP_LOGE(TAG, "RTSP startup failed: %s", ec.message().c_str());
    stop_client_();
    return false;
  }

  connected_ = true;
  ESP_LOGI(TAG, "RTSP PLAY started; expecting MJPEG over RTP");
  return true;
}

void RtspMjpeg::stop_client_() {
  if (client_) {
    std::error_code ec;
    client_->disconnect(ec);
    client_.reset();
  }
  connected_ = false;
}

void RtspMjpeg::loop() {
  if (is_failed() || connected_ || !reconnect_pending_) return;
  if (millis() - last_report_ms_ < 5000) return;

  stop_client_();
  reconnect_pending_ = !start_client_();
  last_report_ms_ = millis();
}

bool RtspMjpeg::copy_latest_frame(std::vector<uint8_t> &out, int &width, int &height) {
  std::lock_guard<std::mutex> lock(frame_mutex_);
  if (latest_frame_.empty()) return false;
  out = latest_frame_;
  width = latest_width_;
  height = latest_height_;
  return true;
}

void RtspMjpeg::dump_config() {
  ESP_LOGCONFIG(TAG, "RTSP MJPEG receiver:");
  ESP_LOGCONFIG(TAG, "  Server: %s:%d%s", host_.c_str(), port_, path_.c_str());
  ESP_LOGCONFIG(TAG, "  FPS limit: %d", fps_);
  ESP_LOGCONFIG(TAG, "  Expected resolution: %dx%d", expected_width_, expected_height_);
  ESP_LOGCONFIG(TAG, "  RTSP authentication: not implemented in this prototype");
}

}  // namespace esp_rtsp_stream
}  // namespace esphome
