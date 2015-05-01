/*
 * questd -- router info daemon for Inteno CPEs
 *
 * Copyright (C) 2012-2013 Inteno Broadband Technology AB. All rights reserved.
 *
 * Author: sukru.senli@inteno.se
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <libubox/blobmsg.h>
#include <libubox/uloop.h>
#include <libubox/ustream.h>
#include <libubox/utils.h>

#include <libubus.h>

#include "questd.h"

#define DEFAULT_SLEEP	5000000

static struct uci_context *uci_ctx;
static struct uci_package *uci_network, *uci_wireless;
static struct ubus_context *ctx = NULL;
static struct blob_buf bb;
static const char *ubus_path;

static Wireless wireless[MAX_VIF];
static Network network[MAX_NETWORK];
static Detail details[MAX_CLIENT];
static Client clients[MAX_CLIENT], clients_old[MAX_CLIENT], clients_new[MAX_CLIENT];
static Client6 clients6[MAX_CLIENT];
static Router router;
static Memory memory;
static Key keys;
static Spec spec;
static USB usb[MAX_USB];

/* POLICIES */
enum {
	QUEST_NAME,
	__QUEST_MAX,
};

static const struct blobmsg_policy quest_policy[__QUEST_MAX] = {
	[QUEST_NAME] = { .name = "info", .type = BLOBMSG_TYPE_STRING },
};

enum {
	NETWORK_NAME,
	__NETWORK_MAX,
};

static const struct blobmsg_policy network_policy[__NETWORK_MAX] = {
	[NETWORK_NAME] = { .name = "network", .type = BLOBMSG_TYPE_STRING },
};

enum {
	RADIO_NAME,
	VIF_NAME,
	__WL_MAX,
};

static const struct blobmsg_policy wl_policy[__WL_MAX] = {
	[RADIO_NAME] = { .name = "radio", .type = BLOBMSG_TYPE_STRING },
	[VIF_NAME] = { .name = "vif", .type = BLOBMSG_TYPE_STRING },
};

enum {
	IP_ADDR,
	MAC_ADDR,
	__HOST_MAX,
};

static const struct blobmsg_policy host_policy[__HOST_MAX] = {
	[IP_ADDR] = { .name = "ipaddr", .type = BLOBMSG_TYPE_STRING },
	[MAC_ADDR] = { .name = "macaddr", .type = BLOBMSG_TYPE_STRING },
};
/* END POLICIES */

pthread_t tid[1];
static long sleep_time = DEFAULT_SLEEP;

void recalc_sleep_time(bool calc, int toms)
{
	long dec = toms * 1000;
	if (!calc)
		sleep_time = DEFAULT_SLEEP;
	else if(sleep_time >= dec)
		sleep_time -= dec;
}

static struct uci_package *
init_package(const char *config)
{
	struct uci_context *ctx = uci_ctx;
	struct uci_package *p = NULL;

	if (!ctx) {
		ctx = uci_alloc_context();
		uci_ctx = ctx;
	} else {
		p = uci_lookup_package(ctx, config);
		if (p)
			uci_unload(ctx, p);
	}

	if (uci_load(ctx, config, &p))
		return NULL;

	return p;
}

static void 
system_fd_set_cloexec(int fd)
{
#ifdef FD_CLOEXEC
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
#endif
}

static bool
wdev_already_there(const char *ifname, char *wdev)
{
	bool ret = false;
	char *token;
	char ifbuf[128];

	strcpy(ifbuf, ifname);

	token = strtok(ifbuf, " ");
	while (token != NULL)
	{
		if(!strcmp(token, wdev)) {
			ret = true;
			break;
		}
		token = strtok (NULL, " ");
	}

	return ret;
}

static void
get_wifs(char *netname, const char *ifname, char **wifs)
{
	struct uci_element *e;
	const char *device = NULL;
	const char *network = NULL;
	char wdev[16];
	char wrl[64];
	const char *wdevs[2];		
	int vif, wno;

	wdevs[0] = "wl0";
	wdevs[1] = "wl1";

	*wifs = NULL;

	memset(wrl, '\0', sizeof(wrl));
	if((uci_wireless = init_package("wireless"))) {
		for(wno = 0; wno <= 1; wno++) {
			vif = 0;
			uci_foreach_element(&uci_wireless->sections, e) {
				struct uci_section *s = uci_to_section(e);

				if (!strcmp(s->type, "wifi-iface")) {
					device = uci_lookup_option_string(uci_ctx, s, "device");
					if(!device || strcmp(device, wdevs[wno]))
						continue;
					network = uci_lookup_option_string(uci_ctx, s, "network");
					if (network && device && !strcmp(network, netname)) {
						if (vif > 0)
							sprintf(wdev, "%s.%d", device, vif);
						else
							strcpy(wdev, device);

						if(wdev_already_there(ifname, wdev))
							continue;

						strcat(wrl, " ");
						strcat(wrl, wdev);
						*wifs = strdup(wrl);
					}
					vif++;
				}
			}
		}
	}
}

static void
load_networks()
{
	struct uci_element *e;
	const char *is_lan = NULL;
	const char *type = NULL;
	const char *proto = NULL;
	const char *ipaddr = NULL;
	const char *netmask = NULL;
	const char *ifname = NULL;
	char *wifs;
	int nno = 0;

	memset(network, '\0', sizeof(network));

	if((uci_network = init_package("network"))) {
		uci_foreach_element(&uci_network->sections, e) {
			struct uci_section *s = uci_to_section(e);

			network[nno].exists = false;
			network[nno].ports_populated = false;
			if (!strcmp(s->type, "interface")) {
				is_lan = uci_lookup_option_string(uci_ctx, s, "is_lan");
				type = uci_lookup_option_string(uci_ctx, s, "type");
				proto = uci_lookup_option_string(uci_ctx, s, "proto");
				ipaddr = uci_lookup_option_string(uci_ctx, s, "ipaddr");
				netmask = uci_lookup_option_string(uci_ctx, s, "netmask");
				ifname = uci_lookup_option_string(uci_ctx, s, "ifname");
				if(!(ifname))
					ifname = "";
				get_wifs(s->e.name, ifname, &wifs);
				if ((ifname && strcmp(ifname, "lo")) || wifs) {
					network[nno].exists = true;
					if(is_lan && !strcmp(is_lan, "1"))
						network[nno].is_lan = true;
					network[nno].name = s->e.name;
					(type) ? (network[nno].type = type) : (network[nno].type = "");
					(proto) ? (network[nno].proto = proto) : (network[nno].proto = "");
					if(proto && !strcmp(network[nno].proto, "static")) {
						(ipaddr) ? (network[nno].ipaddr = ipaddr) : (network[nno].ipaddr = "");
						(netmask) ? (network[nno].netmask = netmask) : (network[nno].netmask = "");
					}
					if(wifs)
						sprintf(network[nno].ifname, "%s%s", ifname, wifs);
					else
						strcpy(network[nno].ifname, ifname);
					nno++;
				}
			}
		}
	}
}

