#include "firmware_manager.h"

namespace firmware {
    RemoteFirmware::RemoteFirmware(std::string website, FmType type) {

    }

    RemoteFirmware::~RemoteFirmware() {

    }

    bool RemoteFirmware::checkConnection() {
        return false;
    }

    std::string RemoteFirmware::downloadIfNewer(std::string oldVersion, std::string storagePath) {
        return "";
    }

    bool RemoteFirmware::cancelDownload() {
        return false;
    }

    bool RemoteFirmware::isNewer(std::string oldVersion) {
        return false;
    }
}