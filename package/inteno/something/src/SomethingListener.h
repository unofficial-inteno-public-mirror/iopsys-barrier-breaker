#ifndef SOMETHING_LISTENER_H
#define SOMETHING_LISTENER_H


#include <alljoyn/BusAttachment.h>
#include <iomanip>
#include <sstream>

const int MAX_DEVICES = 50;

using namespace ajn;
using namespace qcc;
using namespace std;

struct Device{
	const char* name;
	char* deviceId;
	const char* busName;
	SessionPort port;
};

struct RetDevice{
    const char* name;
    char* deviceId;
};




class SomethingListener : public AboutListener, public SessionListener {
public:
    static SessionId SESSION_ID;

public:
    SomethingListener(BusAttachment *busAttachment);
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArgument, const MsgArg& aboutDataArgument);
    static string QStatusToString(QStatus status);
    struct RetDevice* listAllDevices(int *numDev);
    int connectToInstance(char* id);
    void setDeviceName(char* name);
    void removeDevice(int index);
    const char * getDeviceName(int index);
    void addDevice(Device device);
    const char * getDeviceBusName(int Device);
    SessionPort getSessionPort(int index);
    void setInterface(string interface);
    string getInterface(){return interface;};
    char* getDeviceId(int index);
    int getNumDevices(){return numDevices;};
    
    
private:
    BusAttachment *g_bus;
    int numDevices;
    Device devices[MAX_DEVICES];
    string interface;
};



#endif //SOMEHTING_LISTENER_H
