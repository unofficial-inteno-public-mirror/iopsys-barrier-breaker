#include "SomethingListener.h"
#include "Wifihelper.h"
#include "LampHelper.h"
#include "something.h"



ajn::ProxyBusObject createProxyBusObject(BusAttachment *g_bus, const char* busName, const char* INTERFACE_PATH, const char* INTERFACE_NAME){
    ProxyBusObject remoteObj(*g_bus, busName, INTERFACE_PATH, SomethingListener::SESSION_ID, false);
    const InterfaceDescription* iterface = g_bus->GetInterface(INTERFACE_NAME);
    assert(iterface);
    remoteObj.AddInterface(*iterface);
    return remoteObj;
}


int* getChannels(int channel, size_t* size){
	if(state != CONNECTED_WIFI)return NULL;
	ProxyBusObject pbu = createProxyBusObject(myBus, busName, "/org/alljoyn/wireless", "org.allseen.WiFi");
	Wifihelper helper(pbu);
	Message message(*myBus);
	size_t numArgs = 0;
	int* values = helper.getChannels(message, channel, &numArgs);
	*size = numArgs;
	return values;
}

bool searchForInterface(char* i_face){
    if(strcmp(i_face, "org.allseen.WiFi") == 0)state = SEARCH_WIFI;
    else state = SEARCH_LAMP;
    string interface(i_face);
    if(listener == NULL || myBus == NULL){
		cout << "listerner or myBus is NULL" << endl;	
		return false;
	}
    listener->setInterface(interface);
    //const char* interfaces[1];
    //interfaces[0] = i_face;
    const char* theInterface = strdup(i_face);
    myBus->CancelWhoImplements(theInterface);
    QStatus status = myBus->WhoImplements(theInterface);
    if (ER_OK == status) {
        cout << "something: " << "WhoImplements called for " << theInterface << endl;
        return true;
    } else {
        cout << "something: " << "WhoImplements call FAILED with status " << SomethingListener::QStatusToString(status) << endl;
        return false;
    }
}

bool setLampOnOff(bool onoff){
	if(state != CONNECTED_LAMP || myBus == NULL)return false;
	ProxyBusObject pbu = createProxyBusObject(myBus, busName, "/org/allseen/LSF/Lamp", "org.allseen.LSF.LampState");
	LampHelper helper(pbu);
	QStatus status = helper.setOnOff(onoff);
	if(status != ER_OK)return false;
	return true;
}

bool setLampValues(int brightness, int colortemp){
	if(state != CONNECTED_LAMP || myBus == NULL)return false;
	ProxyBusObject pbu = createProxyBusObject(myBus, busName, "/org/allseen/LSF/Lamp", "org.allseen.LSF.LampState");
	LampHelper helper(pbu);
	QStatus status;
	if(brightness != -1){
		status = helper.setBrightness(brightness);
		if(status != ER_OK)return false;
	}
	if(colortemp != -1 && brightness != -1)usleep(100000);
	if(colortemp != -1){
		status = helper.setColorTemp(colortemp);
		if(status != ER_OK)return false;
	}
	return true;
}

bool getLampValues(uint32_t* brightness, uint32_t* colortemp, bool* onoff){
	if(state != CONNECTED_LAMP || myBus == NULL)return false;
	ProxyBusObject pbu = createProxyBusObject(myBus, busName, "/org/allseen/LSF/Lamp", "org.allseen.LSF.LampState");
	LampHelper helper(pbu);
	uint32_t cb,ct;
	bool oo;
	QStatus status = helper.getInfo(cb, ct);
	if(status != ER_OK)return false;
	*brightness = cb;
	*colortemp = ct;
	usleep(10000);
	status = helper.getOnOff(oo);
	if(status != ER_OK)return false;
	*onoff = oo;
	return true;
}

bool setWifiValues(char* SSID, char* pass, int channel2G, int channel5G){
	if(state != CONNECTED_WIFI || myBus == NULL)return false;
	ProxyBusObject pbu = createProxyBusObject(myBus, busName, "/org/alljoyn/wireless", "org.allseen.WiFi");
	Wifihelper helper(pbu);
	QStatus status;
	if(strcmp(SSID, "null")){
		status = helper.changeSSID((const char*)SSID);
		if(status != ER_OK)return false;
	}
	if(strcmp(pass, "null")){
		status = helper.changePass((const char*)pass);
		if(status != ER_OK)return false;
	}
	if(channel2G != -1){
		status = helper.setChannel2G((const int)channel2G);
		if(status != ER_OK)return false;
	}
	if(channel5G != -1){
		status = helper.setChannel5G((const int)channel5G);
		if(status != ER_OK)return false;
	}
	return true;
}

bool getWifiValues(char* SSID, char* pass, uint32_t* channel2G, uint32_t* channel5G){
	if(state != CONNECTED_WIFI || myBus == NULL)return false;
	ProxyBusObject pbu = createProxyBusObject(myBus, busName, "/org/alljoyn/wireless", "org.allseen.WiFi");
	Wifihelper helper(pbu);
	string ss, pas;
	int ch2g, ch5g;
	QStatus status = helper.getInfo(ss, pas, &ch2g, &ch5g);
	if(status != ER_OK)return false;
	strcpy(SSID, strdup(ss.c_str()));
	strcpy(pass, strdup(pas.c_str()));
	*channel2G = ch2g;
	*channel5G = ch5g;
	return true;
}

void printh(char* string){
	cout << string << endl;
}


struct RetDevice* getAllDevices(int* numDev){
	if(state == NOT_CONNECTED){
		cout << "need to call whoImplements first " << endl;
		return NULL;
	}
    return listener->listAllDevices(numDev);
}

bool connectToDevice(char* id){
	if(state == NOT_CONNECTED)return false;
	if(myBus->IsConnected())myBus->LeaveSession(SomethingListener::SESSION_ID);//in case of multiple connections in a row
    int i = listener->connectToInstance(id);
    if(i >= 0){
    	if(state == SEARCH_WIFI)state = CONNECTED_WIFI;
		else if(state == SEARCH_LAMP)state = CONNECTED_LAMP;
        busName = strdup(listener->getDeviceBusName(i));
        return true;
    }
    return false;
}


int main(int argc, char** argv){
    QCC_UNUSED(argc);
    QCC_UNUSED(argv);


    QStatus status;

    BusAttachment g_bus("alljoyn-ubus-daemon", true);
    
    myBus = &g_bus;
    
    status = g_bus.CreateInterfacesFromXml(lampInterfaceXML);
    if(ER_OK != status)return 1;
    
    status = g_bus.CreateInterfacesFromXml(wirelessInterfaceXML);
    if(ER_OK != status)return 1;
    
    status = g_bus.Start();
    if (ER_OK == status) {
        cout << "something: " << "BusAttachment started." << endl;
    } else {
        cout << "something: " << "FAILED to start BusAttachment " << SomethingListener::QStatusToString(status) << endl;
        exit(1);
    }

    status = g_bus.Connect();
    if (ER_OK == status) {
        cout << "something: " << "BusAttachment connect succeeded." << endl;
    } else {
        cout << "something: " << "FAILED to connect to router node " << SomethingListener::QStatusToString(status) << endl;
        exit(1);
    }
    
    SomethingListener busListener(&g_bus);
    listener = &busListener;
    g_bus.RegisterAboutListener(busListener);

	
	initUbus();
   
    
    g_bus.UnregisterAboutListener(busListener);

    return 0;
}
