#ifndef TORRENT_H
#define TORRENT_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef enum {
	STRING,
	INTEGER,
	LIST,
	DICTIONARY
} bcode_type;

struct bcode_node {
	bcode_type value_type;
	void *value;
};

struct bcode_list {
	struct bcode_node *element;

	struct bcode_list *next;
};

struct bcode_dict {
	char *key;
	struct bcode_node *element;

	struct bcode_dict *next;
};
	
struct file_torrent {
	struct bcode_node *element;

	struct file_torrent *next;
};

void free_list(struct bcode_list *);
void free_dict(struct bcode_dict *);
void free_node(struct bcode_node *);
void free_torrent_file(struct file_torrent *);
struct bcode_node *parse_string(char **);
struct file_torrent *parse_file(const char *);

#endif  // TORRENT_H
