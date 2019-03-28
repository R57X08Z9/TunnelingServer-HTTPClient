#include <stdio.h>
#include <stdlib.h>
#include "modul.h"

int main() {

	char *type_q = NULL;
	type_q = calloc(4, sizeof(char));

	char *dns_q = NULL;
	dns_q = calloc(255, sizeof(char));

	for (;;) {

		printf("---");
		
		scanf("%*[^\n]");

		printf("\ntype=");
		scanf("%3s", type_q);

		scanf("%*[^\n]");
		
		printf("dns=");
		scanf("%254s", dns_q);
		
		int status = sent_dns_query(dns_q, type_q);

	}

	printf("\nexit");

	return 0;
}