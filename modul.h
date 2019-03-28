#ifndef MODUL_H_
#define MODUL_H_

struct buf_memory {
	char *memory;
	size_t size;
};

static size_t write_answer(void *ptr, size_t size, size_t nmemd, void* arg);

int sent_dns_query(const char *dns, const char *type);

#endif
