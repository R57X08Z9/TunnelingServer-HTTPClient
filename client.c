#include <stdio.h>

#include "curl/curl.h"

#include <json.h>

static size_t write_answer(char *ptr, size_t size, size_t nmemd, void* arg) {
	struct json_object *obj = json_tokener_parse(ptr);
	struct json_object *tmp, *tmp2;
	json_object_object_get_ex(obj, "answer", &tmp);
	// проверка наличия ответа
	printf("begin answer \n-\n");
	for (int i = 0; i < json_object_array_length(tmp); i++) {
		tmp2 = json_object_array_get_idx(tmp, i);
		printf("%s\n", json_object_to_json_string(tmp2));
		json_object_put(tmp2);
	}
	json_object_put(tmp);
	printf("-\nend answer\n");
	json_object_put(obj);
//	printf("serwer answer \n-\n%s\n-\n", json_object_to_json_string(obj));
}

int main() {
	
	CURL *curl_handle = curl_easy_init();

	if (!curl_handle) {
		printf("curl not int!");
		return 1;
	}

	char *type_q = NULL;
	type_q = realloc(type_q, 4);

	char *dns_q = NULL;
	dns_q = realloc(dns_q, 255);

	for (;;) {

		printf("---");

		curl_easy_setopt(curl_handle, CURLOPT_URL, "localhost");
		curl_easy_setopt(curl_handle, CURLOPT_PORT, 8000);

		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_answer);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);

		struct json_object *obj = NULL;
		obj = json_object_new_object();
		
		scanf("%*[^\n]");

		printf("\ntype=");
		scanf("%3s", type_q);

		scanf("%*[^\n]");
		
		printf("dns=");
		scanf("%254s", dns_q);

//		printf("\ntype=%s", type_q);
//		printf("\n---read dns=%s\nread type=%s\n---\n", dns_q, type_q);

		json_object_object_add(obj, "dns", json_object_new_string(dns_q));
		json_object_object_add(obj, "type", json_object_new_string(type_q));

		const char *str = json_object_to_json_string(obj);

		curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, str);

		CURLcode res = curl_easy_perform(curl_handle);
		json_object_put(obj);
		printf("---\n");
	}
	curl_easy_cleanup(curl_handle);

	printf("\nexit");

	return 0;
}