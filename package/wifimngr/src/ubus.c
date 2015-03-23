#include <unistd.h>

#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <libubox/uloop.h>
#include <libubus.h>

#include "common.h"

static struct ubus_context *ctx;
static struct ubus_event_handler event_listener;
static struct blob_buf b;

const char*
json_parse_and_get(const char *str, char *var)
{
	json_object *obj;
	char result[128];
	
	obj = json_tokener_parse(str);
	if (is_error(obj) || json_object_get_type(obj) != json_type_object) {
		return NULL;
	}

	memset(result, 0, 24);

	json_object_object_foreach(obj, key, val) {
		if(!strcmp(key, var)) {
			switch (json_object_get_type(val)) {
			case json_type_object:
				break;
			case json_type_array:
				break;
			case json_type_string:
				sprintf(result, "%s", json_object_get_string(val));
				break;
			case json_type_boolean:
				sprintf(result, "%d", json_object_get_boolean(val));
				break;
			case json_type_int:
				sprintf(result, "%d", json_object_get_int(val));
				break;
			default:
				break;
			}
		}
	}
	if (strlen(result))
		return strdup(result);
	else
		return NULL;
}

void
get_clients(Client *client)
{
	FILE *leases;
	char line[256];
	char leaseno[24];
	char macaddr[24];
	char ipaddr[24];
	char hostname[128];
	char hwaddr[24];
	FILE *in;
	char cmnd[256];
	char result[2048];
	int cno = 0;

	if(!(leases = fopen("/var/dhcp.leases", "r")))
		return;

	while(fgets(line, sizeof(line), leases) != NULL)
	{
		removeNewline(line);
		if (sscanf(line, "%s %s %s %s %s", leaseno, macaddr, ipaddr, hostname, hwaddr) != 5)
			break;

		sprintf(cmnd, "ubus -S call router host \"{ \\\"ipaddr\\\" : \\\"%s\\\" }\"", ipaddr);
		if ((in = popen(cmnd, "r")))
			fgets(result, sizeof(result), in);
		pclose(in);

		if(strlen(result) < 64)
			continue;

		if(!strcmp(json_parse_and_get(result, "wireless"), "1"))
			client[cno].conntype = 2;
		else if(!strcmp(json_parse_and_get(result, "connected"), "1"))
			client[cno].conntype = 1;
		else
			client[cno].conntype = 0;

		strcpy(client[cno].hostname, hostname);
		strcpy(client[cno].macaddr, macaddr);

		cno++;
	}

	fclose(leases);
}

static void
receive_event(struct ubus_context *ctx, struct ubus_event_handler *ev, const char *type, struct blob_attr *msg)
{
	char *str;
	const char *sta;

	str = blobmsg_format_json(msg, true);

	if(!strcmp(type, "wps")) {
		if(sta = json_parse_and_get(str, "status"))
			wps_event("status", sta);
		else if (sta = json_parse_and_get(str, "sta"))
			wps_event("sta", sta);
	}

	free(str);
}

void
ubus_listener()
{
	const char *ubus_socket = NULL;
	int ret;

	uloop_init();

	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return;
	}

	ubus_add_uloop(ctx);

	event_listener.cb = receive_event;
	ret = ubus_register_event_handler(ctx, &event_listener, "wps");
	if (ret)
		fprintf(stderr, "Couldn't register to router events\n");

	uloop_run();

	ubus_free(ctx);
	uloop_done();
}
