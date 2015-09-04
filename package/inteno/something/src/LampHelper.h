#ifndef LAMP_HELPER_H
#define LAMP_HELPER_H

#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/MsgArg.h>

using namespace ajn;
using namespace std;

class LampHelper {

private:
    ProxyBusObject remotObj;
    uint32_t mapRanges(float x, float inLow, float inHigh, float outLow, float outHigh);

public:
    LampHelper(ProxyBusObject object):remotObj(object){};
    QStatus setOnOff(const bool& onoff);
    QStatus getInfo(uint32_t& brightness, uint32_t& colortemp);
    QStatus setBrightness(const uint32_t& brightness);
    QStatus setColorTemp(const uint32_t& colortemp);
    QStatus getOnOff(bool& onoff);
};


#endif //WIFI_HELPER_H
