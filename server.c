#include <sys/types.h> 
#include <stdio.h> 

#include "fcgi_config.h" 
#include "fcgiapp.h" 

#include <ares.h>
#include <arpa/nameser.h>
#include <netdb.h>

#include <json.h>

#define SOCKET_PATH "127.0.0.1:9000"
#define TIME_LIMIT_IN_SEC 5
#define IS_DEBUG "-d"

static int socket_id;
static int is_debug = 0;

static void answer_a_to_json_array(struct hostent *host, json_object *obj) {
	char ip[INET6_ADDRSTRLEN];
	for (int i = 0; host->h_addr_list[i]; i++) {
		inet_ntop((int)host->h_addrtype, (const void *)host->h_addr_list[i], ip, sizeof(ip));
		json_object_array_add(obj, json_object_new_string(ip));
		if (is_debug) {
			fprintf(stderr, "ip_i = %s\n", ip);
		}
	}
}

static void callback_a(void *arg, int status, int timeouts, unsigned char *abuf, int alen) {

	struct hostent *host = NULL;
	int *naddrttls = NULL;
	struct ares_addrttl *addrttls = NULL;

	if (status == ARES_SUCCESS) {

		int status_pars = ares_parse_a_reply(abuf, alen, &host, addrttls, naddrttls);
		
		if (status_pars == ARES_SUCCESS) {
			/*обработка ответа, код ошибки(если есть)б при ОК код 0 и поле answer*/
			struct json_object *obj = json_object_new_object();
			answer_a_to_json_array(host, obj);
			json_object_object_add(arg, "status", json_object_new_int(0));
			json_object_object_add(arg, "answer", obj);
			free(addrttls);
			free(naddrttls);
		} else {
			json_object_object_add(arg, "status", json_object_new_int(1));
			printf("callback can't parse answer\n");
			fprintf(stderr, "parse error: %s\n", ares_strerror(status));
		}

		ares_free_hostent(host);
	} else {
		json_object_object_add(arg, "status", json_object_new_int(1));
		printf("query faild\n");
		fprintf(stderr, "query error: %s\n", ares_strerror(status));
	}
}

static void answer_txt_to_json_array(struct ares_txt_reply *txt, struct json_object *obj) {
	json_object_array_add(obj, json_object_new_string(txt->txt));
	if (is_debug) {
		fprintf(stderr, "txt_i = %s\n", txt->txt);
	}
	if (txt->next) {
		answer_txt_to_json_array(txt->next, obj);
	}
}

static void callback_txt(void *arg, int status, int timeouts, unsigned char *abuf, int alen) {

	struct ares_txt_reply *txt_out = NULL;
	if (status == ARES_SUCCESS) {

		int status_pars = ares_parse_txt_reply(abuf, alen, &txt_out);
		
		if (status_pars == ARES_SUCCESS) {
			struct json_object *obj = json_object_new_object();
			json_object_object_add(arg, "status", json_object_new_int(0));
			json_object_object_add(arg, "answer", obj);
			if (txt_out) {
				answer_txt_to_json_array(txt_out, obj);
			}
		} else {
			json_object_object_add(arg, "status", json_object_new_int(1));
			printf("callback can't parse answer");
			fprintf(stderr, "parse error: %s\n", ares_strerror(status));
		}

		ares_free_data(txt_out);
	} else {
		json_object_object_add(arg, "status", json_object_new_int(1));
		printf("query faild\n");
		fprintf(stderr, "query error: %s\n", ares_strerror(status));
	}
}

static void wait_ares(ares_channel channel) {
	printf("start wait_ares\n");
	int nfds, count;
	fd_set readers, writers;
	struct timeval tv, *tvp, tvmax;
	tvmax.tv_sec = TIME_LIMIT_IN_SEC;
	for (;;) {
		FD_ZERO(&readers);
		FD_ZERO(&writers);
		nfds = ares_fds(channel, &readers, &writers);
		if (nfds == 0)
			break;
		tvp = ares_timeout(channel, &tvmax, &tv);
		count = select(nfds, &readers, &writers, NULL, tvp);
		ares_process(channel, &readers, &writers);
		
	}
}

