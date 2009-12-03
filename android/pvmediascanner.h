#ifndef PV_MEDIA_SCANNER_H_

#define PV_MEDIA_SCANNER_H_

#include <media/mediascanner.h>

namespace android {

struct PVMediaScanner : public MediaScanner
{
public:
    PVMediaScanner();
    virtual ~PVMediaScanner();

    virtual status_t processFile(
            const char *path, const char *mimeType,
            MediaScannerClient &client);

    // extracts album art as a block of data
    virtual char *extractAlbumArt(int fd);

    static void uninitializeForThread();

private:
    void initializeForThread();

    PVMediaScanner(const PVMediaScanner &);
    PVMediaScanner &operator=(const PVMediaScanner &);
};

}  // namespace android

#endif  // PV_MEDIA_SCANNER_H_
