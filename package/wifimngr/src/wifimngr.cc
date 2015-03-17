#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/AboutIconObj.h>

#include <signal.h>
#include <stdio.h>

#include "wifi.h"

using namespace ajn;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig) {
    s_interrupt = true;
}

static SessionPort ASSIGNED_SESSION_PORT = 900;

class MySessionPortListener : public SessionPortListener {
    bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
    {
        if (sessionPort != ASSIGNED_SESSION_PORT) {
            printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
            return false;
        }

//        std::cout << "Accepting JoinSessionRequest from " << joiner << " (opts.proximity= " << opts.proximity
//                << ", opts.traffic=" << opts.traffic << ", opts.transports=" << opts.transports << ")." << std::endl;
        return true;
    }
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
    {
        printf("Session Joined SessionId = %u\n", id);
    }
};

class WirelessBusObject : public BusObject {
  public:
    WirelessBusObject(BusAttachment& bus, const char* path)
        : BusObject(path) {
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

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { wireless_iface->GetMember("GetChannels"), static_cast<MessageReceiver::MethodHandler>(&WirelessBusObject::GetChannels) },
	    { wps_iface->GetMember("WpsPushButton"), static_cast<MessageReceiver::MethodHandler>(&WirelessBusObject::WpsPushButton) }
        };
        AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    }

    void GetChannels(const InterfaceDescription::Member* member, Message& msg) {
        printf("GetChannels method called: %s", msg->GetArg(0)->v_string.str);
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = MethodReply(msg, arg, 1);
        if (status != ER_OK) {
            printf("Failed to created MethodReply.\n");
        }
    }

    void WpsPushButton(const InterfaceDescription::Member* member, Message& msg) {
        printf("WpsPushButton method called: %s", msg->GetArg(0)->v_string.str);
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = MethodReply(msg, arg, 1);
        if (status != ER_OK) {
            printf("Failed to created MethodReply.\n");
        }
    }
};

/** Main entry point */
int main(int argc, char** argv)
{
    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);

    populateWireless();

    QStatus status;

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

    const char* wirelessInterfaceXML =
	"<node>"
		"<interface name='org.allseen.WiFi'>" 
			"<property name='Version' type='q' access='read'/>"
			"<property name='Enable' type='i' access='readwrite'/>" 
			"<property name='Ssid' type='s' access='readwrite'/>" 
			"<property name='Key' type='s' access='readwrite'/>" 
			"<property name='Channel2g' type='i' access='readwrite'/>" 
			"<property name='Channel5g' type='i' access='readwrite'/>" 
			"<method name='GetChannels'>" 
				"<arg name='freq' type='q' direction='in'/>" 
				"<arg name='channels' type='s' direction='out'/>" 
			"</method>" 
			"<annotation name='org.alljoyn.Bus.Secure' value='true'/>"
		"</interface>"
		"<interface name='org.allseen.WPS'>"
			"<property name='Version' type='q' access='read'/>"
			"<property name='Enable' type='q' access='readwrite'/>"
			"<method name='WpsPushButton'>" 
				"<arg name='active' type='q' direction='in'/>"
			"</method>"  
			"<signal name='WpsResponse' sessionless='true'>" 
				"<arg name='respCode' type='q'/>" 
			"</signal>"
			"<annotation name='org.alljoyn.Bus.Secure' value='true'/>"
		"</interface>"
	"</node>";

    status = bus.CreateInterfacesFromXml(wirelessInterfaceXML);

    WirelessBusObject wirelessBusObject(bus, "/org/alljoyn/wireless");

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

    // Setup the about data
    AboutData aboutData("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = aboutData.SetAppId(appId, 16);
    status = aboutData.SetDeviceName("CG300");
    status = aboutData.SetDeviceId("CG300A-WU21UD");
    status = aboutData.SetAppName("Inteno Wireless");
    status = aboutData.SetManufacturer("Inteno");
    status = aboutData.SetModelNumber("123456");
    status = aboutData.SetDescription("Inteno Wireless Settings");
    status = aboutData.SetDateOfManufacture("2015-03-16");
    status = aboutData.SetSoftwareVersion("3.2.6");
    status = aboutData.SetHardwareVersion("1.2.3");
    status = aboutData.SetSupportUrl("http://www.alljoyn.org");
    if (!aboutData.IsValid()) {
        printf("failed to setup about data.\n");
    }

    AboutIcon icon;
    status = icon.SetUrl("wp-content/uploads/2014/09/iopsys_logo_CMYK_invert_circle.png", "http://www.iopsys.eu");
    if (ER_OK != status) {
        printf("Failed to setup the AboutIcon.\n");
    }
    AboutIconObj aboutIconObj(bus, icon);

    // Announce about signal
    AboutObj aboutObj(bus, BusObject::ANNOUNCED);
    status = aboutObj.Announce(ASSIGNED_SESSION_PORT, aboutData);
    if (ER_OK == status) {
        printf("AboutObj Announce Succeeded.\n");
    } else {
        printf("AboutObj Announce failed (%s)\n", QCC_StatusText(status));
    }

    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
        while (s_interrupt == false) {
            usleep(100000);
        }
    }

    return 0;
}