static void
load_wireless()
{
	struct uci_element *e;
	const char *device = NULL;
	const char *network = NULL;
	const char *ssid = NULL;
	char wdev[16];
	int wno = 0;
	int vif;
	int vif0 = 0;
	int vif1 = 0;

	memset(wireless, '\0', sizeof(wireless));

	if((uci_wireless = init_package("wireless"))) {
		uci_foreach_element(&uci_wireless->sections, e) {
			struct uci_section *s = uci_to_section(e);

			if (!strcmp(s->type, "wifi-iface")) {
				device = uci_lookup_option_string(uci_ctx, s, "device");
				network = uci_lookup_option_string(uci_ctx, s, "network");
				ssid = uci_lookup_option_string(uci_ctx, s, "ssid");
				if (device) {
					wireless[wno].device = device;
					(network) ? (wireless[wno].network = network) : (wireless[wno].network = "");
					(ssid) ? (wireless[wno].ssid = ssid) : (wireless[wno].ssid = "");
					if (!strcmp(device, "wl0")) {
						vif = vif0;
						vif0++;
					} else {
						vif = vif1;
						vif1++;
					}
					if (vif > 0)
						sprintf(wdev, "%s.%d", device, vif);
					else
						strcpy(wdev, device);

					wireless[wno].vif = strdup(wdev);

					wno++;
				}
			}
		}
	}
}

static void
match_client_to_network(Network *lan, char *hostaddr, bool *local, char **net, char **dev)
{
	if(!lan->ipaddr || !lan->netmask)
		return;

	struct in_addr ip, mask, snet, host, rslt;

	inet_pton(AF_INET, lan->ipaddr, &(ip.s_addr));
	inet_pton(AF_INET, lan->netmask, &(mask.s_addr));
	inet_pton(AF_INET, hostaddr, &(host.s_addr));

	snet.s_addr = (ip.s_addr & mask.s_addr);
	rslt.s_addr = (host.s_addr & mask.s_addr);

	if((snet.s_addr ^ rslt.s_addr) == 0) {
		*local = true;
		snprintf(net, 32, lan->name);
		if (lan->type && !strcmp(lan->type, "bridge"))
			snprintf(dev, 32, "br-%s", lan->name);
		else
			snprintf(dev, 32, lan->ifname);
	}
}

