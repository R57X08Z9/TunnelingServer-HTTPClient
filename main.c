#include <sys/types.h> 
#include <stdio.h> 

#include "fcgi_config.h" 
#include "fcgiapp.h" 

#include <ares.h>
#include <arpa/nameser.h>
#include <netdb.h>

#include <json.h>

#define SOCKET_PATH "127.0.0.1:9000"
#define NS_T_A 1
#define NS_T_TXT 2

static int socketId; 

static void state_cb(void *data, int s, int read, int write) {}

static void callback_a(void *arg, int status, int timeouts, unsigned char *abuf, int alen) {

	struct json_object *obj = arg;
	struct hostent *host = NULL;
	int *naddrttls = NULL;
	struct ares_addrttl *addrttls = NULL;

	if (status == ARES_SUCCESS) {

		int status_pars = ares_parse_a_reply(abuf, alen, &host, addrttls, naddrttls);
		
		if (status_pars == ARES_SUCCESS) {
			char ip[INET6_ADDRSTRLEN];
			for (int i = 0; host->h_addr_list[i]; i++) {
				inet_ntop((int)host->h_addrtype, (const void *)host->h_addr_list[i], ip, sizeof(ip));
				json_object_array_add(obj, json_object_new_string(ip));
				printf("%s\n", ip);
			}
			free(addrttls);
			free(naddrttls);
		} else {
			printf("callback can't parse answer");
			printf("parse error: %s\n", ares_strerror(status));
		}

		ares_free_hostent(host);	
	} else {
		printf("query error: %s\n", ares_strerror(status));
	}
}

static void print_txt(struct ares_txt_reply *txt, void *arg) {
	if (txt) {
		struct json_object *obj = arg;
		json_object_array_add(obj, json_object_new_string(txt->txt));
		printf("txt_i = %s \n", txt->txt);
		print_txt(txt->next, arg);
	}
}

static void callback_txt(void *arg, int status, int timeouts, unsigned char *abuf, int alen) {

	struct ares_txt_reply *txt_out = NULL;
	if (status == ARES_SUCCESS) {

		int status_pars = ares_parse_txt_reply(abuf, alen, &txt_out);
		
		if (status_pars == ARES_SUCCESS) {
			print_txt(txt_out, arg);
		} else {
			printf("callback can't parse answer");
		}

		ares_free_data(txt_out);
	}
}

static void wait_ares(ares_channel channel) {
	printf("start wait_ares \n");
	int nfds, count;
	int t = 0;
	fd_set readers, writers;
	struct timeval tv, *tvp;
	for (;;) {
		FD_ZERO(&readers);
		FD_ZERO(&writers);
		nfds = ares_fds(channel, &readers, &writers);
		if (nfds == 0)
			break;
		tvp = ares_timeout(channel, NULL, &tv);
		count = select(nfds, &readers, &writers, NULL, tvp);
		ares_process(channel, &readers, &writers);
		t++;
		printf("iter = %d \n", t);
		if (t > 10) {
			printf("time out \n");
			break;
		}
	}
}

int main(void) 
{
	// настройк c-ares
	ares_channel channel;
	int status;
	struct ares_options options;
	int optmask = 0;

	status = ares_library_init(ARES_LIB_INIT_ALL);
	if (status != ARES_SUCCESS) {
		printf("ares_library_init: %s\n", ares_strerror(status));
		return 1;
	}

	options.sock_state_cb = state_cb;
	optmask |= ARES_OPT_SOCK_STATE_CB;

	status = ares_init_options(&channel, &options, optmask);
	if (status != ARES_SUCCESS) {
		printf("ares_init_options: %s\n", ares_strerror(status));
		return 1;
	} 

	// настройка FastCGI
	FCGX_Init(); 
	printf("Lib is inited\n"); 
	    
	socketId = FCGX_OpenSocket(SOCKET_PATH, 20); 
	if(socketId < 0) 
	{ 
		return 1; 
	}
 
	printf("Socket is opened\n"); 
	
	int rc; 
	FCGX_Request request; 
	char *server_name;

	if(FCGX_InitRequest(&request, socketId, 0) != 0) 
	{ 
		printf("Can not init request\n"); 
		return 1; 
	} 

	printf("Request is inited\n"); 
 
	for(;;) 
	{
		printf("Try to accept new request\n");
		
		rc = FCGX_Accept_r(&request);

		if(rc < 0)
		{
			printf("Can not accept new request\n");
			break;
		}

		printf("request is accepted\n");

		struct json_object *obj;

		int inSize = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
		char *str = NULL;
		str = (char*) realloc(str, inSize + 1);
		FCGX_GetStr(str, inSize, request.in);
		str[inSize] = '\0';

		printf("\n---\n%s\n---\n", str);

		obj = json_tokener_parse(str);

		printf("\n---\n%s\n---\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));

		struct json_object *tmp;

		json_object_object_get_ex(obj, "dns", &tmp);
		const char *dns_query = json_object_get_string(tmp);

		json_object_object_get_ex(obj, "type", &tmp);
		const char *type_query = json_object_get_string(tmp);

		tmp = json_object_new_array();

		if (!strcmp(type_query, "A"))
			ares_query(channel, dns_query, ns_c_in, ns_t_a, callback_a, tmp);
		if (!strcmp(type_query, "TXT"))
			ares_query(channel, dns_query, ns_c_in, ns_t_txt, callback_txt, tmp);

		wait_ares(channel);

		// сформировать и вернуть ответ
		json_object_object_add(obj, "answer", tmp);
		printf("\n---\n%s\n---\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
		FCGX_PutS("Content-type: application/json\r\n", request.out);
		FCGX_PutS("\r\n", request.out);
		FCGX_PutS(json_object_to_json_string(obj), request.out); 
		//закрыть текущее соединение 
		FCGX_Finish_r(&request); 
	} 

	printf("hello world \n it's end \n");
	return 0; 
}
