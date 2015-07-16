#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define CLIENT_NAME "MayzieBot"
#define CLIENT_VERSION "0.01"

#define IRC_TOTAL_MSG_LEN 512
#define IRC_MSG_LEN (IRC_TOTAL_MSG_LEN - 2)
#define CRLF "\r\n"

struct irc_channel {
	const char *name;
	char *topic;
};

struct irc_channel_list {
	struct irc_channel *channel;
	struct irc_channel_list *next;
};

struct irc_last_message {
	char *message;
	int length;
};

struct irc {
	int sockid;

	char *nickname;
	char *username;
	char *realname;

	const char *password;  // Server password
	const char *hostname;  // Server address
	const char *port;  // Server port

	struct irc_channel_list *channels;
//	char *message;
	struct irc_last_message last_msg;
};

int irc_send(const struct irc *, const char *);
void irc_parse_raw(struct irc *, char *pos_crlf);
bool irc_receive_raw(struct irc *);
bool irc_join(struct irc *, const char *);
struct irc *irc_init(const char *, const char *, const char *);
void irc_free(struct irc *);
int irc_connect(struct irc *);

#endif	// CLIENT_H