static void
handle_client(Client *clnt)
{
	int ip[4];
	int netno;

	clnt->local = false;
	if (sscanf(clnt->hostaddr, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4) {
		for (netno=0; network[netno].exists; netno++) {
			if (network[netno].is_lan) {
				match_client_to_network(&network[netno], clnt->hostaddr, &clnt->local, (char**)&clnt->network, (char**)&clnt->device);
				if (clnt->local)
					break;
			}
		}
	}
}

static bool
wireless_sta(Client *clnt, Detail *dtl)
{
	FILE *stainfo;
	char cmnd[64];
	char line[128];
	int i = 0;
	bool there = false;
	char tab[16];
	int ret, tmp;
	int noise;

	for (i = 0; wireless[i].device; i++) {
		sprintf(cmnd, "wlctl -i %s sta_info %s 2>/dev/null", wireless[i].vif, clnt->macaddr);
		if ((stainfo = popen(cmnd, "r"))) {
			while(fgets(line, sizeof(line), stainfo) != NULL)
			{
				remove_newline(line);
				if(sscanf(line, "%sstate: AUTHENTICATED ASSOCIATED AUTHORIZED", tab)) {
					there = true;
					strncpy(clnt->wdev, wireless[i].vif, sizeof(clnt->wdev));
				}
				ret = sscanf(line, "\t idle %d seconds", &(dtl->idle));
				ret = sscanf(line, "\t in network %d seconds", &(dtl->in_network));
				ret = sscanf(line, "\t tx total bytes: %ld\n", &(dtl->tx_bytes));
				ret = sscanf(line, "\t rx data bytes: %ld", &(dtl->rx_bytes));
				ret = sscanf(line, "\t rate of last tx pkt: %d kbps - %d kbps", &tmp, &(dtl->tx_rate));
				ret = sscanf(line, "\t rate of last rx pkt: %d kbps", &(dtl->rx_rate));
			}
			pclose(stainfo);
		}
		if (there) {
			sprintf(cmnd, "wlctl -i %s noise", wireless[i].device);
			if ((stainfo = popen(cmnd, "r"))) {
				fread(line, sizeof(line), 1, stainfo);
				remove_newline(line);
				noise = atoi(line);
				pclose(stainfo);
			}
			sprintf(cmnd, "wlctl -i %s rssi %s", wireless[i].vif, clnt->macaddr);
			if ((stainfo = popen(cmnd, "r"))) {
				fread(line, sizeof(line), 1, stainfo);
				remove_newline(line);
				dtl->snr = atoi(line) - noise;
				pclose(stainfo);
			}
		}
		if (there)
			break;
	}
	return there;
}

static int
active_connections(char *ipaddr)
{
	FILE *conn;
	char cmnd[64];
	char line[8];
	int connum = 0;

	sprintf(cmnd, "grep %s /proc/net/ip_conntrack | wc -l", ipaddr);
	if ((conn = popen(cmnd, "r"))) {
		fread(line, sizeof(line), 1, conn);
		remove_newline(line);
		connum = atoi(line);
		pclose(conn);
	}

	return connum;
}

static void
populate_clients()
{
	FILE *leases, *arpt;
	char line[256];
	int cno = 0;
	int lno = 0;
	int hw;
	int flag;
	char mask[64];
	int i;
	bool there;
	int toms = 1000;

	memset(clients_new, '\0', sizeof(clients));

	if ((leases = fopen("/var/dhcp.leases", "r"))) {
		while(fgets(line, sizeof(line), leases) != NULL)
		{
			remove_newline(line);
			clients[cno].exists = false;
			clients[cno].wireless = false;
			memset(clients[cno].hostname, '\0', 64);
			if (sscanf(line, "%s %s %s %s %s", clients[cno].leaseno, clients[cno].macaddr, clients[cno].hostaddr, clients[cno].hostname, mask) == 5) {
				clients[cno].exists = true;
				clients[cno].dhcp = true;
				handle_client(&clients[cno]);
				if((clients[cno].connected = wireless_sta(&clients[cno], &details[cno])))
					clients[cno].wireless = true;
				else if(!(clients[cno].connected = arping(clients[cno].hostaddr, clients[cno].device, toms)))
					recalc_sleep_time(true, toms);

				if (clients[cno].connected)
					details[cno].connum = active_connections(clients[cno].hostaddr);
				cno++;
			}
		}
		fclose(leases);
	}

	if ((arpt = fopen("/proc/net/arp", "r"))) {
		while(fgets(line, sizeof(line), arpt) != NULL)
		{
			remove_newline(line);
			there = false;
			clients[cno].exists = false;
			clients[cno].wireless = false;
			memset(clients[cno].hostname, '\0', 64);
			if ((lno > 0) && sscanf(line, "%s 0x%d 0x%d %s %s %s", clients[cno].hostaddr, &hw, &flag, clients[cno].macaddr, mask, clients[cno].device)) {
				for (i=0; i < cno; i++) {
					if (!strcmp(clients[cno].macaddr, clients[i].macaddr)) {
						if (clients[i].connected) {
							there = true;
							break;
						} else {
							strcpy(clients[cno].hostname, clients[i].hostname);
						}
					}
					if (!strcmp(clients[cno].hostaddr, clients[i].hostaddr)) {
						there = true;
						break;
					}
				}
				if (!there) {
					handle_client(&clients[cno]);
					if(clients[cno].local) {
						clients[cno].exists = true;
						clients[cno].dhcp = false;
						if((clients[cno].connected = wireless_sta(&clients[cno], &details[cno])))
							clients[cno].wireless = true;
						else if(!(clients[cno].connected = arping(clients[cno].hostaddr, clients[cno].device, toms)))
							recalc_sleep_time(true, toms);
						cno++;

						if (clients[cno].connected)
							details[cno].connum = active_connections(clients[cno].hostaddr);
					}
				}
			}
			lno++;
		}
		fclose(arpt);
	}

	memcpy(&clients_new, &clients, sizeof(clients));
	if(memcmp(&clients_new, &clients_old, sizeof(clients)))
		system("ubus send client");
	memcpy(&clients_old, &clients_new, sizeof(clients));
}

static bool
wireless_sta6(Client6 *clnt6)
{
	FILE *stainfo;
	char cmnd[64];
	char line[128];
	int i = 0;
	bool there = false;

	for (i = 0; wireless[i].device; i++) {
		sprintf(cmnd, "wlctl -i %s sta_info %s 2>/dev/null | grep ASSOCIATED", wireless[i].vif, clnt6->macaddr);
		if ((stainfo = popen(cmnd, "r"))) {
			if(fgets(line, sizeof(line), stainfo) != NULL) {
				there = true;
				strncpy(clnt6->wdev, wireless[i].vif, sizeof(clnt6->wdev));
			}
			pclose(stainfo);
		}
		if (there)
			break;
	}
	return there;
}

static void
populate_clients6()
{
	FILE *hosts6;
	char line[512];
	int cno = 0;
	int iaid, ts, id, length;
	int toms = 500;

	if ((hosts6 = fopen("/tmp/hosts/6relayd", "r"))) {
		while(fgets(line, sizeof(line), hosts6) != NULL)
		{
			remove_newline(line);
			clients6[cno].exists = false;
			clients6[cno].wireless = false;
			memset(clients6[cno].hostname, '\0', 64);
			if (sscanf(line, "# %s %s %x %s %d %x %d %s", clients6[cno].device, clients6[cno].duid, &iaid, clients6[cno].hostname, &ts, &id, &length, clients6[cno].ip6addr)) {
				clients6[cno].exists = true;
				clear_macaddr();
				if(!(clients6[cno].connected = ndisc (clients6[cno].hostname, clients6[cno].device, 0x8, 1, toms)))
					recalc_sleep_time(true, toms);
				sprintf(clients6[cno].macaddr, get_macaddr());
				if(clients6[cno].connected && wireless_sta6(&clients6[cno]))
					clients6[cno].wireless = true;
				cno++;
			}
		}
		fclose(hosts6);
	}
}

static void
populate_ports(Network *network)
{
	char bridge[32];
	char *macaddr;
	char *theports;
	char *prt, *mac;
	int i = 1;
	int j, k, l;
	Port *port = (Port*)&network->port;
	
	sprintf(bridge, "br-%s", network->name);

	if (network->ports_populated)
		goto get_clients;

	get_bridge_ports(bridge, &theports);
	memset(port, '\0', sizeof(Port));

	prt = strtok(theports, " ");
	while (prt != NULL)
	{
		strcpy(port[i].device, prt);
		get_port_name(&port[i]);
		prt = strtok (NULL, " ");
		i++;
	}
	
	network->ports_populated = true;
		
get_clients:	
	for(i=1; strlen(port[i].device)>2; i++)
	{				
		memset(&port[i].stat, '\0', sizeof(Statistic));
		for (j=0; port[i].client[j].exists; j++) {
			memset(&port[i].client[j], '\0', sizeof(Client));
		}
		
		get_port_stats(&port[i]);
		get_clients_onport(bridge, i, &macaddr);

		l = 0;
		if(network->is_lan) {
			for (k=0; clients[k].exists; k++) {
				if (strstr(macaddr, clients[k].macaddr) && clients[k].connected) {
					port[i].client[l] = clients[k];
					l++;
				}
			}
		} else {
			mac = strtok(macaddr, " ");
			while (mac != NULL)
			{
				port[i].client[l].exists = true;
				strcpy(port[i].client[l].macaddr, mac);
				mac = strtok (NULL, " ");
				l++;
			}	
		}
	}
}

static void
router_dump_specs(struct blob_buf *b, bool table)
{
	void *t;

	if (table) t = blobmsg_open_table(b, "specs");
	blobmsg_add_u8(b, "wifi", spec.wifi);
	blobmsg_add_u8(b, "adsl", spec.adsl);
	blobmsg_add_u8(b, "vdsl", spec.vdsl);
	blobmsg_add_u8(b, "voice", spec.voice);
	blobmsg_add_u32(b, "voice_ports", spec.vports);
	blobmsg_add_u32(b, "eth_ports", spec.eports);
	if (table) blobmsg_close_table(b, t);
}

static void
router_dump_keys(struct blob_buf *b, bool table)
{
	void *t;

	if (table) t = blobmsg_open_table(b, "keys");
	blobmsg_add_string(b, "auth", keys.auth);
	blobmsg_add_string(b, "des", keys.des);
	blobmsg_add_string(b, "wpa", keys.wpa);
	if (table) blobmsg_close_table(b, t);
}

static void
router_dump_system_info(struct blob_buf *b, bool table)
{
	void *t;

	if (table) t = blobmsg_open_table(b, "system");
	blobmsg_add_string(b, "name", router.name);
	blobmsg_add_string(b, "hardware", router.hardware);
	blobmsg_add_string(b, "model", router.model);
	blobmsg_add_string(b, "nvramver", router.nvramver); 
	blobmsg_add_string(b, "firmware", router.firmware);
	blobmsg_add_string(b, "brcmver", router.brcmver);
	blobmsg_add_string(b, "socmod", router.socmod);
	blobmsg_add_string(b, "socrev", router.socrev);
	blobmsg_add_string(b, "cfever", router.cfever);
	blobmsg_add_string(b, "kernel", router.kernel);
	blobmsg_add_string(b, "basemac", router.basemac);
	blobmsg_add_string(b, "serialno", router.serialno);
	blobmsg_add_string(b, "uptime", router.uptime);
	blobmsg_add_u32(b, "procs", router.procs);
	blobmsg_add_u32(b, "cpu_per", router.cpu);
	if (table) blobmsg_close_table(b, t);
}

static void
router_dump_memory_info(struct blob_buf *b, bool table)
{
	void *t;

	if (table) t = blobmsg_open_table(b, "memoryKB");
	blobmsg_add_u64(b, "total", memory.total);
	blobmsg_add_u64(b, "used", memory.used);
	blobmsg_add_u64(b, "free", memory.free);
	blobmsg_add_u64(b, "shared", memory.shared);
	blobmsg_add_u64(b, "buffers", memory.buffers);
	if (table) blobmsg_close_table(b, t);
}

static void
router_dump_networks(struct blob_buf *b)
{
	void *t;
	int i;

	for (i = 0; i < MAX_NETWORK; i++) {
		if (!network[i].exists)
			break;
		t = blobmsg_open_table(b, network[i].name);
		blobmsg_add_u8(b, "is_lan", network[i].is_lan);
		blobmsg_add_string(b, "type", network[i].type);
		blobmsg_add_string(b, "proto", network[i].proto);
		if (!strcmp(network[i].proto, "static")) {
			blobmsg_add_string(b, "ipaddr", network[i].ipaddr);
			blobmsg_add_string(b, "netmask", network[i].netmask);
		}
		blobmsg_add_string(b, "ifname", network[i].ifname);
		blobmsg_close_table(b, t);
	}
}

static void
router_dump_clients(struct blob_buf *b)
{
	void *t;
	char clientnum[10];
	int num = 1;
	int i;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (!clients[i].exists)
			break;
		sprintf(clientnum, "client-%d", num);
		t = blobmsg_open_table(b, clientnum);
		blobmsg_add_string(b, "hostname", clients[i].hostname);
		blobmsg_add_string(b, "ipaddr", clients[i].hostaddr);
		blobmsg_add_string(b, "macaddr", clients[i].macaddr);
		blobmsg_add_string(b, "network", clients[i].network);
		blobmsg_add_string(b, "device", clients[i].device);
		blobmsg_add_u8(b, "dhcp", clients[i].dhcp);
		blobmsg_add_u8(b, "connected", clients[i].connected);
		blobmsg_add_u8(b, "wireless", clients[i].wireless);
		if(clients[i].connected)
			blobmsg_add_u32(b, "active_cons", details[i].connum);
		if(clients[i].wireless) {
			blobmsg_add_string(b, "wdev", clients[i].wdev);
			blobmsg_add_u32(b, "idle", details[i].idle);
			blobmsg_add_u32(b, "in_network", details[i].in_network);
			blobmsg_add_u32(b, "snr", details[i].snr);
			blobmsg_add_u64(b, "tx_bytes", details[i].tx_bytes);
			blobmsg_add_u64(b, "rx_bytes", details[i].rx_bytes);
			blobmsg_add_u32(b, "tx_rate", details[i].tx_rate);
			blobmsg_add_u32(b, "rx_rate", details[i].rx_rate);
		}
		blobmsg_close_table(b, t);
		num++;
	}
}

