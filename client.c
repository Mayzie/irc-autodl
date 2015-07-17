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

int irc_send_message(struct irc *irc, char *to, char *type, char *message) {
	size_t type_len = strlen(type);
	size_t to_len = strlen(to);
	size_t message_len = strlen(message);

	char fixed_msg[type_len + 1 + to_len + 2 + message_len];  // type + " " + to + " :" + message
	strcpy(fixed_msg, type);
	strcat(fixed_msg, " ");
	strcat(fixed_msg, to);
	strcat(fixed_msg, " :");
	strcat(fixed_msg, message);

	return irc_send(irc, fixed_msg);
}

int irc_send_notice(struct irc *irc, char *to, char *message) {
	return irc_send_message(irc, to, "NOTICE", message);
}

int irc_send_privmsg(struct irc *irc, char *to, char *message) {
	return irc_send_message(irc, to, "PRIVMSG", message);
}

int irc_send_ctcpreply_raw(struct irc *irc, char *to, char *ctcp_type, char *reply) {
	size_t ctcp_type_len = strlen(ctcp_type);
	size_t ctcp_reply_len = strlen(reply);

	char reply_str[1 + ctcp_type_len + 1 + ctcp_reply_len + 1];  // '\0x01' + ctcp_type + ' ' + reply  '\0x01'
	strcpy(reply_str, "\001");
	strcat(reply_str, ctcp_type);
	strcat(reply_str, " ");
	strcat(reply_str, reply);
	strcat(reply_str, "\001");

	return irc_send_notice(irc, to, reply_str);
}

int irc_send_ctcpreply_version(struct irc *irc, char *to) {
	size_t total_len = 0;
	total_len += CLIENT_NAME_LEN + 1;  // "MayzieBot "
	total_len += CLIENT_VERSION_LEN;  // "0.01"

#if defined(_POSIX_VERSION)
#if _POSIX_VERSION >= 200112L
	struct utsname os_version_struct;
	if(uname(&os_version_struct) >= 0) {
		size_t sysname_len = strlen(os_version_struct.sysname);
		size_t release_len = strlen(os_version_struct.release);
		size_t version_len = strlen(os_version_struct.version);

		total_len += 1;  // Space after "MayzieBot 0.01"
		total_len += strlen("running on ");
		total_len += sysname_len + 1;  // "Linux "
		total_len += release_len + 1;  // "2.6.28 "
		total_len += version_len + 1;

		char ver_str[total_len];
		strcpy(ver_str, CLIENT_NAME);
		strcat(ver_str, " ");
		strcat(ver_str, CLIENT_VERSION);
		strcat(ver_str, " ");
		strcat(ver_str, "running on ");
		strcat(ver_str, os_version_struct.sysname);
		strcat(ver_str, " ");
		strcat(ver_str, os_version_struct.release);
		strcat(ver_str, " ");
		strcat(ver_str, os_version_struct.version);

		return irc_send_ctcpreply_raw(irc, to, "VERSION", ver_str);
	}
#endif  // _POSIX_VERSION >= 200112L
#endif  // defined(_POSIX_VERSION)

	char ver_str[total_len];
	strcpy(ver_str, CLIENT_NAME);
	strcat(ver_str, " ");
	strcat(ver_str, CLIENT_VERSION);

	return irc_send_ctcpreply_raw(irc, to, "VERSION", ver_str);
}

void irc_recv_privmsg(struct irc *irc, char *from, char *to, char *message) {
	printf("%s <%s> %s\n", to, from, message);
	if(*from != '#') {
		/* Parse CTCP messages */
		if(*message == 0x01) {  // CTCP requests begin with 0x01
			size_t message_len = strlen(message);
			if(*(message + message_len - 1) == 0x01) {  // They also end with 0x01
				if(strncmp("VERSION", message + 1, 7) == 0) {
					irc_send_ctcpreply_version(irc, from);
				} else if(strncmp("PING", message + 1, 4) == 0) {
					irc_send_notice(irc, from, message);
				} else if(strncmp("\001TIME\001", message, 6) == 0) {
					time_t time_r = time(NULL);
					irc_send_ctcpreply_raw(irc, from, "TIME", asctime(localtime(&time_r)));
				}
			}
		} else {
			;
		}
	}
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
				if(strncmp("PRIVMSG ", message, LENGTH_PRIVMSG) == 0) {
					char *nickto = strtok(message + LENGTH_PRIVMSG, " ");
					message = strtok(NULL, "\0");
					if(*message == 0x3a) {
						message++;
						irc_recv_privmsg(irc, nickfrom, nickto, message);
					} else {
						goto CLEANUP;  // Malformed IRC message.
					}
				} else if(strncmp("NOTICE ", message, LENGTH_NOTICE) == 0) {

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
