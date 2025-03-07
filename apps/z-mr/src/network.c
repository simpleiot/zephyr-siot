#include <point.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/reboot.h>

#define STACKSIZE                1024
#define PRIORITY                 1
#define BROADCAST_PORT           64209
#define NTP_TIMEOUT              5000
#define NTP_SERVER_BUFFER_LENGTH 3

// debug levels
LOG_MODULE_REGISTER(z_network, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan); // sets up point channel subscription
ZBUS_CHAN_DECLARE(ticker_chan); // this will be used for our timer


ZBUS_MSG_SUBSCRIBER_DEFINE(network_sub);
ZBUS_CHAN_ADD_OBS(point_chan, network_sub, 3);
ZBUS_CHAN_ADD_OBS(ticker_chan, network_sub, 4);

static struct net_mgmt_event_callback dhcp_cb;

static void dhcp_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			 struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	char buf[NET_IPV4_ADDR_LEN];
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	struct net_if_addr *if_addr = &ipv4->unicast[0].ipv4;

	if (if_addr->addr_type != NET_ADDR_DHCP) {
		return;
	}

	LOG_INF("DHCP address received: %s",
		net_addr_ntop(AF_INET, &if_addr->address.in_addr, buf, sizeof(buf)));
}

static struct net_mgmt_event_callback mgmt_cb;

static void net_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			      struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected\n");
	}
}

// defines point storage
struct networkInfo {
	int static_ip_state;
	char static_ip_address[20];
	char static_ip_gateway[20];
	char static_ip_netmask[20];
};

// stores our most important points
static struct networkInfo buffer = {0, "", "", ""};

struct ntpInfo {
	char ntp_server_address[40];
};

struct sntp_time timestamp;


static struct ntpInfo ntp_servers[NTP_SERVER_BUFFER_LENGTH];
static struct ntpInfo usedServer[1];

void remove_all_ipv4_addresses(struct net_if *iface)
{
	int index = net_if_get_by_iface(iface);
	struct in_addr addr;
	int i;
	LOG_DBG("removing all ipv4 interfaces");
	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (net_if_ipv4_addr_lookup_by_index(&addr) >= 0) {
			net_if_ipv4_addr_rm_by_index(index, &addr);
		}
	}
}

void broadcast_new_ip(const char *new_ip)
{
	int sock;
	struct sockaddr_in broadcast_addr;
	int optval = 1;

	// Create a UDP socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create socket");
		return;
	}

	// Enable broadcasting on the socket
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
		LOG_ERR("Failed to set socket options");
		close(sock);
		return;
	}

	// Configure the broadcast address
	memset(&broadcast_addr, 0, sizeof(broadcast_addr));
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_port = htons(BROADCAST_PORT);
	inet_pton(AF_INET, "255.255.255.255", &broadcast_addr.sin_addr);

	// Send the new IP address as a broadcast message
	if (sendto(sock, new_ip, strlen(new_ip), 0, (struct sockaddr *)&broadcast_addr,
		   sizeof(broadcast_addr)) < 0) {
		LOG_ERR("Failed to send broadcast message");
	} else {
		LOG_DBG("Broadcasted new IP: %s", new_ip);
	}

	close(sock);
}

void configure_ntp_static(void)
{
	int i;

	for (i = 0; i < NTP_SERVER_BUFFER_LENGTH; i++) {

		if (ntp_servers[i].ntp_server_address[0] != '\0' && usedServer[0].ntp_server_address[0] != '\0') {

			int rc = sntp_simple(ntp_servers[i].ntp_server_address, NTP_TIMEOUT, &timestamp);
			if (rc == 0) {
				strncpy(usedServer[0].ntp_server_address, ntp_servers[i].ntp_server_address, sizeof(usedServer[0].ntp_server_address) - 1);
				LOG_DBG("NTP Config Successful!");
				return;
			}

		} else {

			// there was an error
			LOG_ERR("NTP CONFIG FAILED");
		}
	}
}

int configure_static_ip(bool network_started)
{
	struct net_if *iface = net_if_get_default();
	struct in_addr address, netmask, gateway;

	LOG_DBG("Configuring static IP");

	net_dhcpv4_stop(iface);
	remove_all_ipv4_addresses(iface);

	LOG_DBG("Removed IP");
	LOG_DBG("One last confirmation of value: %s", buffer.static_ip_address);

	if (net_addr_pton(AF_INET, buffer.static_ip_address, &address) < 0) {
		LOG_ERR("Invalid address: %s", buffer.static_ip_address);
		return -EINVAL;
	}
	if (net_addr_pton(AF_INET, buffer.static_ip_netmask, &netmask) < 0) {
		LOG_ERR("Invalid netmask: %s", buffer.static_ip_netmask);
		return -EINVAL;
	}

	net_if_ipv4_addr_add(iface, &address, NET_ADDR_MANUAL, 0);
	net_if_ipv4_set_netmask_by_addr(iface, &address, &netmask);

	if (strlen(buffer.static_ip_gateway) > 0) {
		if (net_addr_pton(AF_INET, buffer.static_ip_gateway, &gateway) < 0) {
			LOG_ERR("Invalid gateway: %s", buffer.static_ip_gateway);
			return -EINVAL;
		}
		net_if_ipv4_set_gw(iface, &gateway);
	}

	LOG_DBG("Static IP configured successfully");

	// configure_ntp_static();

	if (network_started) {
		sys_reboot(SYS_REBOOT_COLD);
	}

	return 0;
}