static void
router_dump_connected_clients(struct blob_buf *b)
{
	void *t;
	char clientnum[10];
	int num = 1;
	int i;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (!clients[i].exists)
			break;
		if (!(clients[i].connected))
			continue;
		sprintf(clientnum, "client-%d", num);
		t = blobmsg_open_table(b, clientnum);
		blobmsg_add_string(b, "hostname", clients[i].hostname);
		blobmsg_add_string(b, "ipaddr", clients[i].hostaddr);
		blobmsg_add_string(b, "macaddr", clients[i].macaddr);
		blobmsg_add_string(b, "network", clients[i].network);
		blobmsg_add_string(b, "device", clients[i].device);
		blobmsg_add_u8(b, "dhcp", clients[i].dhcp);
		blobmsg_add_u8(b, "wireless", clients[i].wireless);
		blobmsg_add_u32(b, "active_cons", details[i].connum);
		if(clients[i].wireless) {
			blobmsg_add_string(b, "wdev", clients[i].wdev);
			blobmsg_add_u32(b, "idle", details[i].idle);
			blobmsg_add_u32(b, "in_network", details[i].in_network);
			blobmsg_add_u64(b, "tx_bytes", details[i].tx_bytes);
			blobmsg_add_u64(b, "rx_bytes", details[i].rx_bytes);
			blobmsg_add_u32(b, "tx_rate", details[i].tx_rate);
			blobmsg_add_u32(b, "rx_rate", details[i].rx_rate);
		}
		blobmsg_close_table(b, t);
		num++;
	}
}

