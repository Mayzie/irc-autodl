#include "client.h"

static int socket_connect(const int sockid, const struct sockaddr *address, socklen_t address_len) {
	printf("In socket_connect(%d).\n", sockid);
	do {
		if(connect(sockid, address, address_len) == -1) {
			if(errno != EINTR)
				return -1;
		} else 
			break;
	} while(true);

	return 0;
}

int irc_send(const struct irc *irc, const char *msg) {
	int length = strlen(msg);
	char fixed_msg[length + 2];
	strcpy(fixed_msg, msg);
	strcat(fixed_msg, CRLF);
	return send(irc->sockid, fixed_msg, length + 2, 0);
}

void irc_recv_privmsg(struct irc *irc, char *from, char *to, char *message) {
	printf("%s <%s> %s\n", to, from, message);
}

/*
 * pos_crlf = position of CRLF in message.
 */
void irc_parse_raw(struct irc *irc, char *pos_crlf) {
	int length = pos_crlf - irc->last_msg.message;
	/* Replace \r\n (CRLF) with \0\0 */
	*pos_crlf = '\0';
	*(pos_crlf + 1) = '\0';

	if(irc->last_msg.message[0] == 0x3a) {
		char *nickfrom = strtok(irc->last_msg.message, "!");  // Nickname the message came from (if any)
		if(nickfrom != NULL) {
			nickfrom++;  // Ignore the colon ':'
			char *message = strchr(nickfrom + strlen(nickfrom) + 1, 0x20);
			if(message != NULL) {
				message++;  // Ignore the space ' '
				if(strncmp("PRIVMSG ", message, 8) == 0) {
					char *nickto = strtok(message + 8, " ");
					message = strtok(NULL, "\0");
					if(*message == 0x3a) {
						message++;
						irc_recv_privmsg(irc, nickfrom, nickto, message);
					} else {
						goto CLEANUP;  // Malformed IRC message.
					}
				}
			}
		}
	} else if(strncmp("PING ", irc->last_msg.message, 5) == 0) {
		char reply[length + 1];
		strcpy(reply, "PONG ");
		strcat(reply, irc->last_msg.message + 5);
		irc_send(irc, reply);
	}

CLEANUP:
	irc->last_msg.length -= length + 2;
	irc->last_msg.message = memmove(irc->last_msg.message, pos_crlf + 2, irc->last_msg.length);
	fflush(stdout);
}

/**
 * Receives a RAW IRC message from the socket. Only call when you know that there is data
 * waiting in the stream (e.g. via select(), poll(), or epoll()), as it WILL block
 * otherwise.
 *
 * This function has no protection against non-conforming IRC servers. Ensure you are
 * connecting to an IRC server that at least conforms to the RFC1459 specification.
 *
 * Returns false on client shutdown.
 **/
static char *strnstr(s, find, slen)
	const char *s;
	const char *find;
	size_t slen;
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

bool irc_receive_raw(struct irc *irc) {
	if(irc->sockid >= 0) {
		if(irc->last_msg.length != 0) {
			char *pos = strnstr(irc->last_msg.message, CRLF, irc->last_msg.length);
			if(pos != NULL) {
				irc_parse_raw(irc, pos);
				return true;
			}
		}
		
		int recv_msg_len = 0;
		do {
			recv_msg_len = recv(irc->sockid, irc->last_msg.message + irc->last_msg.length,
			                    IRC_TOTAL_MSG_LEN - irc->last_msg.length, 0);
			if(recv_msg_len == 0) {
				irc->sockid = -1;
				return false;
			} else if(recv_msg_len == -1) {
				if(errno == EINTR || errno == ETIMEDOUT)
					continue;
				else if(errno == ENOTCONN) {
					fprintf(stderr, "%s@%s:%s not connected. Unable to receive data."
						        " Disabling socket.\n", irc->nickname, irc->hostname,
						irc->port);

					return false;
				} 
			} else
				break;  // Successfully retrieved data from the stream.
		} while(true);
		irc->last_msg.length += recv_msg_len;
		
		char *pos = strnstr(irc->last_msg.message, CRLF, irc->last_msg.length);
		if(pos == NULL) {
			return false;  // There must exist at least one CRLF delimiter in the message.
		} else {
			irc_parse_raw(irc, pos);
			return true;
		}
	} else
		return false;
}

bool irc_join(struct irc *irc, const char *channel) {
	char *msg = malloc(strlen("JOIN ") + strlen(channel) + 1);
	strcpy(msg, "JOIN ");
	strcat(msg, channel);	

	bool ret = irc_send(irc, msg);

	free(msg);
	return ret;
}

struct irc *irc_init(const char *hostname, const char *password, const char *port) {
	struct irc *ret = malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));

	ret->sockid = 0;

	ret->nickname = NULL;
	ret->username = NULL;
	ret->realname = NULL;

	ret->hostname = hostname;
	ret->password = password;
	ret->port = port;

	ret->channels = malloc(sizeof(*ret->channels));
	ret->channels->channel = NULL;
	ret->channels->next = NULL;

	ret->last_msg.message = malloc(sizeof(char) * IRC_TOTAL_MSG_LEN);
	ret->last_msg.length = 0;

	return ret;
}

void irc_free(struct irc *irc) {
	if(irc != NULL) {
		if(irc->nickname != NULL) {
			free(irc->nickname);
			irc->nickname = NULL;
		}

		if(irc->username != NULL) {
			free(irc->username);
			irc->username = NULL;
		}

		if(irc->realname != NULL) {
			free(irc->realname);
			irc->realname == NULL;
		}

		// Leave it up to the caller to free these
		/*
		if(irc->hostname != NULL) {
			free((char *) irc->hostname);
			irc->hostname = NULL;
		}

		if(irc->password != NULL) {
			free((char *) irc->password);
			irc->password = NULL;
		}

		if(irc->port != NULL) {
			free((char *) irc->port);
			irc->port = NULL;
		}
		*/

		if(irc->last_msg.message != NULL) {
			free(irc->last_msg.message);
			irc->last_msg.message = NULL;
		}

		if(irc->channels != NULL) {
			// ToDo: Complete section/free linked list
			free(irc->channels);
			irc->channels = NULL;
		}

		free(irc);
		irc = NULL;
	}
}

int irc_connect(struct irc *irc) {
	int status = 0;

//	struct addrinfo *hints = malloc(sizeof(*hints));
	struct addrinfo hints;
	struct addrinfo *results;
	struct addrinfo *active;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	printf("Attempting to connect to %s:%s\n", irc->hostname, irc->port);
	status = getaddrinfo(irc->hostname, irc->port, &hints, &results);
	if(status != 0) {
		fprintf(stdout, "Unable to connect to IRC server '%s:%s'\n", irc->hostname, irc->port);
		goto CLEANUP;
	}

	printf("Entering loop.\n");
	for(active = results; active != NULL; active = active->ai_next) {
		irc->sockid = socket(active->ai_family, active->ai_socktype, active->ai_protocol);
		if(irc->sockid == -1)
			continue;
		
		if(socket_connect(irc->sockid, active->ai_addr, active->ai_addrlen) != 0)
			continue;

		break;
	}

CLEANUP:
	if(active == NULL) {
		irc->sockid = -1;
	}

//	freeaddrinfo(hints);
	freeaddrinfo(results);

	return irc->sockid;
}
