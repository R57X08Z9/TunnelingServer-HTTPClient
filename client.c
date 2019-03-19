#include <stdio.h>

#include "curl/curl.h"

#include <json.h>

static size_t write_answer(char *ptr, size_t size, size_t nmemd, void* arg) {
	struct json_object *obj = json_tokener_parse(ptr);
	printf("serwer answer \n---\n%s\n---\n", json_object_to_json_string(obj));
}

int main() {
	
	CURL *curl_handle = curl_easy_init();

	if (curl_handle) {

//		curl_easy_setopt(curl_handle, CURLOPT_URL, "http://www.cyberforum.ru");
		curl_easy_setopt(curl_handle, CURLOPT_URL, "localhost");
		curl_easy_setopt(curl_handle, CURLOPT_PORT, 8000);

		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_answer);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);

		struct json_object *obj = NULL;
		obj = json_object_new_object();
		json_object_object_add(obj, "dns", json_object_new_string("vk.com"));
		json_object_object_add(obj, "type", json_object_new_string("A"));
		const char *str = json_object_to_json_string(obj);

		curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, str);

		CURLcode res = curl_easy_perform(curl_handle);

		curl_easy_cleanup(curl_handle);

	}

	printf("hello World!\n");
	return 0;
}