#ifndef VXMT_CPPSHARE_FIRMWARE_REMOTEFIRMWARE
#define VXMT_CPPSHARE_FIRMWARE_REMOTEFIRMWARE

#include <string.h>

namespace firmware
{
    class RemoteFirmware {
    public:
        enum class FmType {
            MtMonitor,
            MtHub
        };

        RemoteFirmware(std::string website, FmType type);
        ~RemoteFirmware();

        bool isNewer(std::string oldVersion);
        std::string downloadIfNewer(std::string oldVersion, std::string storagePath);
        bool cancelDownload();
        bool checkConnection();
    private:
    }
} // namespace firmware


#endif