static void
router_dump_network_clients(struct blob_buf *b, char *net)
{
	void *t;
	char clientnum[10];
	int num = 1;
	int i;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (!clients[i].exists)
			break;
		if (strcmp(clients[i].network, net))
			continue;
		sprintf(clientnum, "client-%d", num);
		t = blobmsg_open_table(b, clientnum);
		blobmsg_add_string(b, "hostname", clients[i].hostname);
		blobmsg_add_string(b, "ipaddr", clients[i].hostaddr);
		blobmsg_add_string(b, "macaddr", clients[i].macaddr);
		blobmsg_add_string(b, "network", clients[i].network);
		blobmsg_add_string(b, "device", clients[i].device);
		blobmsg_add_u8(b, "dhcp", clients[i].dhcp);
		blobmsg_add_u8(b, "connected", clients[i].connected);
		blobmsg_add_u8(b, "wireless", clients[i].wireless);
		if(clients[i].connected)
			blobmsg_add_u32(b, "active_cons", details[i].connum);
		if(clients[i].wireless) {
			blobmsg_add_string(b, "wdev", clients[i].wdev);
			blobmsg_add_u32(b, "idle", details[i].idle);
			blobmsg_add_u32(b, "in_network", details[i].in_network);
			blobmsg_add_u64(b, "tx_bytes", details[i].tx_bytes);
			blobmsg_add_u64(b, "rx_bytes", details[i].rx_bytes);
			blobmsg_add_u32(b, "tx_rate", details[i].tx_rate);
			blobmsg_add_u32(b, "rx_rate", details[i].rx_rate);
		}
		blobmsg_close_table(b, t);
		num++;
	}
}

static void
router_dump_connected_clients6(struct blob_buf *b)
{
	void *t;
	char clientnum[10];
	int num = 1;
	int i;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (!clients6[i].exists)
			break;
		if (!(clients6[i].connected))
			continue;
		sprintf(clientnum, "client-%d", num);
		t = blobmsg_open_table(b, clientnum);
		blobmsg_add_string(b, "hostname", clients6[i].hostname);
		blobmsg_add_string(b, "ip6addr", clients6[i].ip6addr);
		blobmsg_add_string(b, "macaddr", clients6[i].macaddr);
		blobmsg_add_string(b, "duid", clients6[i].duid);
		blobmsg_add_string(b, "device", clients6[i].device);
		blobmsg_add_u8(b, "wireless", clients6[i].wireless);
		if(clients6[i].wireless) {
			blobmsg_add_string(b, "wdev", clients6[i].wdev);
		}
		blobmsg_close_table(b, t);
		num++;
	}
}

static void
router_dump_clients6(struct blob_buf *b)
{
	void *t;
	char clientnum[10];
	int num = 1;
	int i;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (!clients6[i].exists)
			break;
		sprintf(clientnum, "client-%d", num);
		t = blobmsg_open_table(b, clientnum);
		blobmsg_add_string(b, "hostname", clients6[i].hostname);
		blobmsg_add_string(b, "ip6addr", clients6[i].ip6addr);
		blobmsg_add_string(b, "macaddr", clients6[i].macaddr);
		blobmsg_add_string(b, "duid", clients6[i].duid);
		blobmsg_add_string(b, "device", clients6[i].device);
		blobmsg_add_u8(b, "connected", clients6[i].connected);
		blobmsg_add_u8(b, "wireless", clients6[i].wireless);
		if(clients6[i].wireless) {
			blobmsg_add_string(b, "wdev", clients6[i].wdev);
		}
		blobmsg_close_table(b, t);
		num++;
	}
}

static void
router_dump_stas(struct blob_buf *b)
{
	void *t;
	char stanum[8];
	int num = 1;
	int i;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (!clients[i].exists)
			break;
		if (!(clients[i].wireless))
			continue;
		sprintf(stanum, "sta-%d", num);
		t = blobmsg_open_table(b, stanum);
		blobmsg_add_string(b, "hostname", clients[i].hostname);
		blobmsg_add_string(b, "ipaddr", clients[i].hostaddr);
		blobmsg_add_string(b, "macaddr", clients[i].macaddr);
		blobmsg_add_string(b, "network", clients[i].network);
		blobmsg_add_u8(b, "dhcp", clients[i].dhcp);
		if(strstr(clients[i].device, "br-"))
			blobmsg_add_string(b, "bridge", clients[i].device);
		blobmsg_add_string(b, "wdev", clients[i].wdev);
		blobmsg_add_u32(b, "active_cons", details[i].connum);
		blobmsg_add_u32(b, "idle", details[i].idle);
		blobmsg_add_u32(b, "in_network", details[i].in_network);
		blobmsg_add_u64(b, "tx_bytes", details[i].tx_bytes);
		blobmsg_add_u64(b, "rx_bytes", details[i].rx_bytes);
		blobmsg_add_u32(b, "tx_rate", details[i].tx_rate);
		blobmsg_add_u32(b, "rx_rate", details[i].rx_rate);
		blobmsg_close_table(b, t);
		num++;
	}
}

static void
router_dump_wireless_stas(struct blob_buf *b, char *wname, bool vif)
{
	void *t;
	char stanum[8];
	char compare[8];
	int num = 1;
	int i;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (!clients[i].exists)
			break;
		if(!(clients[i].wireless))
			continue;

		memset(compare, '\0', 8);
		if (vif)
			strcpy(compare, clients[i].wdev);
		else
			strncpy(compare, clients[i].wdev, 3);

		if (strcmp(compare, wname))
			continue;

		sprintf(stanum, "sta-%d", num);
		t = blobmsg_open_table(b, stanum);
		blobmsg_add_string(b, "hostname", clients[i].hostname);
		blobmsg_add_string(b, "ipaddr", clients[i].hostaddr);
		blobmsg_add_string(b, "macaddr", clients[i].macaddr);
		blobmsg_add_string(b, "network", clients[i].network);
		blobmsg_add_u8(b, "dhcp", clients[i].dhcp);
		if(strstr(clients[i].device, "br-"))
			blobmsg_add_string(b, "bridge", clients[i].device);
		if(!vif)
			blobmsg_add_string(b, "wdev", clients[i].wdev);
		blobmsg_add_u32(b, "active_cons", details[i].connum);
		blobmsg_add_u32(b, "idle", details[i].idle);
		blobmsg_add_u32(b, "in_network", details[i].in_network);
		blobmsg_add_u64(b, "tx_bytes", details[i].tx_bytes);
		blobmsg_add_u64(b, "rx_bytes", details[i].rx_bytes);
		blobmsg_add_u32(b, "tx_rate", details[i].tx_rate);
		blobmsg_add_u32(b, "rx_rate", details[i].rx_rate);
		blobmsg_close_table(b, t);
		num++;
	}
}

