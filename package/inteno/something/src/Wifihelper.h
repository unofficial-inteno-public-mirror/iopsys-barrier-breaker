#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/MsgArg.h>

using namespace ajn;
using namespace std;

class Wifihelper {

private:
    ProxyBusObject remotObj;
    QStatus applyChanges();
public:
    Wifihelper(ProxyBusObject object):remotObj(object){};
    QStatus getInfo(string& Ssid, string& key, int* chanel2G, int* chanel5G);
    QStatus changeSSID(const char* SSID);
    QStatus setChannel2G(const int channel);
    QStatus setChannel5G(const int channel);
    QStatus changePass(const char* key);
    int* getChannels(Message message, int channel, size_t* numChannels);
};


#endif //WIFI_HELPER_H
