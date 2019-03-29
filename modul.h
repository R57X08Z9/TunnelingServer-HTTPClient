#ifndef MODUL_H_
#define MODUL_H_

struct buf_memory {
	char *memory;
	size_t size;
};

struct responce {
	char *type;
	char *dns;
	int status;
	char *error;
	char **answer;
	int count_string_answer;
};

static size_t write_answer(void *ptr, size_t size, size_t nmemd, void* arg);

responce sent_dns_query(const char *dns, const char *type);

#endif
