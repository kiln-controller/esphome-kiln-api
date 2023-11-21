#include "kiln_api.h"
#include "esphome.h"

namespace esphome {
namespace kiln_api {

static const char *TAG = "kiln_api";

void KilnApi::setup() {
  base_->add_handler(this);
  base_->add_handler(&this->kiln_state_);

  // push state every 10s
  this->set_interval(10000, [this]() { this->kiln_state_.send(this->get_state().c_str(), "state", millis(), 30000); });
}

void KilnApi::dump_config() {
  ESP_LOGCONFIG(TAG, "Listening on path '/kiln'");
}

bool KilnApi::canHandle(AsyncWebServerRequest *request) {
  if (request->url().startsWith("/kiln/schedule")) {
    return true;
  }
  return false;
}

void KilnApi::handle_schedule_request(AsyncWebServerRequest *request) {
  if (request->method() == HTTP_OPTIONS) {
    // basic CORS response
    AsyncWebServerResponse *response = request->beginResponse(204);
    response->addHeader("Access-Control-Allow-Methods", "OPTIONS, DELETE, POST");
    response->addHeader("Access-Control-Allow-Headers", "*");
    response->addHeader("Access-Control-Max-Age", "86400");
    request->send(response);

  } else if (request->method() == HTTP_DELETE) {
    // cancel current run and shutdown kiln
    ESP_LOGI(TAG, "Cancelled schedule %s, shutdown kiln", this->schedule_name.c_str());
    kiln_->target_temperature = 0;
    this->reset_progress();
    request->send(200, "application/json", "{\"status\": \"ok\"}");

  } else if (request->method() == HTTP_POST) {
    // reset current progress
    this->reset_progress();

    if (request->_tempObject != NULL) {
      // https://github.com/esphome/esphome/blob/d82471942f91a85fe5e4812edfd954f47af74c2b/esphome/components/mqtt/mqtt_client.cpp#L413
      json::parse_json(std::string((char *)request->_tempObject), [&](JsonObject x) {
        if (x.containsKey("name") && x.containsKey("schedule")) {
          this->schedule_name.assign(x["name"].as<std::string>());
          JsonArrayConst parsed = x["schedule"].as<JsonArrayConst>();
          for(JsonVariantConst step : parsed) {
            int s[3];
            copyArray(step, s);
            this->schedule.push_back(std::to_array(s));
          }
        } else {
          request->send(500, "application/json", "{\"status\": \"error\", \"reason: \"missing name or schedule key in JSON\"}");
        }
      });
      request->send(200, "application/json", "{\"status\": \"ok\"}");
    } else { // no _tempObject
      request->send(500, "application/json", "{\"status\": \"error\", \"reason: \"invalid JSON body\"}");
    }
  } else {  // unsupported method
    request->send(405);
  }
}

void KilnApi::handleRequest(AsyncWebServerRequest *request) {
  String path = request->url().substring(5);  // strip /kiln
  if (path == "/schedule") {
    this->handle_schedule_request(request);
    return;
  }
}

void KilnApi::handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  // https://github.com/me-no-dev/ESPAsyncWebServer/blob/f71e3d427b5be9791a8a2c93cf8079792c3a9a26/src/AsyncJson.h#L241
  if (total > 0 && request->_tempObject == NULL) {
    request->_tempObject = malloc(total);
  }
  if (request->_tempObject != NULL) {
    memcpy((uint8_t*)(request->_tempObject) + index, data, len);
  }
}

bool KilnApi::isRequestHandlerTrivial() { return false; }

RequestHandler::RequestHandler(KilnApi *parent) {
  parent->set_request_handler(this);
}

void KilnApi::reset_progress() {
  this->schedule.clear();
  this->schedule_name = "";
  this->current_step = 0;
  this->remaining_hold = -1;
  this->runtime = 0;
}

std::string KilnApi::get_state() {
  return json::build_json([this](JsonObject root) {
    root["step"] = this->current_step;
    root["runtime"] = this->runtime;
    root["temperature"] = this->kiln_->current_temperature;
    root["schedule"]["name"] = this->schedule_name;
    root["schedule"]["steps"] = this->schedule;
  });
}

// controlloop, called every second
void KilnApi::update() {
  // id(kiln).mode = climate::CLIMATE_MODE_OFF;
  // id(kiln).action = climate::CLIMATE_ACTION_OFF;
  if (this->schedule.empty()) {
    return;
  }

  // shutdown kiln when last step done
  if(this->current_step >= this->schedule.size()) {
    ESP_LOGI(TAG, "Shutdown kiln");
    kiln_->target_temperature = 0;
    this->reset_progress();
    return;
  }

  // update runtime
  this->runtime++;

  // extract data
  int ramp = this->schedule[current_step][0];
  int target = this->schedule[current_step][1];
  int hold = this->schedule[current_step][2];
  // divide 3600 to make ramp calculated each 1s
  float ramp_calculated = (float)ramp / (float)3600;

  // if target is lower then previous it is a cooling cycle, negate ramp_calculated
  if(current_step != 0 && target < this->schedule[current_step-1][1]) {
    ESP_LOGD(TAG, "Cooling step, negating ramp");
    ramp_calculated = -ramp_calculated;
  }

  // check if target from step reached
  // TODO: check this with real temp or on PID state?
  // id(kiln).action == climate::CLIMATE_ACTION_IDLE
  if (
    this->remaining_hold < 0
    && (
      (ramp_calculated > 0 && kiln_->current_temperature >= target) || (ramp_calculated < 0 && kiln_->current_temperature <= target)
    )
  ) {
    ESP_LOGI(TAG, "Target reached");
    if (hold > 0) {
      // set seconds to hold
      this->remaining_hold = hold * 60;
      ESP_LOGI(TAG, "Hold started, duration: %ih %im %is", this->remaining_hold/3600, (this->remaining_hold % 3600) /60, this->remaining_hold % 60);
    } else {
      // move to next step, no hold
      current_step++;
      return;
    }
  }
  // prepare when hold is at last iteration, next iteration will use new schedule
  if (this->remaining_hold == 1) {
    ESP_LOGI(TAG, "Hold done");
    // reset hold
    this->remaining_hold = -1;
    // move to next step
    this->current_step++;
    return;
  // just keep the hold going
  } else if (this->remaining_hold > 1) {
    this->remaining_hold--;
    ESP_LOGI(TAG, "Hold time remaining: %ih %im %is", this->remaining_hold/3600, (this->remaining_hold % 3600) /60, this->remaining_hold % 60);
    return;
  }

  // set temperature when needed
  float new_target = kiln_->target_temperature + ramp_calculated;
  // when overshoot, just set target
  if((ramp_calculated > 0 && new_target >= target) || (ramp_calculated < 0 && new_target <= target)) {
    new_target = target;
  }
  if (new_target != kiln_->target_temperature) {
    ESP_LOGI(TAG, "Set target to %.1f", new_target);
    kiln_->target_temperature = new_target;
  }
}

// std::string KilnApi::binary_sensor_json(binary_sensor::BinarySensor *obj, bool value, JsonDetail start_config) {
//   return json::build_json([obj, value, start_config](JsonObject root) {
//     set_json_state_value(root, obj, "binary_sensor-" + obj->get_object_id(), value ? "ON" : "OFF", value, start_config);
//   });
// }

}  // namespace kiln_api
}  // namespace esphome

