#include <qcc/Debug.h>
#include <qcc/Log.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/AboutIconObj.h>

#include <signal.h>
#include <stdio.h>

#include "wifi.h"

extern "C" {
	#include "common.h"
}

#define ArraySize(a)  (sizeof(a) / sizeof(a[0]))

using namespace qcc;
using namespace ajn;

static struct uci_package *uci_wireless;

static const char* wirelessInterfaceXML =
	"<node>"
		"<interface name='org.allseen.WiFi'>"
			"<property name='Version' type='i' access='read'/>"
			"<property name='Enable' type='i' access='readwrite'/>"
			"<property name='Ssid' type='s' access='readwrite'/>"
			"<property name='Key' type='s' access='readwrite'/>"
			"<property name='Channel2g' type='i' access='readwrite'/>"
			"<property name='Channel5g' type='i' access='readwrite'/>"
			"<method name='GetChannels'>"
				"<arg name='freq' type='i' direction='in'/>"
				"<arg name='channels' type='ai' direction='out'/>"
			"</method>"
		"</interface>"
		"<interface name='org.allseen.WPS'>"
			"<property name='Version' type='i' access='read'/>"
			"<property name='Enable' type='i' access='readwrite'/>"
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
				"<arg name='clients' type='a(iss)' direction='out'/>"
			"</method>"
		"</interface>"
	"</node>";

static SessionPort ASSIGNED_SESSION_PORT = 900;

class MySessionPortListener : public SessionPortListener {
    bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
    {
        if (sessionPort != ASSIGNED_SESSION_PORT) {
            printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
            return false;
        }

//        cout << "Accepting JoinSessionRequest from " << joiner << " (opts.proximity= " << opts.proximity
//                << ", opts.traffic=" << opts.traffic << ", opts.transports=" << opts.transports << ")." << endl;
        return true;
    }
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
    {
        printf("Session Joined SessionId = %u\n", id);
    }
};

class WirelessBusObject : public BusObject {
  public:
    WirelessBusObject(BusAttachment& bus, const char* path);

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    QStatus Set(const char* ifcName, const char* propName, MsgArg& val);
    QStatus SendWpsSignal(int respcode);

    void GetChannels(const InterfaceDescription::Member* member, Message& msg);
    void WpsPushButton(const InterfaceDescription::Member* member, Message& msg);
    void GetClients(const InterfaceDescription::Member* member, Message& msg);

    void setChannel(int freq);
    void setSsid();
    void setKey();
    void changeWifiStatus(void);
    void changeWpsStatus(void);
    void wpsPushButton(int status);

    String ssid, encryption, key, radio2g, radio5g;
    int32_t enableWifi, enableWps;
    int32_t channel2g, channel5g;
    SessionId id;
    bool stop;
    Mutex lock;
};

WirelessBusObject::WirelessBusObject(BusAttachment& bus, const char* path):BusObject(path) {
        const InterfaceDescription* wireless_iface = bus.GetInterface("org.allseen.WiFi");
        if (wireless_iface == NULL) {
            printf("The interfaceDescription pointer for org.allseen.WiFi was NULL when it should not have been.\n");
            return;
        }
        AddInterface(*wireless_iface, ANNOUNCED);

        const InterfaceDescription* wps_iface = bus.GetInterface("org.allseen.WPS");
        if (wps_iface == NULL) {
            printf("The interfaceDescription pointer for org.allseen.WPS was NULL when it should not have been.\n");
            return;
        }
        AddInterface(*wps_iface, ANNOUNCED);

        const InterfaceDescription* client_iface = bus.GetInterface("org.allseen.Clients");
        if (client_iface == NULL) {
            printf("The interfaceDescription pointer for org.allseen.Clients was NULL when it should not have been.\n");
            return;
        }
        AddInterface(*client_iface, ANNOUNCED);

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { wireless_iface->GetMember("GetChannels"), static_cast<MessageReceiver::MethodHandler>(&WirelessBusObject::GetChannels) },
	    { wps_iface->GetMember("WpsPushButton"), static_cast<MessageReceiver::MethodHandler>(&WirelessBusObject::WpsPushButton) },
	    { client_iface->GetMember("GetClients"), static_cast<MessageReceiver::MethodHandler>(&WirelessBusObject::GetClients) }
        };
        AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
}

