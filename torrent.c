#include "torrent.h"

void free_list(struct bcode_list *list) {

}

void free_dict(struct bcode_dict *dictionary) {

}

void free_node(struct bcode_node *node) {

}

void free_torrent_file(struct file_torrent *torrent_file) {

}

struct bcode_node *parse_string(char **string) {
	struct bcode_node *ret = malloc(sizeof(*ret));

	switch (**string) {
	case 'i': {
		ret->value_type = INTEGER;

		char *endptr = NULL;
		ret->value = (void *) strtol(*string + 1, &endptr, 0);
		if(endptr == *string + 1) {  // Check the condition that no digits were read
			free(ret);
			ret = NULL;
		} else if(*endptr != 'e') {  // Ensure that it has the correct delimiter, 'e'.
			free(ret);
			ret = NULL;
		}

		*string = endptr + 1;

		break;
	}
	case 'l': {
		ret->value_type = LIST;

		if(*(*string + 1) != 'e') {
			struct bcode_list *list = malloc(sizeof(*list));
			ret->value = list;
			
			char *list_string = *string + 1;
			do {
				list->element = parse_string(&list_string);
				list->next = NULL;

				if(list->element == NULL) {
					free_list(ret->value);
					free(ret);
					ret = NULL;
				}
	
				if(*list_string != 'e') {
					list->next = malloc(sizeof(*(list->next)));
				}
	
				list = list->next;
			} while (list != NULL);

			*string = list_string;
		} else {
			ret->value = NULL;
			*string = *(string + 2);
		}

		break;
	}
	case 'd': {
		// Dictionary
		
		break;
	}
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': {
		// Parse string
		ret->value_type = STRING;
		char *endptr = NULL;
		long int length = strtol(*string, &endptr, 0);

		if(*endptr != ':') {
			free(ret);
			ret = NULL;

			*string = endptr + 1;
		} else {
			char *value_string = malloc(sizeof(char) * (length + 1));
			value_string = strncpy(value_string, endptr + 1, length);
			*(value_string + length) = '\0';
			ret->value = value_string;

			*string = endptr + 1 + length + 1;
		}
		
		break;
	}
	default: {
		// An error has occurred.
		free(ret);
		ret = NULL;
	}
	}

	return ret;
}

struct file_torrent *parse_file(const char *filename) {

}
