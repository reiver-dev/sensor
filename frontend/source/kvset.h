#ifndef KVSET_H_
#define KVSET_H_

typedef struct kvset *kvset;

enum KVS_TYPE {
	KVS_STRING,
	KVS_HEX,
	KVS_UINT32,
	KVS_UINT8,
	KVS_INT32,
	KVS_INT8,
	KVS_BOOL
};

kvset InitKVS(int count, const char **sections);
void AddKVS(kvset self, int section, int type, char *name, void *value);
int LoadKVS(kvset self, const char *filename);
void DestroyKVS(kvset self);



#endif /* KVSET_H_ */