void WirelessBusObject::GetChannels(const InterfaceDescription::Member* member, Message& msg) {
        printf("GetChannels method called: %d\n", msg->GetArg(0)->v_int32);
	int freq = msg->GetArg(0)->v_int32;
	int32_t chn[24];
	String wl;
	string channels;
	char *token;
	char chbuf[128];
	int i = 0;

	(freq == 5)?(wl = radio5g):(wl = radio2g);
	channels = strCmd("wlctl -i %s channels", wl.c_str());
	snprintf(chbuf, 128, "%s", channels.c_str());

	token = strtok(chbuf, " ");
	while (token != NULL)
	{
		chn[i] = atoi(token);
		token = strtok (NULL, " ");
		i++;
	}

	MsgArg arg[1];
	arg[0].Set("ai", ArraySize(chn), chn);
        QStatus status = MethodReply(msg, arg, 1);
        if (status != ER_OK) {
            printf("Failed to create MethodReply for GetChannels.\n");
        }
}

void WirelessBusObject::GetClients(const InterfaceDescription::Member* member, Message& msg) {
	printf("GetClients method called\n");
	Client clients[128];
	MsgArg cln[128];
	int i = 0;

	get_clients(clients);
	while(strlen(clients[i].macaddr) > 16) {
		cln[i].Set("(iss)", clients[i].conntype, clients[i].hostname, clients[i].macaddr);
		i++;
	}

	MsgArg arg[1];
	arg[0].Set("a(iss)", ArraySize(cln), cln);
        QStatus status = MethodReply(msg, arg, 1);
        if (status != ER_OK) {
            printf("Failed to create MethodReply for GetClients.\n");
        }
}

void WirelessBusObject::WpsPushButton(const InterfaceDescription::Member* member, Message& msg) {
        printf("WpsPushButton method called: %d\n", msg->GetArg(0)->v_int32);
	QStatus status = MethodReply(msg, NULL, 1);
        if (status != ER_OK) {
            printf("Failed to create MethodReply for WpsPushButton.\n");
        }
	wpsPushButton(msg->GetArg(0)->v_int32);
}

QStatus WirelessBusObject::SendWpsSignal(int respcode) {
	const InterfaceDescription* bus_ifc = bus->GetInterface("org.allseen.WPS");
	const InterfaceDescription::Member* wpsResponse = (bus_ifc ? bus_ifc->GetMember("WpsResponse") : NULL);
	uint8_t flags = ALLJOYN_FLAG_SESSIONLESS;

	assert(wpsResponse);

        MsgArg arg("i", respcode);
        QStatus status = Signal(NULL, 0, *wpsResponse, &arg, 1, 0, flags);


        if (status != ER_OK) {
            printf("Failed to create SendWpsSignal.\n");
        }
	return status;
}

QStatus WirelessBusObject::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    if (strcmp(ifcName, "org.allseen.WiFi") == 0) {
        lock.Lock();
        if (strcmp(propName, "Enable") == 0) {
            val.Set("i", enableWifi);
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%d) at %s\n", propName, enableWifi, GetPath());
        } else if (strcmp(propName, "Ssid") == 0) {
            val.Set("s", ssid.c_str());
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%s) at %s\n", propName, ssid.c_str(), GetPath());
        } else if (strcmp(propName, "Key") == 0) {
            val.Set("s", key.c_str());
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%s) at %s\n", propName, key.c_str(), GetPath());
        } else if (strcmp(propName, "Channel2g") == 0) {
            val.Set("i", channel2g);
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%d) at %s\n", propName, channel2g, GetPath());
        } else if (strcmp(propName, "Channel5g") == 0) {
            val.Set("i", channel5g);
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%d) at %s\n", propName, channel5g, GetPath());
        }
        lock.Unlock();
    } else if (strcmp(ifcName, "org.allseen.WPS") == 0) {
        lock.Lock();
        if (strcmp(propName, "Enable") == 0) {
            val.Set("i", enableWps);
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%d) at %s\n", propName, enableWps, GetPath());
        }
        lock.Unlock();
    }
    return status;
}

