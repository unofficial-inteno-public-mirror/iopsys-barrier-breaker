/*
 * ami_connection.h
 *
 * Manage connections to AMI.
 *
 * In order to become "ready" we must:
 * - send login, get login successful response
 * - send sip reload and receive CHANNELRELOAD event for SIP (event)
 * - send show brcm module command and receive MODULESHOW (response)
 * - send show brcm ports command and receive BRCMPORTSHOW (response)
 */

#define MESSAGE_FRAME "\r\n\r\n"
#define MESSAGE_FRAME_LOGIN "\r\n"

#define AMI_BUFLEN 512

typedef enum ami_message {
	UNKNOWN_MESSAGE,
	LOGIN_MESSAGE,
	EVENT_MESSAGE,
	RESPONSE_MESSAGE
} ami_message;

typedef struct {
	enum {
		BRCM_UNKNOWN_EVENT,
		BRCM_STATUS_EVENT,
		BRCM_STATE_EVENT,
		BRCM_MODULE_EVENT
	} type;
	struct {
		int line_id;
		int off_hook;
	} status;
	struct {
		int line_id;
		int subchannel_id;
		char* state;
	} state;
	int module_loaded;
} brcm_event;

typedef struct {
	char* account_name;
	enum {
		REGISTRY_UNKNOWN_EVENT,
		REGISTRY_REQUEST_SENT_EVENT,
		REGISTRY_UNREGISTERED_EVENT,
		REGISTRY_REGISTERED_EVENT
	} status;
} registry_event;

typedef struct {
	enum {
		CHANNELRELOAD_UNKNOWN_EVENT,
		CHANNELRELOAD_SIP_EVENT
	} channel_type;
} channel_reload_event;

typedef struct {
	char* channel;
	char* variable;
	char* value;
} varset_event;

typedef enum ami_event_type {
	LOGIN,
	REGISTRY,
	BRCM,
	CHANNELRELOAD,
	FULLYBOOTED,
	VARSET,
	UNKNOWN_EVENT,
} ami_event_type;

typedef struct ami_event {
	ami_event_type type;
	brcm_event* brcm_event;
	registry_event* registry_event;
	channel_reload_event* channel_reload_event;
	varset_event* varset_event;
} ami_event;

typedef struct ami_connection ami_connection;
typedef void (*ami_event_cb) (ami_connection* con, ami_event event);
typedef void (*ami_response_cb) (ami_connection* con, char* buf);
/*
 * Holds data related to an AMI connection
 */
struct ami_connection {
	int connected; //ami_connection is connected to asterisk
	int sd;
	char* message_frame;
	char left_over[AMI_BUFLEN * 2 + 1];
	ami_event_cb event_callback;
	ami_response_cb response_callback;
};

ami_connection* ami_init(ami_event_cb on_event);
void ami_connect(ami_connection* con, const char* hostname, const char* portno);
void ami_disconnect(ami_connection* con);
void ami_free(ami_connection* con);

/*
 * Call when new data is available
 */
void ami_handle_data(ami_connection* con);

/*
 * Actions
 */
int ami_send_sip_reload(ami_connection* con, ami_response_cb on_response);
int ami_send_login(ami_connection* con, char*username, char* password, ami_response_cb on_response);
int ami_send_brcm_module_show(ami_connection* con, ami_response_cb on_response);
int ami_send_brcm_dialtone_settings(ami_connection* con, const int line_id, const char *dialtone_state, ami_response_cb on_response);
int ami_send_brcm_ports_show(ami_connection* con, ami_response_cb on_response);