// this just sets DHCP (back) up
void configure_dhcp(bool network_started)
{

	LOG_DBG("Configuring DHCP");

	struct net_if *iface = net_if_get_default();

	net_mgmt_init_event_callback(&dhcp_cb, dhcp_handler, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&dhcp_cb);

	net_if_down(iface);

	remove_all_ipv4_addresses(iface);

	net_dhcpv4_start(iface);

	net_if_up(iface);


	LOG_DBG("interface started");

	if (network_started) {
		sys_reboot(SYS_REBOOT_COLD);
	}
}

// this will run once we have stopped receiving that batch of points
// TODO: How do we revert bad configs?
static int network_init_start(bool network_started)
{

	LOG_DBG("Buffer static IP state: %d", buffer.static_ip_state);
	LOG_DBG("Buffer static IP address: %s", buffer.static_ip_address);
	LOG_DBG("Buffer static IP netmask: %s", buffer.static_ip_netmask);
	LOG_DBG("Buffer static IP gateway: %s", buffer.static_ip_gateway);

	if (buffer.static_ip_state == 1 && strcmp(buffer.static_ip_address, "") != 0 &&
	    strcmp(buffer.static_ip_netmask, "") != 0) {

		configure_static_ip(network_started);

	} else {

		configure_dhcp(network_started);
	}

	return 0;
}

// Main thread function definition
// Watches for points, starts network initialization when new points are received
void network_thread(void *arg1, void *arg2, void *arg3)
{

	LOG_INF("Network Starter Thread");

	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);

	point p;

	const struct zbus_channel *chan;

	int status_tick = 0;
	int sntp_tick = 0;
	bool network_started = false;

	while (!zbus_sub_wait_msg(&network_sub, &chan, &p, K_FOREVER)) {
		if (chan == &point_chan) {
			if (strcmp(p.type, POINT_TYPE_STATICIP) == 0) {
				LOG_DBG("StaticIP state received");
				// change to static IP boolean received
				int ip_state = point_get_int(&p);
				buffer.static_ip_state = ip_state;
				status_tick = 0;
			} else if (strcmp(p.type, POINT_TYPE_ADDRESS) == 0) {
				// change to static IP address received
				LOG_DBG("StaticIP address received");
				strncpy(buffer.static_ip_address, p.data,
					sizeof(buffer.static_ip_address) - 1);
				status_tick = 0;
			} else if (strcmp(p.type, POINT_TYPE_NETMASK) == 0) {
				LOG_DBG("StaticIP netmask received");
				// change to static IP netmask received
				strncpy(buffer.static_ip_netmask, p.data,
					sizeof(buffer.static_ip_netmask) - 1);
				status_tick = 0;
			} else if (strcmp(p.type, POINT_TYPE_GATEWAY) == 0) {
				LOG_DBG("StaticIP gateway received");
				// change to static IP gateway received
				strncpy(buffer.static_ip_gateway, p.data,
					sizeof(buffer.static_ip_gateway) - 1);
				status_tick = 0;
			} else if (strcmp(p.type, POINT_TYPE_NTP) == 0) {
				LOG_DBG("NTP Point Received");
				// check what key the NTP server is, as this will tell us
				// what network we should configure.
				int server_priority = atoi(p.key);
				strncpy(ntp_servers[server_priority].ntp_server_address,
					p.data,
					sizeof(ntp_servers[server_priority]
								.ntp_server_address) -
						1);
				// configure_ntp_static();
				status_tick = 0;
				sntp_tick = 0;
			}
		} else if (chan == &ticker_chan) {
			status_tick++;
			sntp_tick++;
			if (status_tick == 10) {
				network_init_start(network_started);
				network_started = true;
			}
			if (sntp_tick >= 7200) {
				sntp_simple(usedServer[0].ntp_server_address, NTP_TIMEOUT, &timestamp);
				sntp_tick=0;
			}


		}
	}
}

// define and start the thread
K_THREAD_DEFINE(network, STACKSIZE, network_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
