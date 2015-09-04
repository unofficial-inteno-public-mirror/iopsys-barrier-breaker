#include "somethingd.h"

enum {
    LIST_NAME,
    __LIST_MAX,
};

static const struct blobmsg_policy alljoyn_list_policy[__LIST_MAX] = {
	[LIST_NAME] = { .name = "info", .type = BLOBMSG_TYPE_STRING },
};

enum {
	WHO_INTERFACE,
	__WHO_MAX,
};

static const struct blobmsg_policy alljoyn_who_policy[__WHO_MAX] = {
	[WHO_INTERFACE] = { .name = "interface", .type = BLOBMSG_TYPE_STRING },
};

enum {
	CON_DEVICE_NAME,
	__CON_MAX,
};

static const struct blobmsg_policy alljoyn_con_policy[__CON_MAX] = {
	[CON_DEVICE_NAME] = { .name = "deviceId", .type = BLOBMSG_TYPE_STRING },
};

enum {
	SET_LAMP_BRIGHTNESS,
	SET_LAMP_COLORTEMP,
	__SET_LAMP_MAX,
};

static const struct blobmsg_policy alljoyn_set_lamp_policy[__SET_LAMP_MAX] = {
	[SET_LAMP_BRIGHTNESS] = { .name = "brightness", .type = BLOBMSG_TYPE_INT32 },
	[SET_LAMP_COLORTEMP] = { .name = "colortemp", .type = BLOBMSG_TYPE_INT32 },
};

enum {
	SET_WIFI_SSID,
	SET_WIFI_PASS,
	SET_CHANNEL_2G,
	SET_CHANNEL_5G,
	__SET_WIFI_MAX,
};

static const struct blobmsg_policy alljoyn_set_wifi_policy[__SET_WIFI_MAX] = {
	[SET_WIFI_SSID] = { .name = "ssid", .type = BLOBMSG_TYPE_STRING },
	[SET_WIFI_PASS] = { .name = "password", .type = BLOBMSG_TYPE_STRING },
	[SET_CHANNEL_2G] = { .name = "channel2G", .type = BLOBMSG_TYPE_INT32 },
	[SET_CHANNEL_5G] = { .name = "channel5G", .type = BLOBMSG_TYPE_INT32 },
};

enum {
	SET_ONOFF_VALUE,
	__SET_ONOFF_MAX,
};

static const struct blobmsg_policy alljoyn_set_lamp_onoff_policy[__SET_ONOFF_MAX] = {
	[SET_ONOFF_VALUE] = { .name = "onoff", .type = 	BLOBMSG_TYPE_BOOL },
};


static int something_alljoyn_set_lamp_onoff(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method, struct blob_attr *msg){
	struct blob_attr *tb[__SET_ONOFF_MAX];
	blobmsg_parse(alljoyn_set_lamp_onoff_policy, __SET_ONOFF_MAX, tb, blob_data(msg), blob_len(msg));
	
	if(!(tb[SET_ONOFF_VALUE]))return UBUS_STATUS_INVALID_ARGUMENT;
	bool onoff = (bool)blobmsg_get_bool(tb[SET_ONOFF_VALUE]);
	
	bool success = setLampOnOff(onoff);

	blob_buf_init(&bb, 0);
	
	if(success)blobmsg_add_string(&bb, "info", "success");
	else blobmsg_add_string(&bb, "info", "error");
	
	ubus_send_reply(ctx, req, bb.head);
	
	return 0;	
}

static int something_alljoyn_set_lamp(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method, struct blob_attr *msg){
	struct blob_attr *tb[__SET_LAMP_MAX];
	blobmsg_parse(alljoyn_set_lamp_policy, __SET_LAMP_MAX, tb, blob_data(msg), blob_len(msg));
	
	if((!(tb[SET_LAMP_BRIGHTNESS])) && (!(tb[SET_LAMP_COLORTEMP])))return UBUS_STATUS_INVALID_ARGUMENT;
	
	int brightness = -1;
	int colortemp = -1;
	
	if(tb[SET_LAMP_BRIGHTNESS])brightness = (int)blobmsg_get_u32(tb[SET_LAMP_BRIGHTNESS]);
	if(tb[SET_LAMP_COLORTEMP])colortemp = (int)blobmsg_get_u32(tb[SET_LAMP_COLORTEMP]);
	
	if(brightness > 100 || colortemp > 100)return UBUS_STATUS_INVALID_ARGUMENT;
	
	blob_buf_init(&bb, 0);
	bool success = setLampValues(brightness, colortemp);
	if(success){
		blobmsg_add_string(&bb, "info", "success");
	}else{
		blobmsg_add_string(&bb, "info", "fail");
	}
	ubus_send_reply(ctx, req, bb.head);
	
	return 0;	
}