QStatus WirelessBusObject::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    if (strcmp(ifcName, "org.allseen.WiFi") == 0) {
        lock.Lock();
        if (strcmp(propName, "Enable") == 0) {
            val.Get("i", &enableWifi);
            EmitPropChanged(ifcName, propName, val, id);
            status = ER_OK;
            QCC_SyncPrintf("Set property %s (%d) at %s\n", propName, enableWifi, GetPath());
	    changeWifiStatus();
        } else if (strcmp(propName, "Ssid") == 0) {
            const char* s;
            val.Get("s", &s);
            ssid = s;
            EmitPropChanged(ifcName, propName, val, id);
            status = ER_OK;
            QCC_SyncPrintf("Set property %s (%s) at %s\n", propName, ssid.c_str(), GetPath());
	    setSsid();
        } else if (strcmp(propName, "Key") == 0) {
            const char* s;
            val.Get("s", &s);
            key = s;
            EmitPropChanged(ifcName, propName, val, id);
            status = ER_OK;
            QCC_SyncPrintf("Set property %s (%s) at %s\n", propName, key.c_str(), GetPath());
	    setKey();
        } else if (strcmp(propName, "Channel2g") == 0) {
            val.Get("i", &channel2g);
            EmitPropChanged(ifcName, propName, val, id);
            status = ER_OK;
            QCC_SyncPrintf("Set property %s (%d) at %s\n", propName, channel2g, GetPath());
	    setChannel(2);
        } else if (strcmp(propName, "Channel5g") == 0) {
            val.Get("i", &channel5g);
            EmitPropChanged(ifcName, propName, val, id);
            status = ER_OK;
            QCC_SyncPrintf("Set property %s (%d) at %s\n", propName, channel5g, GetPath());
	    setChannel(5);
        }
        lock.Unlock();
    } else if (strcmp(ifcName, "org.allseen.WPS") == 0) {
        lock.Lock();
        if (strcmp(propName, "Enable") == 0) {
            val.Get("i", &enableWps);
            EmitPropChanged(ifcName, propName, val, id);
            status = ER_OK;
            QCC_SyncPrintf("Set property %s (%d) at %s\n", propName, enableWps, GetPath());
	    changeWpsStatus();
        }
        lock.Unlock();
    }
    return status;
}

void WirelessBusObject::setChannel(int freq) {
	String wl;
	int ch;
	char channel[3];

	if (freq == 5) {
		wl = radio5g;
		ch = channel5g;
	} else {
		wl = radio2g;
		ch = channel2g;
	}

	snprintf(channel, 3, "%d", ch);

	uciSet("wireless", wl.c_str(), "channel", channel);
	uciCommit("wireless");
	runCmd("wlctl -i %s down", wl.c_str());
	runCmd("wlctl -i %s channel %d", wl.c_str(), ch);
	runCmd("wlctl -i %s up", wl.c_str());
}

void WirelessBusObject::setSsid() {
	struct uci_element *e;
	struct uci_section *s;

	uci_foreach_element(&uci_wireless->sections, e) {
		s = uci_to_section(e);
		if (!strcmp(s->type, "wifi-iface")) {
			uset(s, "ssid", ssid.c_str());
		}
	}
	ucommit(s);

	runCmd("sed -i \"s/_ssid=.*/_ssid=%s/g\" /etc/config/broadcom", ssid.c_str());
	runCmd("killall -9 nas; nas");
	runCmd("killall -SIGTERM wps_monitor; wps_monitor &");
}

