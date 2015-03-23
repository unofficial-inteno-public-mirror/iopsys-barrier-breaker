typedef struct {
	int conntype;
	char hostname[128];
	char macaddr[24];
} Client;

void ubus_listener();
void wps_event(const char *key, const char *val);
void get_clients(Client *client);
void removeNewline(char *buf);
