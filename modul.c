#include <stdio.h>

#include "curl/curl.h"

#include <json.h>

#include "modul.h"

#include <string.h>

static size_t write_answer(void *contents, size_t size, size_t nmemd, void* arg) {
	size_t in_size = size * nmemd;
	struct buf_memory *mem = (struct buf_memory *)arg;
	char *ptr = realloc(mem->memory, mem->size + in_size + 1);
	if (ptr == NULL) {
		printf("out of memory\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, in_size);
	mem->size += in_size;
	mem->memory[mem->size] = 0;

	return in_size;
}

int sent_dns_query(struct response *res, const char *dns, const char *type) {
	
//	struct response res;
	int status = 1;
	struct buf_memory in_data;
	in_data.memory = malloc(1);
	in_data.size = 0;

	res->status = -1;

	CURL *curl_handle = curl_easy_init();

	if (!curl_handle) {
		printf("curl not int!\n");
		return 1;
	}

	curl_easy_setopt(curl_handle, CURLOPT_URL, "localhost");
	curl_easy_setopt(curl_handle, CURLOPT_PORT, 8000);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_answer);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&in_data);

	struct json_object *obj = NULL;
	obj = json_object_new_object();

	json_object_object_add(obj, "dns", json_object_new_string(dns));
	json_object_object_add(obj, "type", json_object_new_string(type));

	const char *str = json_object_to_json_string(obj);

	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, str);

	CURLcode res_curl = curl_easy_perform(curl_handle);

	json_object_put(obj);
	
	obj = json_tokener_parse(in_data.memory);
	
	struct json_object *sub_obj = NULL;
	struct json_object *sub2_obj = NULL;

	const char *p_str = NULL;
	int p_str_len;

	status = json_object_object_get_ex(obj, "type", &sub_obj);
	if (status == 0) {
		fprintf(stderr, "\"type\" not find\n");
		return 1;
	}
	p_str = json_object_get_string(sub_obj);
	p_str_len = strlen(p_str);
	res->type = calloc(p_str_len, sizeof(char));
	strncpy(res->type, p_str, p_str_len);

	status = json_object_object_get_ex(obj, "dns", &sub_obj);
	if (status == 0) {
		fprintf(stderr, "\"dns\" not find\n");
		return 1;
	}
	p_str = json_object_get_string(sub_obj);
	p_str_len = strlen(p_str);
	res->dns = calloc(p_str_len, sizeof(char));
	strncpy(res->dns, p_str, p_str_len);

	status = json_object_object_get_ex(obj, "response", &sub_obj);
	if (status == 0) {
		fprintf(stderr, "\"response\" not find\n");
		return 1;
	}

	status = json_object_object_get_ex(sub_obj, "status", &sub2_obj);
	if (status == 0) {
		fprintf(stderr, "\"status\" not find\n");
		return 1;
	}

	int status_query = json_object_get_int(sub2_obj);
	
	char **ans2 = NULL;

/*	printf("status_query: %d\n", status_query); */
	if (status_query == 0) {
		
		res->error = NULL;

		status = json_object_object_get_ex(sub_obj, "answer", &sub2_obj);
		if (status == 0) {
			fprintf(stderr, "\"answer\" not find\n");
			return 1;
		}
/*		printf("%s, %s, %d, ", type_query, dns_query, status_query);*/

		res->answer = calloc(json_object_array_length(sub2_obj), sizeof(char *));

		res->count_string_answer = json_object_array_length(sub2_obj);
		
		struct json_object *answer = NULL;

		for (int i = 0; i < json_object_array_length(sub2_obj); i++) {
			answer = json_object_array_get_idx(sub2_obj, i);
			p_str = json_object_to_json_string(answer);
			p_str_len = strlen(p_str);
			res->answer[i] = calloc(p_str_len, sizeof(char));
			strncpy(res->answer[i], p_str, p_str_len);
/*			printf("%s\n", json_object_to_json_string(answer));
			printf("%s\n", res->answer[i]);*/
		}
	} else {

		status = json_object_object_get_ex(sub_obj, "error", &sub2_obj);
		if (status == 0) {
			fprintf(stderr, "\"error\" not find\n");
			return 1;
		}
		p_str = json_object_get_string(sub2_obj);
		p_str_len = strlen(p_str);
		res->error = calloc(p_str_len, sizeof(char));
		strncpy(res->error, p_str, p_str_len);
/*		printf("%s, %s, %d, %s\n", type_query, dns_query, &status_query, error_query); */
	}

	res->status = status_query;
	curl_easy_cleanup(curl_handle);
	json_object_put(obj);
	return 0;
}