void WirelessBusObject::setKey() {
	struct uci_element *e;
	struct uci_section *s;

	uci_foreach_element(&uci_wireless->sections, e) {
		s = uci_to_section(e);
		if (!strcmp(s->type, "wifi-iface")) {
			uset(s, "key", key.c_str());
		}
	}
	ucommit(s);

	runCmd("sed -i \"s/_wpa_psk=.*/_wpa_psk=%s/g\" /etc/config/broadcom", key.c_str());
	runCmd("killall -9 nas; nas");
	runCmd("killall -SIGTERM wps_monitor; wps_monitor &");
}

void WirelessBusObject::changeWifiStatus() {
	struct uci_element *e;
	struct uci_section *s;

	uci_foreach_element(&uci_wireless->sections, e) {
		s = uci_to_section(e);
		if (!strcmp(s->type, "wifi-iface")) {
			uset(s, "disabled", enableWifi?"0":"1");
		}
	}
	ucommit(s);

	string vifs;
	char vifsbuf[32];
	char *token;

	vifs = strCmd("nvram show | grep vifs | cut -d'=' -f2 | tr '\n' ' '");
	snprintf(vifsbuf, 32, "%s", vifs.c_str());

	token = strtok(vifsbuf, " ");
	while (token != NULL)
	{
		runCmd("wifi %s %s", enableWifi?"enable":"disable", token);
		token = strtok (NULL, " ");
	}
}

void WirelessBusObject::changeWpsStatus() {
	struct uci_element *e;
	struct uci_section *s;

	char wps[2];
	snprintf(wps, 2, "%d", enableWps);

	uci_foreach_element(&uci_wireless->sections, e) {
		s = uci_to_section(e);
		if (!strcmp(s->type, "wifi-iface")) {
			uset(s, "wps_pbc", wps);
		}
	}
	ucommit(s);

	if (enableWps) {
		runCmd("sed -i \"s/wps_mode 'disabled'/wps_mode 'enabled'/g\" /etc/config/broadcom");
		if (strCmd("pidof wps_monitor") == "")
			runCmd("wps_monitor &");
	} else {
		runCmd("sed -i \"s/wps_mode 'enabled'/wps_mode 'disabled'/g\" /etc/config/broadcom");
		runCmd("killall -SIGTERM wps_monitor 2>/dev/null");
	}
}

void WirelessBusObject::wpsPushButton(int status) {
	if (status) {
		if (strCmd("pidof wps_monitor") == "") {
			runCmd("wps_monitor &");
			usleep(100000);
		}
		runCmd("killall -SIGUSR2 wps_monitor");
	} else
		runCmd("killall -SIGTERM wps_monitor");
}


WirelessBusObject *MyBus;

void wps_event(const char *key, const char *val)
{
	int respcode = 2;
	if(!strcmp(val, "timeout"))
		respcode = 0;
	else if (!strcmp(key, "sta"))
		respcode = 1;

	MyBus->SendWpsSignal(respcode);
}

static void
populateWireless()
{
	struct uci_element *e;
	struct uci_section *s;
	String channel, chanspec;

	if(!(uci_wireless = init_package("wireless")))
		return;

	MyBus->enableWps = 0;
	MyBus->channel2g = 0;
	MyBus->channel5g = -1;

	uci_foreach_element(&uci_wireless->sections, e) {
		s = uci_to_section(e);
		if (!strcmp(s->type, "wifi-device")) {
			if(!strcmp(ugets(s, "band"), "a")) {
				MyBus->radio5g = s->e.name;
				chanspec = ugets(s, "channel");
				channel = chanspec.substr(0, chanspec.find('/'));
				if (!strcmp(channel.c_str(), "auto"))
					MyBus->channel5g = 0;
				else
					MyBus->channel5g = ugeti(s, "channel");
			} else {
				MyBus->radio2g = s->e.name;
				chanspec = ugets(s, "channel");
				channel = chanspec.substr(0, chanspec.find('/'));
				if (channel == "auto")
					MyBus->channel2g = 0;
				else
					MyBus->channel2g = ugeti(s, "channel");
			}
		} else if (!strcmp(s->type, "wifi-iface")) {
			MyBus->ssid = ugets(s, "ssid");
			MyBus->encryption = ugets(s, "encryption");
			MyBus->key = ugets(s, "key");
			if (ugeti(s, "wps_pbc") == 1)
				MyBus->enableWps = 1;
		}
	}
}