static int something_alljoyn_con(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method, struct blob_attr *msg){
	struct blob_attr *tb[__CON_MAX];

	blobmsg_parse(alljoyn_con_policy, __CON_MAX, tb, blob_data(msg), blob_len(msg));

    if (!(tb[CON_DEVICE_NAME]))return UBUS_STATUS_INVALID_ARGUMENT;
    
	blob_buf_init(&bb, 0);
    if(connectToDevice((char*)blobmsg_data(tb[CON_DEVICE_NAME]))){
        blobmsg_add_string(&bb, "info", "connection succeded");
    }else{
        blobmsg_add_string(&bb, "info", "connection failed");
    }

	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int something_alljoyn_who(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method, struct blob_attr *msg){
	struct blob_attr *tb[__WHO_MAX];
	blobmsg_parse(alljoyn_who_policy, __WHO_MAX, tb, blob_data(msg), blob_len(msg));


	if (!(tb[WHO_INTERFACE]) || (strcmp(blobmsg_data(tb[WHO_INTERFACE]), "org.allseen.WiFi") && 
	                  strcmp(blobmsg_data(tb[WHO_INTERFACE]), "org.allseen.LSF.LampState"))){
		return UBUS_STATUS_INVALID_ARGUMENT;
    }

	blob_buf_init(&bb, 0);

    if(searchForInterface(blobmsg_data(tb[WHO_INTERFACE]))){
        blobmsg_add_string(&bb, "info", "who implements called");
    }else{
        blobmsg_add_string(&bb, "info", "failed to call who implements");
    }

	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int something_alljoyn_list(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method,  struct blob_attr *msg){
	blob_buf_init(&bb, 0);
	int numdevs;
	struct RetDevice * devs = getAllDevices(&numdevs);
	int i;
	void* t;
	void* a;
	if(numdevs == -1){
		blobmsg_add_string(&bb, "error", "no devices in list");
	}else if(devs == NULL){
		blobmsg_add_string(&bb, "error", "error getting devices");
	}else{
		a = blobmsg_open_array(&bb, "devices");
		for(i = 0; i < numdevs; i++){
			t = blobmsg_open_table(&bb, NULL);
			blobmsg_add_string(&bb, "name", devs[i].name);
			blobmsg_add_string(&bb, "deviceId", devs[i].deviceId);
			blobmsg_close_table(&bb, t);
		}
		blobmsg_close_array(&bb, a);
	}
	ubus_send_reply(ctx, req, bb.head);
    free(devs);
   	return 0;
}

static int something_alljoyn_get_lamp(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method,  struct blob_attr *msg){
	blob_buf_init(&bb, 0);
	
	uint32_t brightness, colortemp;
	bool onoff;
	bool success = getLampValues(&brightness, &colortemp, &onoff);
	
	if(success){
		blobmsg_add_u32(&bb, "brightness", brightness);
		blobmsg_add_u32(&bb, "colortemp", colortemp);
		blobmsg_add_u8(&bb, "onoff", onoff);
	}else{
		blobmsg_add_u32(&bb, "brightness", 101);
		blobmsg_add_u32(&bb, "colortemp", 101);
		blobmsg_add_u8(&bb, "onoff", 0);
	}
	
	ubus_send_reply(ctx, req, bb.head);
	return 0;
}

static int something_alljoyn_get_wifi(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method,  struct blob_attr *msg){
	blob_buf_init(&bb, 0);
	char SSID[MAX_STRING_LENGTH];
	char pass[MAX_STRING_LENGTH];
	uint32_t channel2G, channel5G;
	
	bool success = getWifiValues(SSID, pass, &channel2G, &channel5G);
	

	if(success){
		blobmsg_add_string(&bb, "ssid", SSID);
		blobmsg_add_string(&bb, "password", pass);
		blobmsg_add_u32(&bb, "channel2G", channel2G);
		blobmsg_add_u32(&bb, "channel5G", channel5G);
		if(channel2G != -1){
			size_t size = 0;
			int* list2G = getChannels(2, &size);
			void* a = blobmsg_open_array(&bb, "list2GChannels");
			int i;
			for(i = 0; i < size; i++){
				blobmsg_add_u32(&bb, NULL, list2G[i]);
			}
			blobmsg_close_array(&bb, a);
			free(list2G);
		}
		if(channel5G != -1){
			size_t size = 0;
			int* list5G = getChannels(5, &size);
			void* a = blobmsg_open_array(&bb, "list5GChannels");
			int i;
			for(i = 0; i < size; i++){
				blobmsg_add_u32(&bb, NULL, list5G[i]);
			}
			blobmsg_close_array(&bb, a);
			free(list5G);
		}
	}else{
		blobmsg_add_string(&bb, "ssid", "error");
		blobmsg_add_string(&bb, "password", "error");
		blobmsg_add_u32(&bb, "channel2G", 101);
		blobmsg_add_u32(&bb, "channel5G", 101);
	}
	
	ubus_send_reply(ctx, req, bb.head);
	return 0;
}

static int something_alljoyn_set_wifi(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method, struct blob_attr *msg){
	struct blob_attr *tb[__SET_WIFI_MAX];
	blobmsg_parse(alljoyn_set_wifi_policy,__SET_WIFI_MAX, tb, blob_data(msg), blob_len(msg));
	
	if(!(tb[SET_WIFI_SSID] || tb[SET_WIFI_PASS] || tb[SET_CHANNEL_2G] || tb[SET_CHANNEL_5G]))return UBUS_STATUS_INVALID_ARGUMENT;
	
	blob_buf_init(&bb, 0);// clear return buffer
	int channel2G = -1, channel5G = -1;
	char SSID[MAX_STRING_LENGTH]; 
	char pass[MAX_STRING_LENGTH];
	strcpy(SSID, "null");
	strcpy(pass, "null");
	if(tb[SET_WIFI_SSID])strcpy(SSID, blobmsg_get_string(tb[SET_WIFI_SSID]));
	if(tb[SET_WIFI_PASS])strcpy(pass, blobmsg_get_string(tb[SET_WIFI_PASS]));
	if(tb[SET_CHANNEL_2G])channel2G = (int)blobmsg_get_u32(tb[SET_CHANNEL_2G]);
	if(tb[SET_CHANNEL_5G])channel5G = (int)blobmsg_get_u32(tb[SET_CHANNEL_5G]);

	bool success = setWifiValues(SSID, pass, channel2G, channel5G);
	
	if(success)blobmsg_add_string(&bb, "info", "success");
	else blobmsg_add_string(&bb, "info", "fail");
		
	ubus_send_reply(ctx, req, bb.head);
	return 0;
}

static struct ubus_method alljoyn_object_methods[] = {
	UBUS_METHOD_NOARG("listDevices", something_alljoyn_list),
	UBUS_METHOD("whoImplements", something_alljoyn_who, alljoyn_who_policy),
	UBUS_METHOD("connect", something_alljoyn_con, alljoyn_con_policy),
	UBUS_METHOD("setLampValues", something_alljoyn_set_lamp, alljoyn_set_lamp_policy),
	UBUS_METHOD_NOARG("getLampValues", something_alljoyn_get_lamp),
	UBUS_METHOD("setWifiValues", something_alljoyn_set_wifi, alljoyn_set_wifi_policy),
	UBUS_METHOD_NOARG("getWifiValues", something_alljoyn_get_wifi),
	UBUS_METHOD("setLampOnOff", something_alljoyn_set_lamp_onoff, alljoyn_set_lamp_onoff_policy),
};

static struct ubus_object_type alljoyn_object_type =
	UBUS_OBJECT_TYPE("alljoyn", alljoyn_object_methods);

static struct ubus_object router_object = {
	.name = "alljoyn",
	.type = &alljoyn_object_type,
	.methods = alljoyn_object_methods,
	.n_methods = ARRAY_SIZE(alljoyn_object_methods),
};


static void  system_fd_set_cloexec(int fd){
#ifdef FD_CLOEXEC
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
#endif
}

static void quest_ubus_add_fd(void){
	ubus_add_uloop(ctx);
	system_fd_set_cloexec(ctx->sock.fd);
}


static void quest_ubus_reconnect_timer(struct uloop_timeout *timeout){
	static struct uloop_timeout retry = {
		.cb = quest_ubus_reconnect_timer,
	};
	int t = 2;

	if (ubus_reconnect(ctx, ubus_path) != 0) {
		printf("failed to reconnect, trying again in %d seconds\n", t);
		uloop_timeout_set(&retry, t * 1000);
		return;
	}

	printf("reconnected to ubus, new id: %08x\n", ctx->local_id);
	quest_ubus_add_fd();
}

static void quest_ubus_connection_lost(struct ubus_context *ctx){
    quest_ubus_reconnect_timer(NULL);
}

static void something_add_object(struct ubus_object *obj){
	int ret = ubus_add_object(ctx, obj);

	if (ret != 0)
		fprintf(stderr, "Failed to publish object '%s': %s\n", obj->name, ubus_strerror(ret));
}

static int quest_ubus_init(const char *path){
	uloop_init();
	ubus_path = path;

	ctx = ubus_connect(path);
	if (!ctx)
		return -EIO;

	printf("connected as %08x\n", ctx->local_id);
	ctx->connection_lost = quest_ubus_connection_lost;
	quest_ubus_add_fd();

	something_add_object(&router_object);

	return 0;
}

void initUbus(){
	int pt;

	if (quest_ubus_init(NULL) < 0) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return;
	}

	uloop_run();
	ubus_free(ctx);	
}
