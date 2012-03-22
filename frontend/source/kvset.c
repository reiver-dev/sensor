#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include <wordexp.h>

#include <assert.h>

#include "kvset.h"

#define MAXSTRING 255

struct value {
	int type;
	char *name;
	void *content;
	struct value *next;
};

struct kvset {
	int catCount;
	char **sections;
	struct value **values;
};

kvset InitKVS(int count, const char **sections) {
	kvset kvs = malloc(sizeof(*kvs));
	kvs->catCount = count;

	if (count && sections) {

		kvs->values   = malloc(count * sizeof(void *));
		kvs->sections = malloc(count * sizeof(void *));
		memset(kvs->values, '\0', count * sizeof(void *));
		memset(kvs->sections, '\0', count * sizeof(void *));

		for (int i = 0; i < count; i++) {
			int tmp = strlen(sections[i]);
			kvs->sections[i] = malloc(tmp);
			strcpy(kvs->sections[i], sections[i]);
		}

	} else {

		kvs->values   = malloc(sizeof(void *));
		kvs->sections = 0;

	}

	return kvs;

}

void AddKVS(kvset self, int section, int type, char *name, void *value) {
	assert(self);

	struct value *val = malloc(sizeof(*val));

	val->content = value;
	val->type    = type;

	int len = strlen(name);
	val->name = malloc(len);
	strcpy(val->name, name);

	val->next = self->values[section];
	self->values[section] = val;

}

static int get_cmp_section(kvset self, char *buf, int len) {
	if (buf[0] != '[') {
		return -1;
	}

	int sec_len = strchr(buf + 1,']') - buf - 1;

	if (sec_len <= 1) {
		return -1;
	}

	for (int i = 0; i < self->catCount; i++) {
		if (!strncmp(self->sections[i], buf + 1, sec_len)) {
			return i;
		}
	}

	return -1;
}

static int get_key_value(char *key, char *value, char *buffer, int keylen, int vallen, int buflen) {

	memset(key, '\0', keylen);
	memset(value, '\0', vallen);

	int i = 0, j = 0;

	/* get key */
	for (; i < buflen && j < keylen; i++) {
		if (buffer[i] == ' ') {
			continue;
		} else if (buffer[i] == '=') {
			break;
		}
		key[j] = buffer[i];
		j++;
	}

	/* no value */
	if (i == buflen || j == keylen) {
		return 1;
	}

	i++;

	/* get value */
	for (j = 0; i < buflen && j < vallen; i++) {
		if (buffer[i] == ' ') {
			continue;
		} else if (buffer[i] == '\n') {
			break;
		}
		value[j] = buffer[i];
		j++;
	}

	return 0;
}


static int parseValue(int type, char *value, void *ptr) {
	char **buf;

	switch(type) {

	case KVS_STRING:
		buf = ptr;
		int len = strlen(value);
		if (len > MAXSTRING)
			len = MAXSTRING;
		*buf = malloc(len + 1);
		memset(*buf, 0, len + 1);
		strncpy(*(char **)ptr, value, len);
		break;

	case KVS_INT32:
		*(int32_t *)ptr = (int32_t) strtol(value, 0, 10);
		break;

	case KVS_INT8:
		*(int8_t *)ptr = (int8_t)strtol(value, 0, 10);
		break;

	case KVS_UINT32:
		*(uint32_t *)ptr = (uint32_t)strtol(value, 0, 10);
		break;

	case KVS_UINT8:
		*(uint8_t *)ptr = (uint8_t)strtol(value, 0, 10);
		break;

	case KVS_BOOL:
		*(bool *)ptr = !strcmp(value, "1") || !strcmp(value, "true") ? true : false;
		break;

	default:
		break;

	}

	return 0;
}


int LoadKVS(kvset self, const char *filename) {
	assert(self);
	assert(filename);
	assert(strlen(filename));

	wordexp_t exp_result;
	wordexp(filename, &exp_result, 0);

	FILE *file = fopen(exp_result.we_wordv[0], "r");
	wordfree(&exp_result);

	if (!file) {
		return 1;
	}

	char buf[512];
	int len;
	int section;
	while(fgets(buf, 512, file)) {

		len = strlen(buf);

		/* comment */
		if (len <= 1 || buf[0] == '#') {
			continue;
		}

		/* section */
		int tmp = get_cmp_section(self, buf, len);
		if (tmp >= 0) {
			section = tmp;
			continue;
		}


		/* data */
		if (section < self->catCount && strchr(buf, '=')) {
			struct value *current = self->values[section];
			char key[128];
			char value[512];
			while(current) {
				get_key_value(key, value, buf, 128, 512, len);
				if (key[0] && value[0] && !strcmp(current->name, key)) {
					parseValue(current->type, value, current->content);
				}
				current = current->next;
			}
		}


	}

	fclose(file);
	return 0;
}

void DestroyKVS(kvset self) {
	struct value *current;
	struct value *tmp;

	for (int i = 0; i < self->catCount; i++) {
		free(self->sections[i]);

		current = self->values[i];
		while (current) {
			free(current->name);
			tmp = current->next;
			free(current);
			current = tmp;
		}
	}

	free(self->sections);
	free(self->values);
	free(self);

}