int sent_requ(struct json_object *obj, ares_channel channel) {
	int status = 0;

	struct json_object *sub_obj = NULL;

	status = json_object_object_get_ex(obj, "dns", &sub_obj);
	if (status == 0) {
		return 0;
	}
	const char *dns_query = json_object_get_string(sub_obj);

	status = json_object_object_get_ex(obj, "type", &sub_obj);
	if (status == 0) {
		return 0;
	}
	const char *type_query = json_object_get_string(sub_obj);
	
	if (is_debug) {
		fprintf(stderr, "get query: DNS = %s, Type = %s", dns_query, type_query); // отладочная
	}

	json_object_put(sub_obj);
	struct json_object *response_obj = json_object_new_object();

	if (strcmp(type_query, "A") == 0) {
		ares_query(channel, dns_query, ns_c_in, ns_t_a, callback_a, response_obj);
		status = 1;
	}
	if (strcmp(type_query, "TXT") == 0) {
		ares_query(channel, dns_query, ns_c_in, ns_t_txt, callback_txt, response_obj);
		status = 1;
	}
	
	wait_ares(channel);

	if (status) {
		json_object_object_add(obj, "response", response_obj);
	}

	return status;
}

int main(int ac, char *av[]) {
	if (ac > 1) {
		is_debug = !strcmp(av[1], IS_DEBUG);
	}
	/* настройк c-ares */
	ares_channel channel;
	int status;
	struct ares_options options;
	int optmask = 0;

	status = ares_library_init(ARES_LIB_INIT_ALL);
	if (status != ARES_SUCCESS) {
		printf("ares_library init failed\n");
		fprintf(stderr, "ares_library_init: %s\n", ares_strerror(status));
		return 1;
	} else {
		printf("ares_library init complit\n");
	}

	status = ares_init_options(&channel, &options, optmask);
	if (status != ARES_SUCCESS) {
		printf("ares_init_options failed\n");
		fprintf(stderr, "ares_init_options: %s\n", ares_strerror(status));
		return 1;
	} else {
		printf("ares_init_options complit\n");
	}

	/* настройка FastCGI */
	FCGX_Init();
	printf("Lib is inited\n");
	    
	socket_id = FCGX_OpenSocket(SOCKET_PATH, 20);
	if(socket_id < 0) { 
		return 1;
	}
 
	printf("Socket is opened\n");
	
	int rc;
	FCGX_Request request;
	char *server_name;

	if(FCGX_InitRequest(&request, socket_id, 0) != 0) { 
		printf("Can not init request\n");
		return 1;
	} 

	printf("Request is inited\n");
 
	for(;;) {

		printf("Try to accept new request\n");
		
		rc = FCGX_Accept_r(&request);
		if(rc < 0) {
			printf("Can not accept new request\n");
			break;
		}

		printf("request is accepted\n");

		char *strLenContent = FCGX_GetParam("CONTENT_LENGTH", request.envp);
		if (!strcmp(strLenContent, "(null)")) {
			if (is_debug) {
				fprintf(stderr, "Status: 400 Bad Request\n");
			}
			FCGX_PutS("Status: 400 Bad Request\r\n", request.out);
			FCGX_Finish_r(&request);
			continue;
		}
		int inSize = atoi(strLenContent);
		char *strIn = NULL;
		strIn = calloc(inSize + 1, sizeof(char));
		FCGX_GetStr(strIn, inSize, request.in);
		strIn[inSize] = '\0';

		struct json_object *obj, *tmp;
		obj = json_tokener_parse(strIn);
		free(strIn);
		if (is_debug) {
			fprintf(stderr, 
				"\n---\n%s\n---\n",
				json_object_to_json_string_ext(obj,
								JSON_C_TO_STRING_SPACED |
								JSON_C_TO_STRING_PRETTY));
		}
		
		if (sent_requ(obj, channel)) {
		
			if (is_debug) {
				fprintf(stderr, 
					"\n---\n%s\n---\n",
					json_object_to_json_string_ext(obj,
									JSON_C_TO_STRING_SPACED |
									JSON_C_TO_STRING_PRETTY));
			}
			FCGX_PutS("Content-type: application/json\r\n", request.out);
			FCGX_PutS("\r\n", request.out);
			FCGX_PutS(json_object_to_json_string_ext(obj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), request.out);
		} else {
			FCGX_PutS("Status: 400 Bad Request\r\n", request.out);
		}

		/* закрыть текущее соединение */
		FCGX_Finish_r(&request);
		json_object_put(obj);
		printf("query complit");
	} 

	printf("\nexit");
	return 0; 
}
