#include "irc-autodl.h"

int main(int argc, char** argv) {
	if(argc > 0 || argv) {};  // Silence warning
	srand(time(NULL));

	struct irc *irc = irc_init("irc.rizon.net", NULL, "6667");
	if(irc_connect(irc) <= 0) {
		printf("Unable to connect.\n");
		return -1;
	}

	printf("Connected %d.\n", irc->sockid);
	irc_send(irc, "NICK MayzieBot");
	irc_send(irc, "USER MaisieBot.Is 8 * :Maisie C");
	irc_join(irc, "#mayziebottest");

	while(true) {
		if(irc_receive_raw(irc) == false)
			break;
		//sleep(1);
	}

	irc_free(irc);

	return 0;
}
