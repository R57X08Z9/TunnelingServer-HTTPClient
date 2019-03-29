#ifndef MODUL_H_
#define MODUL_H_

struct buf_memory {
	char *memory;
	size_t size;
};

struct response {
	const char *type;
	const char *dns;
	int status;
	const char **answer;
	const char *error;
	int count_string_answer;
};

static size_t write_answer(void *ptr, size_t size, size_t nmemd, void* arg);

int sent_dns_query(struct response *res, const char *dns, const char *type);

#endif
