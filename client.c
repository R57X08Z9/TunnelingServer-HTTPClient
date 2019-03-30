#include <stdio.h>
#include <stdlib.h>
#include "modul.h"
#include <string.h>

int main(int ac, char *av[]) {

	char *name_input_file = NULL;
	char *name_output_file = NULL;

	FILE *input_file = NULL;
	FILE *output_file = NULL;

	if (ac > 2) {
		name_input_file = malloc(sizeof(av[1]));
		strcpy(name_input_file, av[1]);
		name_output_file = malloc(sizeof(av[2]));
		strcpy(name_output_file, av[2]);
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

	char *type_q = NULL;
	type_q = calloc(4, sizeof(char));

	char *dns_q = NULL;
	dns_q = calloc(255, sizeof(char));

	while (!feof(input_file)) {

		fscanf(input_file, "%3s", type_q);

		fscanf(input_file, "%254s", dns_q);
	
		struct response *resp = calloc(1, sizeof(struct response));
		int status = sent_dns_query(resp, dns_q, type_q);

		printf("%d\n", status);

		if (status == 0) {
			printf("%s, %s, %d, ", resp->type, resp->dns, resp->status);
			fprintf(output_file, "%s, %s, %d, ", resp->type, resp->dns, resp->status);
			if (resp->status == 0) {
				for (int i = 0; i < resp->count_string_answer - 1; i++) {
					fprintf(output_file, "%s, ", resp->answer[i]);
					printf("%s, ", resp->answer[i]);
				}
				fprintf(output_file, "%s\n", resp->answer[resp->count_string_answer - 1]);
				printf("%s\n", resp->answer[resp->count_string_answer - 1]);
			} else {
				fprintf(output_file, "%s", resp->error);
				printf("%s\n", resp->error);
			}
		}
	}

	fclose(input_file);
	fclose(output_file);

	printf("\nexit\n");

	return 0;
}