// https://arduinojson.org/v6/how-to/create-converters-for-stl-containers/
namespace ARDUINOJSON_NAMESPACE {
template <typename T>
struct Converter<std::vector<T> > {
  static void toJson(const std::vector<T>& src, JsonVariant dst) {
    JsonArray array = dst.to<JsonArray>();
    for (T item : src)
      array.add(item);
  }

  static std::vector<T> fromJson(JsonVariantConst src) {
    std::vector<T> dst;
    for (T item : src.as<JsonArrayConst>())
      dst.push_back(item);
    return dst;
  }

  static bool checkJson(JsonVariantConst src) {
    JsonArrayConst array = src;
    bool result = array;
    for (JsonVariantConst item : array)
      result &= item.is<T>();
    return result;
  }
};
}  // namespace ARDUINOJSON_NAMESPACE

namespace ARDUINOJSON_NAMESPACE {
template <typename T, size_t N>
struct Converter<std::array<T, N> > {
  static void toJson(const std::array<T, N>& src, JsonVariant dst) {
    JsonArray array = dst.to<JsonArray>();
    for (T item : src)
      array.add(item);
  }

  static std::array<T, N> fromJson(JsonVariantConst src) {
    std::array<T, N> dst;
    size_t idx = 0;
    for (T item : src.as<JsonArrayConst>())
      dst[idx++] = item;
    return dst;
  }

  static bool checkJson(JsonVariantConst src) {
    JsonArrayConst array = src;
    bool result = array;
    size_t size = 0;
    for (JsonVariantConst item : array) {
      result &= item.is<T>();
      size++;
    }
    return result && size == N;
  }
};
}  // namespace ARDUINOJSON_NAMESPACE
