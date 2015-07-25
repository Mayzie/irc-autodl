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

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#include <unistd.h>
#if defined(_POSIX_VERSION)
#if _POSIX_VERSION >= 200112L
#include <sys/utsname.h>
#endif  // _POSIX_VERSION >= 200112L
#endif  // defined(_POSIX_VERSION)
#endif  // UNIX

#define CLIENT_NAME "MayzieBot"
#define CLIENT_VERSION "0.01"

#define CLIENT_NAME_LEN 9  // strlen(CLIENT_NAME)
#define CLIENT_VERSION_LEN 4  // strlen(CLIENT_VERSION)

#define IRC_TOTAL_MSG_LEN 512
#define IRC_MSG_LEN (IRC_TOTAL_MSG_LEN - 2)
#define CRLF "\r\n"

#define LENGTH_PRIVMSG 8  // "PRIVMSG "
#define LENGTH_NOTICE 7  // "NOTICE "

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

struct irc *irc_init(const char *, const char *, const char *);
void irc_free(struct irc *);
int irc_join(struct irc *, const char *);
int irc_quit(struct irc *, const char *);
int irc_send(const struct irc *, const char *);
int irc_send_notice(struct irc *, char *, char *);
int irc_send_privmsg(struct irc *, char *, char *);
bool irc_receive_raw(struct irc *);
int irc_connect(struct irc *);

#endif	// CLIENT_H
