#ifndef SOMETHING_H
#define SOMETHING_H
#include <iostream>
#include <string>


using namespace std;
using namespace ajn;
using namespace qcc;

enum State { NOT_CONNECTED, CONNECTED_WIFI, CONNECTED_LAMP, SEARCH_WIFI, SEARCH_LAMP };


extern "C" void initUbus();
extern "C" {
    bool searchForInterface(char* i_face); 
    struct RetDevice* getAllDevices(int* numDev);
    bool connectToDevice(char* id);
    bool setLampValues(int brightness, int colortemp);
    bool getLampValues(uint32_t* brightness, uint32_t* colortemp, bool* onoff);
    bool setWifiValues(char* SSID, char* pass, int channel2G, int channel5G);
    bool getWifiValues(char* SSID, char* pass, uint32_t* channel2G, uint32_t* channel5G);
    bool setLampOnOff(bool onoff);
    int* getChannels(int channel, size_t* size);
    void printh(char* string);
}

BusAttachment * myBus = NULL;
SomethingListener * listener = NULL;
char * busName = NULL;
State state = NOT_CONNECTED;


static const char* wirelessInterfaceXML =
	"<node>"
		"<interface name='org.allseen.WiFi'>"
			"<property name='Version' type='i' access='read'/>"
			"<property name='Enable' type='i' access='readwrite'/>"
			"<property name='Ssid' type='s' access='readwrite'/>"
			"<property name='Key' type='s' access='readwrite'/>"
			"<property name='EnableWps' type='i' access='readwrite'/>"
			"<property name='Channel2g' type='i' access='readwrite'/>"
			"<property name='Channel5g' type='i' access='readwrite'/>"
			"<method name='ApplyChanges'>"
			"</method>"
			"<method name='CurrentChannel'>"
				"<arg name='freq' type='i' direction='in'/>"
				"<arg name='channel' type='i' direction='out'/>"
			"</method>"
			"<method name='GetChannels'>"
				"<arg name='freq' type='i' direction='in'/>"
				"<arg name='channels' type='ai' direction='out'/>"
			"</method>"
		"</interface>"
		"<interface name='org.allseen.WPS'>"
			"<property name='Version' type='i' access='read'/>"
			"<method name='WpsPushButton'>"
				"<arg name='active' type='i' direction='in'/>"
			"</method>"
			"<signal name='WpsResponse' sessionless='true'>"
				"<arg name='respCode' type='i'/>"
			"</signal>"
		"</interface>"
		"<interface name='org.allseen.Clients'>"
			"<property name='Version' type='i' access='read'/>"
			"<method name='GetClients'>"
				"<arg name='clients' type='a(ssiii)' direction='out'/>"
			"</method>"
		"</interface>"
	"</node>";

static const char* lampInterfaceXML =
	"<node>"
        "<interface name='org.freedesktop.DBus.Properties'>"
            "<method name='Get'>"
                "<arg type='s' direction='in'/>"
                "<arg type='s' direction='in'/>"
                "<arg type='v' direction='out'/>"
            "</method>"
            "<method name='Set'>"
                "<arg type='s' direction='in'/>"
                "<arg type='s' direction='in'/>"
                "<arg type='v' direction='in'/>"
            "</method>"
            "<method name='GetAll'>"
                "<arg type='s' direction='in'/>"
                "<arg type='a{sv}' direction='out'/>"
            "</method>"
        "</interface>"
        "<interface name='org.allseen.LSF.LampService'>"
            "<property name='Version' type='u' access='read'/>"
            "<property name='LampServiceVersion' type='u' access='read'/>"
            "<method name='ClearLampFault'>"
                "<arg name='LampFaultCode' type='u' direction='in'/>"
                "<arg name='LampResponseCode' type='u' direction='out'/>"
                "<arg name='LampFaultCode' type='u' direction='out'/>"
            "</method>"
            "<property name='LampFaults' type='au' access='read'/>"
        "</interface>"
        "<interface name='org.allseen.LSF.LampParameters'>"
            "<property name='Version' type='u' access='read'/>"
            "<property name='Energy_Usage_Milliwatts' type='u' access='read'/>"
            "<property name='Brightness_Lumens' type='u' access='read'/>"
        "</interface>"
        "<interface name='org.allseen.LSF.LampDetails'>"
            "<property name='Version' type='u' access='read'/>"
            "<property name='Make' type='u' access='read'/>"
            "<property name='Model' type='u' access='read'/>"
            "<property name='Type' type='u' access='read'/>"
            "<property name='LampType' type='u' access='read'/>"
            "<property name='LampBaseType' type='u' access='read'/>"
            "<property name='LampBeamAngle' type='u' access='read'/>"
            "<property name='Dimmable' type='b' access='read'/>"
            "<property name='Color' type='b' access='read'/>"
            "<property name='VariableColorTemp' type='b' access='read'/>"
            "<property name='HasEffects' type='b' access='read'/>"
            "<property name='MinVoltage' type='u' access='read'/>"
            "<property name='MaxVoltage' type='u' access='read'/>"
            "<property name='Wattage' type='u' access='read'/>"
            "<property name='IncandescentEquivalent' type='u' access='read'/>"
            "<property name='MaxLumens' type='u' access='read'/>"
            "<property name='MinTemperature' type='u' access='read'/>"
            "<property name='MaxTemperature' type='u' access='read'/>"
            "<property name='ColorRenderingIndex' type='u' access='read'/>"
            "<property name='LampID' type='s' access='read'/>"
        "</interface>"
        "<interface name='org.allseen.LSF.LampState'>"
            "<property name='Version' type='u' access='read'/>"
            "<method name='TransitionLampState'>"
                "<arg name='Timestamp' type='t' direction='in'/>"
                "<arg name='NewState' type='a{sv}' direction='in'/>"
                "<arg name='TransitionPeriod' type='u' direction='in'/>"
                "<arg name='LampResponseCode' type='u' direction='out'/>"
            "</method>"
            "<method name='ApplyPulseEffect'>"
                "<arg name='FromState' type='a{sv}' direction='in'/>"
                "<arg name='ToState' type='a{sv}' direction='in'/>"
                "<arg name='period' type='u' direction='in'/>"
                "<arg name='duration' type='u' direction='in'/>"
                "<arg name='numPulses' type='u' direction='in'/>"
                "<arg name='timestamp' type='t' direction='in'/>"
                "<arg name='LampResponseCode' type='u' direction='out'/>"
            "</method>"
            "<signal name='LampStateChanged'>"
                "<arg name='LampID' type='s'/>"
            "</signal>"
            "<property name='OnOff' type='b' access='readwrite'/>"
            "<property name='Hue' type='u' access='readwrite'/>"
            "<property name='Saturation' type='u' access='readwrite'/>"
            "<property name='ColorTemp' type='u' access='readwrite'/>"
            "<property name='Brightness' type='u' access='readwrite'/>"
        "</interface>"
    "</node>";

#endif //SOMEHTING_H
