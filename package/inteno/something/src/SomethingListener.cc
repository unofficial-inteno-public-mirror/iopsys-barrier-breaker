#include "SomethingListener.h"


SessionId SomethingListener::SESSION_ID = 50;


string SomethingListener::QStatusToString(QStatus status){
    return QCC_StatusText(status);
}

SomethingListener::SomethingListener(BusAttachment *busAttachment){
    g_bus = busAttachment;
    numDevices = 0;
    interface = "";
}

void SomethingListener::setInterface(string newInterface){
    numDevices = 0;
    interface = newInterface;
    cout << interface << endl;
}

void SomethingListener::Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg){
    cout << "something: interface = " << interface << endl;
    AboutObjectDescription aod(objectDescriptionArg);
    if(aod.HasInterface(interface.c_str())){
        AboutData data(aboutDataArg);
        char* string;
        Device device;
        QStatus status = data.GetDeviceId(&string);
        if(status != ER_OK){
            cout << "error getting deviceid" << endl;
            return;
        }else{
            device.deviceId = string;
        }
        status = data.GetAppName(&string, NULL);
        if(status != ER_OK){
            cout << "error getting deviceid" << endl;
            return;
        }else{
            device.name = string;
        }
        device.busName = busName;
        device.port = port;
        addDevice(device);
    }
}


struct RetDevice* SomethingListener::listAllDevices(int* numDev){
    if(numDevices == 0){
    	cout << "no devices in list" << endl;
    	*numDev = -1;
    	return NULL;
    	}
    int i;
    RetDevice* devs = (RetDevice*)malloc(numDevices*sizeof(RetDevice));
    if(devs == NULL)return devs;
    for(i = 0; i < numDevices; i++){
        devs[i].name = strdup(getDeviceName(i));
        devs[i].deviceId = strdup(getDeviceId(i));
        cout << "something: Instance nr:" << i << " " << devs[i].name << endl;
    }
	*numDev = i;
    return devs;
}

int SomethingListener::connectToInstance(char* id){
    int i;
    for(i = 0; i < numDevices; i++){
        if(strcmp(id, getDeviceId(i)) == 0){
            g_bus->EnableConcurrentCallbacks();
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status = g_bus->JoinSession(getDeviceBusName(i), getSessionPort(i), this, SESSION_ID, opts);
            if(ER_OK == status){
                cout << "something: session joined" << endl;
                return i;
            }else{
                cout << "something: error joining session" << QStatusToString(status) << endl;
                return -1;
            }
        }
    }
    return -1;   
}
char* SomethingListener::getDeviceId(int index){
    if(index < 0 || index >= numDevices) return "invalid index";
    return strdup(devices[index].deviceId);
}

const char * SomethingListener::getDeviceName(int device) {
    if(device < 0 || device >= numDevices) return "invalid index";
    return strdup(devices[device].name);
}

const char * SomethingListener::getDeviceBusName(int index) {
    if(index < 0 || index >= numDevices) return "invalid index";
    return strdup(devices[index].busName);
}

SessionPort SomethingListener::getSessionPort(int index){
    if(index < 0 || index >= numDevices)return 0;
    return devices[index].port;
}

void SomethingListener::addDevice(Device device){
    char* deviceId = strdup(device.deviceId);
    for(int i = 0; i < numDevices; i++){
        if(strcmp(devices[i].deviceId, deviceId) == 0)return;
    }
    devices[numDevices].name = strdup(device.name);
    devices[numDevices].busName = strdup(device.busName);
    devices[numDevices].deviceId = strdup(device.deviceId);
    devices[numDevices].port = device.port;
    numDevices ++;
}

void SomethingListener::removeDevice(int index){
    for(int i = index; i < numDevices-1; i++){
        if(i == MAX_DEVICES){
            numDevices --;
            return;
        }
        devices[i+1] = devices[i];
    }
    numDevices--;
}