static void
router_dump_usbs(struct blob_buf *b)
{
	void *t;
	int uno = 0;
	FILE *usbdevs;
	char cmnd[64];
	char line[16];

	memset(usb, '\0', sizeof(usb));
	sprintf(cmnd, "ls /sys/bus/usb/devices/ | grep -v ':' | grep -v 'usb'");
	if ((usbdevs = popen(cmnd, "r"))) {
		while(fgets(line, sizeof(line), usbdevs) != NULL)
		{
			remove_newline(line);
			dump_usb_info(&usb[uno], line);
			if(strlen(usb[uno].product) < 1)
				continue;
			t = blobmsg_open_table(b, usb[uno].name);
			blobmsg_add_string(b, "product", usb[uno].product);
			//blobmsg_add_string(b, "speed", usb[uno].speed);
			if (usb[uno].maxchild && strcmp(usb[uno].maxchild, "0")) {
				blobmsg_add_u32(b, "maxchild", atoi(usb[uno].maxchild));
			}
			else {
				blobmsg_add_string(b, "vendor", usb[uno].vendor);
				blobmsg_add_string(b, "serial", usb[uno].serial);
				if(usb[uno].device) {
					blobmsg_add_string(b, "device", usb[uno].device);
					blobmsg_add_u64(b, "sizeMB", usb[uno].size);
					blobmsg_add_string(b, "mntdir", usb[uno].mount);
				}
			}
			blobmsg_close_table(b, t);
			uno++;
		}
		pclose(usbdevs);
	}
}


static void
router_dump_ports(struct blob_buf *b, char *interface)
{
	void *t, *c, *h, *s;
	int pno, i, j;
	const char *ports[8];
	bool found = false;

	ports[0] = "LAN";
	ports[1] = "LAN1";
	ports[2] = "LAN2";
	ports[3] = "LAN3";
	ports[4] = "LAN4";
	ports[5] = "GbE";
	ports[6] = "WAN";
	ports[7] = "WLAN";

	Port *port;
	
	for (i = 0; i < MAX_NETWORK; i++) {
		if (network[i].exists && !strcmp(network[i].name, interface)) {
			populate_ports(&network[i]);
			port = (Port*)&network[i].port;
			found = true;
			break;
		}
	}
	
	if (!found)
		return;

	for (pno=0; pno<=7; pno++) {
		for (i = 1; strlen(port[i].name) > 2; i++) {
			if(strcmp(port[i].name, ports[pno]))
				continue;
			if(!strncmp(port[i].device, "wl", 2) && strlen(port[i].ssid) > 2)
				t = blobmsg_open_table(b, port[i].ssid);
			else
				t = blobmsg_open_table(b, port[i].name);
			blobmsg_add_string(b, "device", port[i].device);
			c = blobmsg_open_array(b, "hosts");
			for(j=0; port[i].client[j].exists; j++) {
				h = blobmsg_open_table(b, "NULL");
				blobmsg_add_string(b, "hostname", port[i].client[j].hostname);
				blobmsg_add_string(b, "ipaddr", port[i].client[j].hostaddr);
				blobmsg_add_string(b, "macaddr", port[i].client[j].macaddr);
				blobmsg_close_table(b, h);
			}
			blobmsg_close_array(b, c);
			s = blobmsg_open_table(b, "statistics");
			blobmsg_add_u64(b, "rx_packets", port[i].stat.rx_packets);
			blobmsg_add_u64(b, "rx_bytes", port[i].stat.rx_bytes);
			blobmsg_add_u64(b, "rx_errors", port[i].stat.rx_errors);				
			blobmsg_add_u64(b, "tx_packets", port[i].stat.tx_packets);
			blobmsg_add_u64(b, "tx_bytes", port[i].stat.tx_bytes);
			blobmsg_add_u64(b, "tx_errors", port[i].stat.tx_errors);
			blobmsg_close_table(b, s);
			blobmsg_close_table(b, t);
		}
	}
}

static void
network_dump_leases(struct blob_buf *b, char *leasenet)
{
	void *t;
	char leasenum[16];
	int i;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (!clients[i].exists)
			break;
		if (clients[i].dhcp && !strcmp(clients[i].network, leasenet)) {
			sprintf(leasenum, "lease-%d", i + 1);
			t = blobmsg_open_table(b, leasenum);
			blobmsg_add_string(b, "leaseno", clients[i].leaseno);
			blobmsg_add_string(b, "hostname", clients[i].hostname);
			blobmsg_add_string(b, "ipaddr", clients[i].hostaddr);
			blobmsg_add_string(b, "macaddr", clients[i].macaddr);
			blobmsg_add_string(b, "device", clients[i].device);
			blobmsg_add_u8(b, "connected", clients[i].connected);
			blobmsg_close_table(b, t);
		}
	}
}

static void
host_dump_status(struct blob_buf *b, char *addr, bool byIP)
{
	int i;

	if(byIP) {
		for (i=0; clients[i].exists; i++)
			if(!strcmp(clients[i].hostaddr, addr)) {
				blobmsg_add_string(b, "hostname", clients[i].hostname);
				blobmsg_add_string(b, "macaddr", clients[i].macaddr);
				blobmsg_add_string(b, "network", clients[i].network);
				blobmsg_add_string(b, "device", clients[i].device);
				blobmsg_add_u8(b, "connected", clients[i].connected);
				blobmsg_add_u8(b, "wireless", clients[i].wireless);
				if(clients[i].connected)
					blobmsg_add_u32(b, "active_cons", details[i].connum);
				if(clients[i].wireless) {
					blobmsg_add_string(b, "wdev", clients[i].wdev);
					blobmsg_add_u32(b, "idle", details[i].idle);
					blobmsg_add_u32(b, "in_network", details[i].in_network);
					blobmsg_add_u64(b, "tx_bytes", details[i].tx_bytes);
					blobmsg_add_u64(b, "rx_bytes", details[i].rx_bytes);
					blobmsg_add_u32(b, "tx_rate", details[i].tx_rate);
					blobmsg_add_u32(b, "rx_rate", details[i].rx_rate);
				}
				break;
			}
	}
	else {
		for (i=0; clients[i].exists; i++)
			if(!strcasecmp(clients[i].macaddr, addr)) {
				blobmsg_add_string(b, "hostname", clients[i].hostname);
				blobmsg_add_string(b, "ipaddr", clients[i].hostaddr);
				blobmsg_add_string(b, "network", clients[i].network);
				blobmsg_add_string(b, "device", clients[i].device);
				blobmsg_add_u8(b, "connected", clients[i].connected);
				blobmsg_add_u8(b, "wireless", clients[i].wireless);
				if(clients[i].connected)
					blobmsg_add_u32(b, "active_cons", details[i].connum);
				if(clients[i].wireless) {
					blobmsg_add_string(b, "wdev", clients[i].wdev);
					blobmsg_add_u32(b, "idle", details[i].idle);
					blobmsg_add_u32(b, "in_network", details[i].in_network);
					blobmsg_add_u64(b, "tx_bytes", details[i].tx_bytes);
					blobmsg_add_u64(b, "rx_bytes", details[i].rx_bytes);
					blobmsg_add_u32(b, "tx_rate", details[i].tx_rate);
					blobmsg_add_u32(b, "rx_rate", details[i].rx_rate);
				}
				break;
			}
	}
}

