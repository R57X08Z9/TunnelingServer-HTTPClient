#include <stdio.h>
#include <stdlib.h>
#include "dns_query_lib.h"
#include <string.h>

int main(int ac, char *av[]) {

	char *name_input_file = NULL;
	char *name_output_file = NULL;

	FILE *input_file = NULL;
	FILE *output_file = NULL;

	if (ac > 2) {

		name_input_file = av[1];
		name_output_file = av[2];

		input_file = fopen(name_input_file, "r");
		if (input_file == NULL) {
			printf("file %s can't open to read\n", name_input_file);
			return 0;
		}
		output_file = fopen(name_output_file, "w");
		if (output_file == NULL) {
			printf("file %s can't open to writh\n", name_output_file);
			return 0;
		}
		
	} else {
		printf("need more argumen\n");
		return 0;
	}

	char type_q[4];
	char dns_q[255];

	while (!feof(input_file)) {

		fscanf(input_file, "%3s", type_q);

		fscanf(input_file, "%254s", dns_q);
	
		dns_response_t *resp = calloc(1, sizeof(dns_response_t));
		send_dns_query(resp, dns_q, type_q, "localhost", 8000);

		printf("%s, %s, %d, ", resp->type, resp->dns, resp->status);
		fprintf(output_file, "%s, %s, %d, ", resp->type, resp->dns, resp->status);

		if (resp->status == 0) {
			for (int i = 0; i < resp->count_string_answer; i++) {
				fprintf(output_file, "%s ", resp->answer[i]);
				printf("%s ", resp->answer[i]);
			} 
		} else if (resp->status == 1) {
			fprintf(output_file, "%s", resp->error);
			printf("%s", resp->error);
		} else {
			fprintf(output_file, "incorrect input_output data");
			printf("incorrect input_output data");
		}

		free_dns_response_s(resp);
		printf("\n");
		fprintf(output_file, "\n");
	}

	fclose(input_file);
	fclose(output_file);

	printf("\nexit\n");

	return 0;
}
