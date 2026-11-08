// PROVISIONAL: native interface owner is not recovered yet.
// Verified symbol: IOSInterface::showMessage(char const*, char const*).
// Restored by b063 in src/iosinterface.mm; preceding PROVISIONAL is historical.
#ifndef IOSINTERFACE_H
#define IOSINTERFACE_H
class IOSInterface {
public:
    static void showMessage(const char* title, const char* message);
};
#endif
