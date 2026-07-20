#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace neko {

class InputManager final {
public:
    esp_err_t init();
private:
    static void task_entry(void *context);
    void run();
    TaskHandle_t task_{nullptr};
};

}  // namespace neko
