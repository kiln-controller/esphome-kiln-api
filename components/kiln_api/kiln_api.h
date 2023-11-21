#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/components/pid/pid_climate.h"

namespace esphome {
namespace kiln_api {

class RequestHandler;

class KilnApi : public PollingComponent, public AsyncWebHandler {
 public:
  KilnApi(web_server_base::WebServerBase *base, pid::PIDClimate *kiln) : PollingComponent(1000), base_(base), kiln_(kiln) {}

  void setup() override;
  void update() override;
  void dump_config() override;

  void set_request_handler(RequestHandler *handler) { handler_ = handler; };
  bool canHandle(AsyncWebServerRequest *request) override;
  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override;
  void handleRequest(AsyncWebServerRequest *request) override;
  bool isRequestHandlerTrivial() override;

  // handle /schedule request
  void handle_schedule_request(AsyncWebServerRequest *request);

  // return json to sent as event
  std::string get_state();

  // reset kiln to ready state
  void reset_progress();

 protected:
  web_server_base::WebServerBase *base_;
  AsyncEventSource kiln_state_{"/kiln/state"};
  pid::PIDClimate *kiln_;
  RequestHandler *handler_;

  // runtime in seconds
  int runtime = 0;
  // schedule name
  std::string schedule_name = "";
  // array by three (ramp in celsius p/h, target temp in celsius, hold in minutes)
  std::vector<std::array<int, 3>> schedule;
  // index of the current_step in the schedule
  int current_step = 0;
  // remaining hold in seconds, negative means no hold
  int remaining_hold = -1;
};

class RequestHandler : public Trigger<AsyncWebServerRequest &, AsyncResponseStream &> {
 public:
  explicit RequestHandler(KilnApi *parent);
};

}  // namespace kiln_api
}  // namespace esphome