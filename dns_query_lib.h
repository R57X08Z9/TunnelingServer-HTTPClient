#ifndef DNS_QUERY_LIB_H_
#define DNS_QUERY_LIB_H_

struct buf_memory {
	char *memory;
	size_t size;
};

typedef struct dns_response_s {
	char *type;
	char *dns;
	int status;
	char **answer;
	char *error;
	int count_string_answer;
} dns_response_t;

static size_t write_answer(void *ptr, size_t size, size_t nmemd, void* arg);

void free_dns_response_s(dns_response_t *res);

void send_dns_query(dns_response_t *res, const char *dns, const char *type, const char *url, int port);

#endif
