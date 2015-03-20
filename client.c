#include "client.h"

void *get_address(struct sockaddr *addr) {
	if(addr->sa_family == AF_INET)
		return &(((struct sockaddr_in *) addr)->sin_addr);
	else if (addr->sa_family == AF_INET6)
		return &(((struct sockaddr_in6 *) addr)->sin6_addr);
	else
		return 0;	// NULL
}

/**
 * establish_irc_connection() - Create a new socket connection to an IRC server
 * @url:		IRC server hostname to connect to (e.g. irc.freenode.net)
 * @port:		Port (as a string) to connect to
 * @ip_addr_type:	IP address type (AF_INET [IPv4], AF_INET6 [IPv6],
 * 			AF_UNSPEC [either])
 *
 * This function establishes a new socket connection to the specified IRC server
 * with hostname @url and on port @port.
 *
 * The caller must specify the IP address family they want the connection on.
 * This can be forced, or if it is irrelevant, then AF_UNSPEC.
 *
 * Return: -1 on failure, and set the global POSIX variable, errno.
 * Otherwise, a valid socket file descriptor will be returned.
 */
int establish_irc_connect(char* url, char* port, int ip_addr_type) {
	int ret;

	struct addrinfo type;
	struct addrinfo info;

	type.ai_family = AF_UNSPEC;	// Do not worry abou
}
