#include <point.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/net/net-if.h>
#include <zephyr/net/net-ip.h>
#include <zephyr/net/net-core.h>

#define STACKSIZE 1024
#define PRIORITY 1

// debug levels
LOG_MODULE_REGISTER(z_network, LOG_LEVEL_DBG);

// sets up point channel subscription
ZBUS_CHAN_DECLARE(point_chan);
ZBUS_MSG_SUBSCRIBER_DEFINE(network_sub);
ZBUS_CHAN_ADD_OBS(point_chan, network_sub, 3);

// defines point storage
struct networkInfo {
    int static_ip_state;
    char static_ip_address[];
    char static_ip_gateway[];
    char static_ip_netmask[];
}

// stores our most important points
static struct networkInfo buffer = {-1,"","",""};

// this configures our static IP
void configure_static_ip(void) {

    struct net_if *iface = net_if_get_default();
    struct in_addr address;
    struct in_addr netmask;
    struct in_addr gateway;

    net_addr_pton(AF_INET, buffer.static_ip_address, &address);
    net_addr_pton(AF_INET, buffer.static_ip_gateway, &gateway);
    net_addr_pton(AF_INET, buffer.static_ip_netmask, &netmask);

    net_if_ipv4_addr_rm_all(iface);

    net_if_ipv4_addr_add(iface, &address, NET_ADDR_MANUAL, 0);
    net_if_ipv4_set_netmask(iface, &netmask);
    net_if_ipv4_set_gw(iface, &gateway);
    net_dhcpv4_stop(iface);

}

// this just sets DHCP (back) up
void configure_dhcp(void) {

    struct net_if *iface = net_if_get_default();
    struct in_addr empty_netmask = {0};
    struct in_addr empty_gateway = {0};

    net_if_ipv4_addr_rm_all(iface);
    net_if_ipv4_set_netmask(iface, &empty_netmask);
    net_if_ipv4_set_gw(iface, &empty_gateway);

    net_dhcpv4_start(iface);

}

// this is run every time we receive a new point, but only runs if we have non-null values in buffer
// the value of this is if someone clicks staticIP, but does not enter gateway, address,
// or netmask values, this will not turn off DHCP, and thus will change nothing.
// this prevents the interface from going down prematurely
// TODO: Account for bad configurations, give a revert option somehow (how?)
static int network_init_start() {

    if (buffer.static_ip_state == 1 &&
        strcmp(buffer.static_ip_address, "") != 0 &&
        strcmp(buffer.static_ip_netmask, "") != 0 &&
        strcmp(buffer.static_ip_gateway, "") != 0) {

        configure_static_ip();

    } else if (buffer.static_ip_state == 0) {

        configure_dhcp();

    }

    return 0;

}

// Main thread function definition
// Watches for points, starts network initialization when new points are received
void network_thread(void *arg1, void *arg2, void *arg3) {

    LOG_INF("Network Starter Thread")
    
    point p;

    const struct zbus_channel *chan;

    while(!zbus_sub_wait_msg(&network_sub, &chan, &p, K_FOREVER)) {
        if (chan == &point_chan) {
            if (chan == &point_chan) {
                if (strcmp(p.type, POINT_TYPE_STATICIP) == 0) {
                    // change to static IP boolean received
                    int ip_state = atoi(p.data);
                    buffer.static_ip_state = ip_state;
                    network_init_start();
                } else if (strcmp(p.type, POINT_TYPE_ADDRESS) == 0) {
                    // change to static IP address received
                    char address[] = p.data;
                    buffer.static_ip_address = address;
                    network_init_start()    
                } else if (strcmp(p.type, POINT_TYPE_NETMASK) == 0) {
                    // change to static IP netmask received
                    char netmask[] = p.data;
                    buffer.static_ip_netmask = netmask;
                    network_init_start();
                } else if (strcmp(p.type, POINT_TYPE_GATEWAY) == 0) {
                    // change to static IP gateway received
                    char gateway[] = p.data;
                    buffer.static_ip_gateway = gateway;
                    network_init_start();
                }
            }
        }
    }
}

// define and start the thread
K_THREAD_DEFINE(network, STACKSIZE, network_thread, NULL, NULL, NULL, PRIORITY, 0, 0);



