#include "app_controller.hpp"

#include "esp_log.h"

extern "C" void app_main(void) {
    static neko::AppController app;
    const esp_err_t error = app.init();
    if (error != ESP_OK) {
        ESP_LOGE("main", "fatal initialization error: %s", esp_err_to_name(error));
        return;
    }
    app.run();
}
