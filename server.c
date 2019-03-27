#include <sys/types.h> 
#include <stdio.h> 

#include "fcgi_config.h" 
#include "fcgiapp.h" 

#include <ares.h>
#include <arpa/nameser.h>
#include <netdb.h>

#include <json.h>

#define SOCKET_PATH "127.0.0.1:9000"
#define MAX_TIME 5

static int socketId; 

static void answer_a_to_json_array(struct hostent *host, json_object *obj) {
	char ip[INET6_ADDRSTRLEN];
	for (int i = 0; host->h_addr_list[i]; i++) {
		inet_ntop((int)host->h_addrtype, (const void *)host->h_addr_list[i], ip, sizeof(ip));
		json_object_array_add(obj, json_object_new_string(ip));
		printf("ip_i = %s\n", ip); // отладочная информация
	}
}

static void callback_a(void *arg, int status, int timeouts, unsigned char *abuf, int alen) {

	struct json_object *obj = arg;
	struct hostent *host = NULL;
	int *naddrttls = NULL;
	struct ares_addrttl *addrttls = NULL;

	if (status == ARES_SUCCESS) {

		int status_pars = ares_parse_a_reply(abuf, alen, &host, addrttls, naddrttls);
		
		if (status_pars == ARES_SUCCESS) {
			answer_a_to_json_array(host, obj);
			free(addrttls);
			free(naddrttls);
		} else {
			printf("callback can't parse answer\n");  // необходимая
			printf("parse error: %s\n", ares_strerror(status)); // ошибка
		}

		ares_free_hostent(host);
	} else {
		printf("query faild\n"); // необходимая
		printf("query error: %s\n", ares_strerror(status)); // ошибка
	}
}

static void answer_txt_to_json_array(struct ares_txt_reply *txt, struct json_object *obj) {
	json_object_array_add(obj, json_object_new_string(txt->txt));
	printf("txt_i = %s\n", txt->txt); // отладочная информация
	if (txt->next) {
		answer_txt_to_json_array(txt->next, obj);
	}
}

static void callback_txt(void *arg, int status, int timeouts, unsigned char *abuf, int alen) {

	struct ares_txt_reply *txt_out = NULL;
	if (status == ARES_SUCCESS) {

		int status_pars = ares_parse_txt_reply(abuf, alen, &txt_out);
		
		if (status_pars == ARES_SUCCESS) {
			if (txt_out)
				answer_txt_to_json_array(txt_out, arg);
		} else {
			printf("callback can't parse answer"); // необходимая
			printf("parse error: %s\n", ares_strerror(status)); // ошибка
		}

		ares_free_data(txt_out);
	} else {
		printf("query faild\n"); // необходимая
		printf("query error: %s\n", ares_strerror(status)); // ошибка
	}
}

static void wait_ares(ares_channel channel) {
	printf("start wait_ares\n"); // необходимая
	int nfds, count;
	fd_set readers, writers;
	struct timeval tv, *tvp, tvmax;
	tvmax.tv_sec = MAX_TIME;
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
/*
int sent_requ(struct json_object *obj, char *dns, char *type) {
	bool status;
}*/

int main(void) {

	/* настройк c-ares */
	ares_channel channel;
	int status;
	struct ares_options options;
	int optmask = 0;

	status = ares_library_init(ARES_LIB_INIT_ALL);
	if (status != ARES_SUCCESS) {
		printf("ares_library init failed\n"); // необходимая
		printf("ares_library_init: %s\n", ares_strerror(status)); // ошибка
		return 1;
	} else {
		printf("ares_library init complit\n"); // необходимая
	}

	status = ares_init_options(&channel, &options, optmask);
	if (status != ARES_SUCCESS) {
		printf("ares_init_options failed\n"); // необходимая
		printf("ares_init_options: %s\n", ares_strerror(status)); // ошибка
		return 1;
	} else {
		printf("ares_init_options complit\n"); // необходимая
	}

	/* настройка FastCGI */
	FCGX_Init();
	printf("Lib is inited\n"); // необходимая
	    
	socketId = FCGX_OpenSocket(SOCKET_PATH, 20);
	if(socketId < 0) { 
		return 1;
	}
 
	printf("Socket is opened\n"); // необходимая
	
	int rc;
	FCGX_Request request;
	char *server_name;

	if(FCGX_InitRequest(&request, socketId, 0) != 0) { 
		printf("Can not init request\n"); // необходимая
		return 1;
	} 

	printf("Request is inited\n"); // необходимая
 
	for(;;) {

		printf("Try to accept new request\n"); // необходимая
		
		rc = FCGX_Accept_r(&request);
		if(rc < 0) {
			printf("Can not accept new request\n"); // необходимая
			break;
		}

		printf("request is accepted\n"); // необходимая

		char *strLenContent = FCGX_GetParam("CONTENT_LENGTH", request.envp);
		if (!strcmp(strLenContent, "(null)")) {
			printf("Status: 400 Bad Request\n"); // необходимая
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

		printf("\n---\n%s\n---\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY)); // отладочная

//		struct json_object *tmp;
		/*вынести в отделный блок*/
		json_object_object_get_ex(obj, "dns", &tmp);
		const char *dns_query = json_object_get_string(tmp);

		json_object_object_get_ex(obj, "type", &tmp);
		const char *type_query = json_object_get_string(tmp);

		printf("get query: DNS = %s, Type = %s", dns_query, type_query); // необходимая

		tmp = json_object_new_array();

		if (!strcmp(type_query, "A"))
			ares_query(channel, dns_query, ns_c_in, ns_t_a, callback_a, tmp);
		if (!strcmp(type_query, "TXT"))
			ares_query(channel, dns_query, ns_c_in, ns_t_txt, callback_txt, tmp);
		/*вынести в отделный блок до сюда*/
		wait_ares(channel);

		// сформировать и вернуть ответ
		json_object_object_add(obj, "answer", tmp);
//		printf("\n---\n%s\n---\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
		FCGX_PutS("Content-type: application/json\r\n", request.out);
		FCGX_PutS("\r\n", request.out);
		FCGX_PutS(json_object_to_json_string_ext(obj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), request.out);
		//закрыть текущее соединение
		FCGX_Finish_r(&request);
		printf("query complit"); // необходимая
	} 

	printf("\nexit"); // необходимая
	return 0; 
}
