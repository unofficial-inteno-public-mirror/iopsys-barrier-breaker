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
	char result[24];
	
	obj = json_tokener_parse(str);
	if (is_error(obj) || json_object_get_type(obj) != json_type_object) {
		return NULL;
	}

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

static void
receive_event(struct ubus_context *ctx, struct ubus_event_handler *ev, const char *type, struct blob_attr *msg)
{
	char *str;
	const char *sta;

	str = blobmsg_format_json(msg, true);
	fprintf(stdout, "I got %s event %s\n", type, str);

	if(!strcmp(type, "wps")) {
		if(sta = json_parse_and_get(str, "status"))
			fprintf(stdout, "%s status is %s\n", type, sta);
		else if (sta = json_parse_and_get(str, "sta"))
			fprintf(stdout, "%s connected sta is %s\n", type, sta);
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
		return 1;
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
