#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CANONICAL 1;
#define ANSWER 0;


int nameServerExtraction(FILE* fp, char name_server_list[100][100], int* canonical, int* refused) {

		char token[100];

		// Flags

		int authoritative = 1;
		*canonical = 0;
		int subdomain = 0;


		// ------ Extracting Tokens and Analyzing the Output. --------------------------------------------------

		int cnt_nameservers = 0;

		printf("TOKENS:\n");

		while(fscanf(fp, "%s", token)!=EOF) {

			printf("%s \n", token);

			if(strcmp(token, "REFUSED") == 0) {
				*refused = 1;
				return;
			}


			if(strcmp(token, "nameserver") == 0) {
				// Getting nameserver names
				fscanf(fp, "%s", token); // '='
				fscanf(fp, "%s", token); // the name server IP
				strcpy(name_server_list[cnt_nameservers++], token);
			}

			// Handling to choose either NS or A type in the next iteration.
			// Check if Authoritative word is present in the output.
			if(strcmp(token, "Authoritative") == 0) {
				authoritative = 0;
			}

			// if(strcmp(token, "***") == 0) {
			// 	// *** Can't find dss.nitc.ac.in: No answer
			// 	fscanf(fp, "%s", token); // 'Can't'
			// 	fscanf(fp, "%s", token); // 'find'
			// 	fscanf(fp, "%s", token); // domain:
			// 	fscanf(fp, "%s", token); // 'No'
			// 	fscanf(fp, "%s", token); // 'answer'
			// 	subdomain = 1;

			// }

			// Handling Canonical Names
			if(strcmp(token, "canonical") == 0) {
				fscanf(fp, "%s", token); // 'name'
				fscanf(fp, "%s", token); // '='
				fscanf(fp, "%s", canonical_name); // 'canonical name'
				printf("Canonical Name: Search Done, Do type=A search Now at the same Nameserver\n");
				
				*canonical = 1;
				break;
			}
		
		}

		// ------------------------------------------------------------------------------------------------------

		// Printing all the Nameservers Extracted
		printf("All Name severs extracted from output:\n");

		for(int j =0; j < cnt_nameservers; j++) {

			printf("nameserver : %s\n", name_server_list[j]);
		
		}

		return cnt_nameservers;

}


void recursiveDNSQuery(char* req_domain, char* canonicalName) {

	printf("Query: %s\n", req_domain);

	// global server assume g server.

	// char name_server[100] = "g.root-servers.net.";


	// TLD servers and Querying other Name Servers

	int iteration = 0;

	char canonical_name[100];
	int canonical = 0;



	char create_file[20] = "touch output.txt";
	char delete_file[20] = "rm output.txt";
	
	char nslookup_command[400];
	sprintf(nslookup_command, "nslookup -type=ns . >> output.txt");
	
	printf("COMMAND: %s\n", nslookup_command);


	system(create_file);
	system(nslookup_command);

	char name_server_list[100][100] = {0};
	int number_of_nameservers = 0;

	FILE* fp = fopen("output.txt", "r");

	number_of_nameservers = nameServerExtraction(fp, name_server_list, &canonical);

	fclose(fp);
	system(delete_file);


	while(iteration < 4) {

		// ------------------------------------------------------------------------------------------------------
		
		// char create_file[20] = "touch output.txt";
		// char delete_file[20] = "rm output.txt";
		
		// char nslookup_command[400];
		// sprintf(nslookup_command, "nslookup -type=ns %s %s >> output.txt", req_domain, name_server);
		
		// printf("COMMAND: %s\n", nslookup_command);


		// system(create_file);
		// system(nslookup_command);

		// // Opening the nslookup output file.

		// FILE* fp = fopen("output.txt", "r");

 
		for (int l=0; l<number_of_nameservers; l++) {

			char create_file[20] = "touch output.txt";
			char delete_file[20] = "rm output.txt";
			
			char nslookup_command[400];
			sprintf(nslookup_command, "nslookup -type=ns %s %s >> output.txt", req_domain, name_server);
			
			printf("COMMAND: %s\n", nslookup_command);


			system(create_file);
			system(nslookup_command);

			// Opening the nslookup output file.

			FILE* fp = fopen("output.txt", "r");

			int refused = 0;
			
			// CALLL
			number_of_nameservers = nameServerExtraction(fp, name_server_list, &canonical, &refused);
			
			if(refused != 1) {

				break;

			}

		}


		// Reassinging the name_server
		if(cnt_nameservers<2) {

			// If the query is a Canonical Name
			if(canonical != 1) {

				strcpy(name_server, name_server_list[0]);

			} else {

				fclose(fp);
				system(delete_file);
				break;
			
			}

		} else {

			strcpy(name_server, name_server_list[1]);

		}

		printf("NAMESERVER CHOOSEN: %s\n", name_server);

		if(authoritative == 1) {
			printf("Search Done, do type=A search at above NS\n");
			fclose(fp);
			system(delete_file);
			break;
		}


		fclose(fp);
		system(delete_file);


		// ------------------------------------------------------------------------------------------------------

		iteration++;
	}


	printf("Going for Type A Query\n");

	char create_file[20] = "touch output.txt";
	char delete_file[20] = "rm output.txt";

	if(canonical==1) {
		printf("CANONICAL NAME FOR %s is %s\n", req_domain, canonical_name);
		strcpy(req_domain, canonical_name);
		bzero(name_server, sizeof(name_server));
	}

	char nslookup_command[400];
	sprintf(nslookup_command, "nslookup -type=A %s %s >> output.txt", req_domain, name_server);
	
	printf("COMMAND(TYPE-A): %s\n", nslookup_command);


	system(create_file);
	system(nslookup_command);


	FILE* fp = fopen("output.txt", "r");

	char token[100];
	char addresses[100][100]={0};
	int addr_cnt = 0;

	// Extracting the Tokens and Analyzing the Output.

	while(fscanf(fp, "%s", token)!=EOF) {

		printf("%s \n", token);
		
		if(strcmp(token, "Name:") == 0) {
			fscanf(fp, "%s", token); // the query
			printf("%s \n", token);
			fscanf(fp, "%s", token); // 'Address:'
			printf("%s \n", token);
			fscanf(fp, "%s", token); // Address value
			printf("%s \n", token);
			strcpy(addresses[addr_cnt++], token);
		}
	
	}

	printf("-------------------------------------------------\nAddresses Retrieved:\n");

	for(int k=0; k<addr_cnt; k++) {
		printf("Address-%d: %s\n", k+1, addresses[k]);
	}

	printf("-------------------------------------------------\n\n");

	fclose(fp);
	system(delete_file);

	return;
	
} 



int main(int argc, char* argv[]) {

	char req_domain[200];
	strcpy(req_domain, argv[1]);

	char canonical_name[100];

	recursiveDNSQuery(req_domain, canonical_name);


	if(argc <= 1) {

		printf("No Arguments!\n");

	}

	
	return 0;


}