static int
quest_router_specific(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__QUEST_MAX];

	blobmsg_parse(quest_policy, __QUEST_MAX, tb, blob_data(msg), blob_len(msg));

	if (!(tb[QUEST_NAME]) || (strcmp(blobmsg_data(tb[QUEST_NAME]), "system") && strcmp(blobmsg_data(tb[QUEST_NAME]), "memory")
		&& strcmp(blobmsg_data(tb[QUEST_NAME]), "keys") && strcmp(blobmsg_data(tb[QUEST_NAME]), "specs")))
		return UBUS_STATUS_INVALID_ARGUMENT;

	blob_buf_init(&bb, 0);

	if (!strcmp(blobmsg_data(tb[QUEST_NAME]), "system"))
		router_dump_system_info(&bb, false);
	else if (!strcmp(blobmsg_data(tb[QUEST_NAME]), "memory"))
		router_dump_memory_info(&bb, false);
	else if (!strcmp(blobmsg_data(tb[QUEST_NAME]), "keys"))
		router_dump_keys(&bb, false);
	else if (!strcmp(blobmsg_data(tb[QUEST_NAME]), "specs"))
		router_dump_specs(&bb, false);

	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_info(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__QUEST_MAX];

	blobmsg_parse(quest_policy, __QUEST_MAX, tb, blob_data(msg), blob_len(msg));

	blob_buf_init(&bb, 0);
	router_dump_system_info(&bb, true);
	router_dump_memory_info(&bb, true);
	router_dump_keys(&bb, true);
	router_dump_specs(&bb, true);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_networks(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__QUEST_MAX];

	blobmsg_parse(quest_policy, __QUEST_MAX, tb, blob_data(msg), blob_len(msg));

	blob_buf_init(&bb, 0);
	router_dump_networks(&bb);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_clients(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__QUEST_MAX];

	blobmsg_parse(quest_policy, __QUEST_MAX, tb, blob_data(msg), blob_len(msg));

	blob_buf_init(&bb, 0);
	router_dump_clients(&bb);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_connected_clients(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__QUEST_MAX];

	blobmsg_parse(quest_policy, __QUEST_MAX, tb, blob_data(msg), blob_len(msg));

	blob_buf_init(&bb, 0);
	router_dump_connected_clients(&bb);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_network_clients(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__NETWORK_MAX];
	bool nthere = false;
	int i;

	blobmsg_parse(network_policy, __NETWORK_MAX, tb, blob_data(msg), blob_len(msg));

	if (!(tb[NETWORK_NAME]))
		return UBUS_STATUS_INVALID_ARGUMENT;

	for (i=0; network[i].is_lan; i++)
		if(!strcmp(network[i].name, blobmsg_data(tb[NETWORK_NAME]))) {
			nthere = true;
			break;
		}

	if (!(nthere))
		return UBUS_STATUS_INVALID_ARGUMENT;

	blob_buf_init(&bb, 0);
	router_dump_network_clients(&bb, blobmsg_data(tb[NETWORK_NAME]));
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_connected_clients6(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__QUEST_MAX];

	blobmsg_parse(quest_policy, __QUEST_MAX, tb, blob_data(msg), blob_len(msg));

	blob_buf_init(&bb, 0);
	router_dump_connected_clients6(&bb);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_clients6(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__QUEST_MAX];

	blobmsg_parse(quest_policy, __QUEST_MAX, tb, blob_data(msg), blob_len(msg));

	blob_buf_init(&bb, 0);
	router_dump_clients6(&bb);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_stas(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__QUEST_MAX];

	blobmsg_parse(quest_policy, __QUEST_MAX, tb, blob_data(msg), blob_len(msg));

	blob_buf_init(&bb, 0);
	router_dump_stas(&bb);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_wireless_stas(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__WL_MAX];
	char lookup[8];
	bool nthere = false;
	int i;

	blobmsg_parse(wl_policy, __WL_MAX, tb, blob_data(msg), blob_len(msg));

	if (!(tb[RADIO_NAME]) && !(tb[VIF_NAME]))
		return UBUS_STATUS_INVALID_ARGUMENT;

	if (tb[RADIO_NAME] && strchr(blobmsg_data(tb[RADIO_NAME]), '.'))
		return UBUS_STATUS_INVALID_ARGUMENT;

	memset(lookup, '\0', 8);
	if (tb[VIF_NAME])
		strcpy(lookup, blobmsg_data(tb[VIF_NAME]));
	else
		strcpy(lookup, blobmsg_data(tb[RADIO_NAME]));

	for (i=0; wireless[i].device; i++)
		if(!strcmp(wireless[i].vif, lookup)) {
			nthere = true;
			break;
		}

	if (!(nthere))
		return UBUS_STATUS_INVALID_ARGUMENT;


	blob_buf_init(&bb, 0);
	if (tb[RADIO_NAME])
		router_dump_wireless_stas(&bb, blobmsg_data(tb[RADIO_NAME]), false);
	else
		router_dump_wireless_stas(&bb, blobmsg_data(tb[VIF_NAME]), true);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_usbs(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__QUEST_MAX];

	blobmsg_parse(quest_policy, __QUEST_MAX, tb, blob_data(msg), blob_len(msg));

	blob_buf_init(&bb, 0);
	router_dump_usbs(&bb);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_network_leases(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__NETWORK_MAX];
	bool nthere = false;
	int i;

	blobmsg_parse(network_policy, __NETWORK_MAX, tb, blob_data(msg), blob_len(msg));

	if (!(tb[NETWORK_NAME]))
		return UBUS_STATUS_INVALID_ARGUMENT;

	for (i=0; network[i].is_lan; i++)
		if(!strcmp(network[i].name, blobmsg_data(tb[NETWORK_NAME])))
			nthere = true;

	if (!(nthere))
		return UBUS_STATUS_INVALID_ARGUMENT;

	blob_buf_init(&bb, 0);
	network_dump_leases(&bb, blobmsg_data(tb[NETWORK_NAME]));
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_router_ports(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__NETWORK_MAX];
	bool nthere = false;
	int i;

	blobmsg_parse(network_policy, __NETWORK_MAX, tb, blob_data(msg), blob_len(msg));
	
	if (!(tb[NETWORK_NAME]))
		return UBUS_STATUS_INVALID_ARGUMENT;
		
	for (i=0; network[i].exists; i++) {
		if(!strcmp(network[i].name, blobmsg_data(tb[NETWORK_NAME])))
			if(!strcmp(network[i].type, "bridge")) {
			nthere = true;
			break;
		}
	}

	if (!(nthere))
		return UBUS_STATUS_INVALID_ARGUMENT;
	
	blob_buf_init(&bb, 0);
	router_dump_ports(&bb, blobmsg_data(tb[NETWORK_NAME]));
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_host_status(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	struct blob_attr *tb[__HOST_MAX];

	blobmsg_parse(host_policy, __HOST_MAX, tb, blob_data(msg), blob_len(msg));

	if (!(tb[IP_ADDR]) && !(tb[MAC_ADDR]))
		return UBUS_STATUS_INVALID_ARGUMENT;

	blob_buf_init(&bb, 0);
	if (tb[IP_ADDR])
		host_dump_status(&bb, blobmsg_data(tb[IP_ADDR]), true);
	else
		host_dump_status(&bb, blobmsg_data(tb[MAC_ADDR]), false);
	ubus_send_reply(ctx, req, bb.head);

	return 0;
}

