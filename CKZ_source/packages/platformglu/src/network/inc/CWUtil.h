// CWUtil.h - web helpers.
#ifndef CWUTIL_H
#define CWUTIL_H

#include "CStrChar.h"

namespace com { namespace glu { namespace platform { namespace network {

class CWUtil {
public:
    /**
     * Split "scheme://host[:port]/path" into host, port and path. Without a port the default
     * is 80, or 443 with `secure` = 1 for "https". Returns 0 on success, 1 for a NULL url.
     */
    static int ParseUrl(const char* url, components::CStrChar& host, unsigned short& port,
                        components::CStrChar& path, unsigned char& secure);
};

} } } }

#endif
