#include <stdio.h>

#include "curl/curl.h"

#include <json.h>

#include "dns_query_lib.h"

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

void free_dns_response_s(dns_response_t *ptr) {
	free(ptr->type);
	free(ptr->dns);
	free(ptr->error);
	for (int i = 0; i < ptr->count_string_answer; i++) {
		free(ptr->answer[i]);
	}
	free(ptr->answer);
	free(ptr);
}

void send_dns_query(dns_response_t *res, const char *dns, const char *type, const char *url, int port) {

	int is_error = 0;
	int status = 1;
	int status_query;
	int p_str_len;
	
	const char *p_str = NULL;
	
	struct buf_memory in_data;
	
	struct json_object *obj = NULL;
	struct json_object *sub_obj = NULL;
	struct json_object *sub2_obj = NULL;
	
	
	obj = json_object_new_object();
	in_data.memory = NULL;
	in_data.size = 0;

	res->status = -1;

	CURL *curl_handle = curl_easy_init();

	if (!curl_handle) {
		fprintf(stderr, "curl not init!\n");
		is_error = 1;
	}

	if (!is_error) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_PORT, port);

		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_answer);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&in_data);

		json_object_object_add(obj, "dns", json_object_new_string(dns));
		json_object_object_add(obj, "type", json_object_new_string(type));

		const char *str = json_object_to_json_string(obj);

		curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, str);

		CURLcode res_curl = curl_easy_perform(curl_handle);

		if (res_curl != CURLE_OK) {
			fprintf(stderr, "%s\n", curl_easy_strerror(res_curl));
		}
		json_object_put(obj);
	
		obj = json_tokener_parse(in_data.memory);
	}

	if (!is_error) {
	status = json_object_object_get_ex(obj, "type", &sub_obj);
		if (status == 0) {
			fprintf(stderr, "\"type\" not found\n");
			is_error = 1;
		}
	}
	if (!is_error) {
		p_str = json_object_get_string(sub_obj);
		p_str_len = strlen(p_str);
		res->type = calloc(p_str_len, sizeof(char));
		strncpy(res->type, p_str, p_str_len);

		status = json_object_object_get_ex(obj, "dns", &sub_obj);
		if (status == 0) {
			fprintf(stderr, "\"dns\" not found\n");
			is_error = 1;
		}
	}
	if (!is_error) {
		p_str = json_object_get_string(sub_obj);
		p_str_len = strlen(p_str);
		res->dns = calloc(p_str_len, sizeof(char));
		strncpy(res->dns, p_str, p_str_len);


		status = json_object_object_get_ex(obj, "response", &sub_obj);
		if (status == 0) {
			fprintf(stderr, "\"response\" not found\n");
			is_error = 1;
		}
	}
	if (!is_error) {
		status = json_object_object_get_ex(sub_obj, "status", &sub2_obj);
		if (status == 0) {
			fprintf(stderr, "\"status\" not found\n");
			is_error = 1;
		}
	}
	if (!is_error) {
		status_query = json_object_get_int(sub2_obj);

		if (status_query == 0) {
		
			res->error = NULL;

			status = json_object_object_get_ex(sub_obj, "answer", &sub2_obj);
			if (status == 0) {
				fprintf(stderr, "\"answer\" not found\n");
				is_error = 1;
			}
			if (!is_error) {

				res->answer = calloc(json_object_array_length(sub2_obj), sizeof(char *));

				res->count_string_answer = json_object_array_length(sub2_obj);
		
				struct json_object *answer = NULL;

				for (int i = 0; i < json_object_array_length(sub2_obj); i++) {
					answer = json_object_array_get_idx(sub2_obj, i);
					p_str = json_object_to_json_string(answer);
					p_str_len = strlen(p_str);
					res->answer[i] = calloc(p_str_len, sizeof(char));
					strncpy(res->answer[i], p_str, p_str_len);
				}
			}

		} else {

			status = json_object_object_get_ex(sub_obj, "error", &sub2_obj);
			if (status == 0) {
				fprintf(stderr, "\"error\" not found\n");
				is_error = 1;
			}
			if (!is_error) {
				p_str = json_object_get_string(sub2_obj);
				p_str_len = strlen(p_str);
				res->error = calloc(p_str_len, sizeof(char));
				strncpy(res->error, p_str, p_str_len);
			}
		}
	}
	
	res->status = status_query;
	curl_easy_cleanup(curl_handle);

	free(in_data.memory);

	json_object_put(obj);
}
