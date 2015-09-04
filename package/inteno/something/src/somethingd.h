#include <libubox/blobmsg.h>
#include <ubusmsg.h>
#include <libubus.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static struct ubus_context *ctx = NULL;
static const char *ubus_path;
static struct blob_buf bb;
struct RetDevice{
    const char* name;
    char* deviceId;
};

const int MAX_DEVICES = 50;
const int DEVICE_LENGTH = 50;
const int MAX_STRING_LENGTH = 100;

bool searchForInterface(char* interface); 
struct RetDevice* getAllDevices(int* numDev);
bool connectToDevice(char* id);
bool setLampValues(int brightness, int colortemp);
bool getLampValues(uint32_t* brightness, uint32_t* colortemp, bool* onoff);
bool setWifiValues(char* SSID, char* pass, int channel2G, int channel5G);
bool getWifiValues(char* SSID, char* pass, uint32_t* channel2G, uint32_t* channel5G);
bool setLampOnOff(bool onoff);
int* getChannels(int channel, size_t* size);
void printh(char* string);
