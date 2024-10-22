#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <stdio.h>
#include <string.h>

#define SNMP_PORT 161

int snmp_send(char *ipaddr, const void *message, size_t len)
{
	int sock;
	struct sockaddr_in addr;
	int ret;

	// Create UDP socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		printf("Failed to create socket\n");
		return -1;
	}

	// Set up server address
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SNMP_PORT);
	inet_pton(AF_INET, ipaddr, &addr.sin_addr);

	// Send data to SNMP port
	ret = sendto(sock, message, len, 0, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		printf("Failed to send data\n");
		close(sock);
		return -1;
	}

	printf("Sent %d bytes to SNMP server\n", ret);

	// Close socket
	close(sock);

	return 0;
}
