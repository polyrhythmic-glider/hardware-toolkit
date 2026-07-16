#include "storage_manager.hpp"

#include <cstdio>
#include <sys/statvfs.h>
#include "board_config.hpp"
#include "esp_log.h"
#include "esp_vfs_fat.h"

namespace neko {
static constexpr char kMountPoint[] = "/sdcard";

esp_err_t StorageManager::mount() {
    if (status_.mounted) return ESP_OK;
    esp_vfs_fat_sdmmc_mount_config_t mount_config{};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 6;
    mount_config.allocation_unit_size = 16 * 1024;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = board::kSdClk;
    slot.cmd = board::kSdCmd;
    slot.d0 = board::kSdD0;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    status_.last_error = esp_vfs_fat_sdmmc_mount(kMountPoint, &host, &slot,
                                                 &mount_config, &card_);
    status_.mounted = status_.last_error == ESP_OK;
    if (status_.mounted) refresh();
    return status_.last_error;
}

esp_err_t StorageManager::refresh() {
    if (!status_.mounted || !card_) return ESP_ERR_INVALID_STATE;
    esp_err_t error = sdmmc_get_status(card_);
    if (error != ESP_OK) {
        status_.last_error = error;
        return error;
    }
    struct statvfs fs{};
    if (statvfs(kMountPoint, &fs) != 0) return ESP_FAIL;
    status_.total_bytes = static_cast<uint64_t>(fs.f_blocks) * fs.f_frsize;
    status_.free_bytes = static_cast<uint64_t>(fs.f_bavail) * fs.f_frsize;
    return ESP_OK;
}

esp_err_t StorageManager::append_log(const char *line) {
    if (!status_.mounted || !line) return ESP_ERR_INVALID_STATE;
    const char *path = "/sdcard/neko.log";
    FILE *file = std::fopen(path, "a");
    if (!file) return ESP_FAIL;
    std::fprintf(file, "%s\n", line);
    std::fclose(file);
    return ESP_OK;
}

esp_err_t StorageManager::unmount() {
    if (!status_.mounted) return ESP_OK;
    esp_err_t error = esp_vfs_fat_sdcard_unmount(kMountPoint, card_);
    card_ = nullptr;
    status_.mounted = false;
    status_.last_error = error;
    return error;
}

}  // namespace neko