int main(int argc, char** argv)
{
    QStatus status;
    String advName = "org.alljoyn.alljoyn_wireless";

    BusAttachment bus("Wireless", true);

    status = bus.Start();
    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("FAILED to start BusAttachment (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    status = bus.Connect();
    if (ER_OK == status) {
        printf("BusAttachment connect succeeded. BusAttachment Unique name is %s\n", bus.GetUniqueName().c_str());
    } else {
        printf("FAILED to connect to router node (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    status = bus.CreateInterfacesFromXml(wirelessInterfaceXML);

    /* Request well-known name */
    status = bus.RequestName(advName.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (ER_OK == status) {
        printf("RequestName('%s') succeeded.\n", advName.c_str());
    } else {
        printf("RequestName('%s') failed (status=%s).\n", advName.c_str(), QCC_StatusText(status));
    }
    /*-------------------------------*/

    WirelessBusObject wirelessBusObject(bus, "/org/alljoyn/wireless");

    MyBus = &wirelessBusObject;

    populateWireless();

    bus.RegisterBusObject(wirelessBusObject);

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort sp = ASSIGNED_SESSION_PORT;
    MySessionPortListener sessionPortListener;
    status = bus.BindSessionPort(sp, opts, sessionPortListener);
    if (ER_OK == status) {
        printf("BindSessionPort succeeded.\n");
    } else {
        printf("BindSessionPort failed (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    /* Advertise well-known name */
    status = bus.AdvertiseName(advName.c_str(), TRANSPORT_ANY);
    if (ER_OK == status) {
        printf("Advertisement of the service name '%s' succeeded.\n", advName.c_str());
    } else {
        printf("Failed to advertise name '%s' (%s).\n", advName.c_str(), QCC_StatusText(status));
    }
    /*-------------------------------*/

//    // Setup the about data
//    AboutData aboutData("en");

//    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
//    status = aboutData.SetAppId(appId, 16);
//    status = aboutData.SetDeviceName(strCmd("db get hw.board.hardware").c_str());
//    status = aboutData.SetDeviceId(strCmd("db get hw.board.BaseMacAddr").c_str());
//    status = aboutData.SetAppName("WiFi Manager");
//    status = aboutData.SetManufacturer("Inteno");
//    status = aboutData.SetModelNumber(strCmd("db get hw.board.routerModel | cut -d'-' -f 2").c_str());
//    status = aboutData.SetDescription("Manage WiFi");
//    status = aboutData.SetDateOfManufacture(strCmd("db get hw.board.iopVersion | awk -F'[-,_]' '{print$4}'").c_str());
//    status = aboutData.SetSoftwareVersion(strCmd("db get hw.board.iopVersion | awk -F'[-,_]' '{print$3}'").c_str());
//    status = aboutData.SetHardwareVersion(strCmd("db get hw.board.hardwareVersion").c_str());
//    status = aboutData.SetSupportUrl("http://www.iopsys.eu");
//    if (!aboutData.IsValid()) {
//        printf("failed to setup about data.\n");
//    }

//    AboutIcon icon;
//    status = icon.SetUrl("wp-content/uploads/2014/09/iopsys_logo_CMYK_invert_circle.png", "http://www.iopsys.eu");
//    if (ER_OK != status) {
//        printf("Failed to setup the AboutIcon.\n");
//    }
//    AboutIconObj aboutIconObj(bus, icon);

//    // Announce about signal
//    AboutObj aboutObj(bus, BusObject::ANNOUNCED);
//    status = aboutObj.Announce(ASSIGNED_SESSION_PORT, aboutData);
//    if (ER_OK == status) {
//        printf("AboutObj Announce Succeeded.\n");
//    } else {
//        printf("AboutObj Announce failed (%s)\n", QCC_StatusText(status));
//    }

    ubus_listener();

    return 0;
}