static int
quest_reload(struct ubus_context *ctx, struct ubus_object *obj,
		  struct ubus_request_data *req, const char *method,
		  struct blob_attr *msg)
{
	dump_hostname(&router);
	load_networks();
	load_wireless();
	return 0;
}

static struct ubus_method router_object_methods[] = {
	UBUS_METHOD_NOARG("info", quest_router_info),
	UBUS_METHOD("quest", quest_router_specific, quest_policy),
	UBUS_METHOD_NOARG("networks", quest_router_networks),
	UBUS_METHOD_NOARG("dslstats", dslstats_rpc), 
	UBUS_METHOD("client", quest_router_network_clients, network_policy),
	UBUS_METHOD_NOARG("clients", quest_router_clients),
	UBUS_METHOD_NOARG("clients6", quest_router_clients6),
	UBUS_METHOD_NOARG("connected", quest_router_connected_clients),
	UBUS_METHOD_NOARG("connected6", quest_router_connected_clients6),
	UBUS_METHOD("sta", quest_router_wireless_stas, wl_policy),
	UBUS_METHOD_NOARG("stas", quest_router_stas),
	UBUS_METHOD("ports", quest_router_ports, network_policy),
	UBUS_METHOD("leases", quest_network_leases, network_policy),
	UBUS_METHOD("host", quest_host_status, host_policy),
	UBUS_METHOD_NOARG("usb", quest_router_usbs),
	UBUS_METHOD_NOARG("reload", quest_reload),
};

static struct ubus_object_type router_object_type =
	UBUS_OBJECT_TYPE("system", router_object_methods);

static struct ubus_object router_object = {
	.name = "router",
	.type = &router_object_type,
	.methods = router_object_methods,
	.n_methods = ARRAY_SIZE(router_object_methods),
};

static void
quest_ubus_add_fd(void)
{
	ubus_add_uloop(ctx);
	system_fd_set_cloexec(ctx->sock.fd);
}

static void
quest_ubus_reconnect_timer(struct uloop_timeout *timeout)
{
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

static void
quest_ubus_connection_lost(struct ubus_context *ctx)
{
	quest_ubus_reconnect_timer(NULL);
}

static void
quest_add_object(struct ubus_object *obj)
{
	int ret = ubus_add_object(ctx, obj);

	if (ret != 0)
		fprintf(stderr, "Failed to publish object '%s': %s\n", obj->name, ubus_strerror(ret));
}

static int
quest_ubus_init(const char *path)
{
	uloop_init();
	ubus_path = path;

	ctx = ubus_connect(path);
	if (!ctx)
		return -EIO;

	printf("connected as %08x\n", ctx->local_id);
	ctx->connection_lost = quest_ubus_connection_lost;
	quest_ubus_add_fd();

	quest_add_object(&router_object);

	return 0;
}

void *dump_router_info(void *arg)
{
	int lpcnt = 0;

	jiffy_counts_t cur_jif, prev_jif;
	
	
	init_db_hw_config();
	load_networks();
	load_wireless();
	dump_keys(&keys);
	dump_specs(&spec);
	dump_static_router_info(&router);
	dump_hostname(&router);
	while (true) {
		dump_sysinfo(&router, &memory);
		dump_cpuinfo(&router, &prev_jif, &cur_jif);
		populate_clients();
		populate_clients6();
		get_jif_val(&prev_jif);
		usleep(sleep_time);
		recalc_sleep_time(false, 0);
		get_jif_val(&cur_jif);
		lpcnt++;
		if (lpcnt == 20) {
			lpcnt = 0;
			memset(clients, '\0', sizeof(clients));
			memset(clients6, '\0', sizeof(clients6));
		}
	}

	return NULL;
}

int main(int argc, char **argv)
{
	int pt;

	if (quest_ubus_init(NULL) < 0) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return 1;
	}
        if ((pt = pthread_create(&(tid[0]), NULL, &dump_router_info, NULL) != 0)) {
		fprintf(stderr, "Failed to create thread\n");
		return 1;
	}
	uloop_run();
	ubus_free(ctx);	

	return 0;
}

