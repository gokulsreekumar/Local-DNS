#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CANONICAL_NAME -10
#define ANSWER 0

#define PORT 8080


int serverStart() {

    int socket;

    struct sockaddr_in si_server, si_client;
    
    int i, slen = sizeof(si_client) , recv_len;
    char buf[BUFLEN];
    
    //Create a UDP socket
    if ((*socket=socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        error("socket");
    }

    // Address Initialization
    memset((char *) &si_server, 0, sizeof(si_server));
    
    // Make Address 
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORT);
    si_server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //Bind address to socket
    if(bind(socket_udp , (struct sockaddr*)&si_server, sizeof(si_server) ) == -1)
    {
        close(socket_udp);
        error("bind");
    }
    printf("Socket binding Succeful...\n");

    return socket;

}


int tokenExtractor(FILE * fp, char name_server_list[100][100],
    int * authoritative, int * canonical, char * canonical_name,
    int * refused) {

    char token[100];

    // Flags
    * canonical = 0;
    int subdomain = 0;

    // ------ Extracting Tokens and Analyzing the Output.
    // --------------------------------------------------

    int cnt_nameservers = 0;

    printf("\n-------------- TOKENS -----------------\n");

    while (fscanf(fp, "%s", token) != EOF) {
        printf("%s \n", token);

        if (strcmp(token, "REFUSED") == 0) {
            * refused = 1;
            return -1;
        }

        if (strcmp(token, "nameserver") == 0) {
            // Getting nameserver names
            fscanf(fp, "%s", token); // '='
            fscanf(fp, "%s", token); // the name server IP
            strcpy(name_server_list[cnt_nameservers++], token);
        }

        // Handling to choose either NS or A type in the next iteration.
        // Check if Authoritative word is present in the output.
        if (strcmp(token, "Authoritative") == 0) {
            * authoritative = 0;
        }

        // Handling Canonical Names
        if (strcmp(token, "canonical") == 0) {
            fscanf(fp, "%s", token); // 'name'
            fscanf(fp, "%s", token); // '='
            fscanf(fp, "%s", canonical_name); // 'canonical name'
            printf( "Canonical Name DETECTED: Search Done, Do type search Now at the same "
                "Nameserver\n");

            *canonical = 1;
            return -10;
        }
    }
    printf("---------------------------------------\n\n");
    // ------------------------------------------------------------------------------------------------------

    return cnt_nameservers;
}

void storeAndPrintFile(FILE* fp, char* data_string) {

        char token;
        int cnt = 0;
        // Extracting the Tokens and Analyzing the Output.
        while ((token=fgetc(fp)) != EOF) {
            data_string[cnt++] = token;
            printf("%c", token);
        }
        data_string[cnt] = '\0';

}

void recursiveDNSQuery(char * req_domain, char * canonicalName, char* type) {

    printf("Query: %s\n", req_domain);

    // global server assume g server.

    char name_server[100] = "g.root-servers.net.";

    // TLD servers and Querying other Name Servers

    int iteration = 0;

    char canonical_name[100];
    int canonical = 0;

    char saved_ns_address[100];

    while (iteration < 4) {
        // ------------------------------------------------------------------------------------------------------

        char create_file[20] = "touch output.txt";
        char delete_file[20] = "rm output.txt";

        char nslookup_command[400];
        sprintf(nslookup_command, "nslookup -type=ns %s %s >> output.txt",
            req_domain, name_server);

        printf("\nCOMMAND: %s\n\n", nslookup_command);

        system(create_file);
        system(nslookup_command);

        // Opening the nslookup output file.

        FILE * fp = fopen("output.txt", "r");

        char name_server_list[100][100] = {
            0
        };
        int number_of_nameservers = 0;

        int authoritative = 1;
        int refused = 0;

        // CALLL
        number_of_nameservers = tokenExtractor(fp, name_server_list, & authoritative, & canonical,
                                    canonical_name, & refused);

        // Canonical Name for the domain Exists
        if(number_of_nameservers == -10) {
            printf( "PROCEEDING WITH NS: %s\n", name_server);
            break;
        }

        // Printing all the Nameservers Extracted
        printf("\nAll Name severs extracted from output:\n");

        for (int j = 0; j < number_of_nameservers; j++) {
            printf("nameserver : %s\n", name_server_list[j]);
        }

        fclose(fp);
        system(delete_file);

        // FOR LOOP FOR CHECKING THE NAMESERVERS
        int l = 0;
        int chosen = 0;
        for (l = 0; l < number_of_nameservers; l++) {
            char create_file1[20] = "touch output1.txt";
            char delete_file1[20] = "rm output1.txt";

            char nslookup_command1[400];
            sprintf(nslookup_command1, "nslookup -type=ns %s %s >> output1.txt",
                req_domain, name_server_list[l]);

            printf("TEST - COMMAND: %s\n", nslookup_command1);

            system(create_file1);
            system(nslookup_command1);

            // Opening the nslookup output file.

            FILE * fp1 = fopen("output1.txt", "r");

            int refused = 0;
            int tmp_no_of_nameservers;
            char tmp_name_server_list[100][100];

            // CALLL
            tmp_no_of_nameservers =
                tokenExtractor(fp1, tmp_name_server_list, & authoritative, & canonical,
                    canonical_name, & refused);

            fclose(fp1);
            system(delete_file1);

            if (refused != 1) {
                chosen = l;
                break;
            }

            printf("TEST RESULT = REFUSED\n");
        }

        // Reassinging the name_server
        strcpy(name_server, name_server_list[chosen]);

        printf("\nNAMESERVER REASSIGNMENT TO: %s\n\n", name_server);

        // STOPPING ITERATION
        if (authoritative == 1) {
            printf("Search Done, do type=A search at above NS\n");
            break;
        }

        // ------------------------------------------------------------------------------------------------------

        iteration++;
    }

    printf("Going for Last Query\n");

    char create_file[20] = "touch output.txt";
    char delete_file[20] = "rm output.txt";


    // If Query was A type but CNAME needed
    if (canonical == 1 && strcmp(type, "CNAME")!=0) {
        printf("CANONICAL NAME FOR %s is %s\n", req_domain, canonical_name);
        strcpy(req_domain, canonical_name);
        bzero(name_server, sizeof(name_server));
    }

    char nslookup_command[400];


    // DEPEPNDING ON EACH TYPE DO DIFFERENTLY
    if(strcmp(type, "A")==0) {

        // THE COMMAND
        sprintf(nslookup_command, "nslookup -type=%s %s %s >> output.txt", type, req_domain,
            name_server);

        printf("COMMAND(TYPE-A): %s\n", nslookup_command);

        // EXCEUTION OF COMMAND
        system(create_file);
        system(nslookup_command);

        FILE * fp = fopen("output.txt", "r");


        char token[100];
        int addr_cnt = 0;
        char addresses[100][100] = {
            0
        };
        

        // Extracting the Tokens and Analyzing the Output.
        while (fscanf(fp, "%s", token) != EOF) {
            printf("%s \n", token);

            if (strcmp(token, "Name:") == 0) {
                fscanf(fp, "%s", token); // the query
                printf("%s \n", token);
                fscanf(fp, "%s", token); // 'Address:'
                printf("%s \n", token);
                fscanf(fp, "%s", token); // Address value
                printf("%s \n", token);
                strcpy(addresses[addr_cnt++], token);
            }
        }

        // MAKING THE OUTPUT
        printf(
            "\n\n-------------------------------------------------\nAddresses "
            "Retrieved:\n");

        for (int k = 0; k < addr_cnt; k++) {
            printf("Address-%d: %s\n", k + 1, addresses[k]);
        }
        printf("-------------------------------------------------\n\n");

        fclose(fp);
        system(delete_file);

    } else if(strcmp(type, "AAAA")==0) {


        // THE COMMAND
        sprintf(nslookup_command, "nslookup -type=%s %s %s >> output.txt", type, req_domain,
            name_server);

        printf("COMMAND(TYPE-AAAA): %s\n", nslookup_command);

        // EXCEUTION OF COMMAND
        system(create_file);
        system(nslookup_command);

        int addr_cnt = 0;
        char addresses[100][100] = {
            0
        };

        FILE * fp = fopen("output.txt", "r");

        char token[100];


        // Extracting the Tokens and Analyzing the Output.
        while (fscanf(fp, "%s", token) != EOF) {
            printf("%s ", token);
            if (strcmp(token, "has") == 0) {
                fscanf(fp, "%s", token); // 'AAAA'
                printf("%s \n", token);
                fscanf(fp, "%s", token); // 'address'
                printf("%s \n", token);
                fscanf(fp, "%s", token); // Address value
                printf("%s \n", token);
                strcpy(addresses[addr_cnt++], token);
            }
        }

        // MAKING THE OUTPUT
        printf(
            "\n\n-------------------------------------------------\nAddresses "
            "Retrieved:\n");

        for (int k = 0; k < addr_cnt; k++) {
            printf("Address-%d: %s\n", k + 1, addresses[k]);
        }
        printf("-------------------------------------------------\n\n");

        fclose(fp);
        system(delete_file);



    } else if(strcmp(type, "CNAME")==0) {
        
        // THE COMMAND
        sprintf(nslookup_command, "nslookup -type=%s %s %s >> output.txt", type, req_domain,
            name_server);

        printf("COMMAND(TYPE-CNAME): %s\n", nslookup_command);

        // EXCEUTION OF COMMAND
        system(create_file);
        system(nslookup_command);

        int addr_cnt = 0;
        char addresses[100][100] = {
            0
        };

        FILE * fp = fopen("output.txt", "r");

        char token[100];


        // Extracting the Tokens and Analyzing the Output.
        while (fscanf(fp, "%s", token) != EOF) {
            printf("%s ", token);
            if (strcmp(token, "canonical") == 0) {
                fscanf(fp, "%s", token); // 'name'
                printf("%s \n", token);
                fscanf(fp, "%s", token); // '='
                printf("%s \n", token);
                fscanf(fp, "%s", token); // Address value
                printf("%s \n", token);
                strcpy(addresses[addr_cnt++], token);
            }
        }

        // MAKING THE OUTPUT
        printf(
            "\n\n-------------------------------------------------\nCanonical Names "
            "Retrieved:\n");

        for (int k = 0; k < addr_cnt; k++) {
            printf("Canonical Names %d: %s\n", k + 1, addresses[k]);
        }
        printf("-------------------------------------------------\n\n");

        fclose(fp);
        system(delete_file);


    } else if(strcmp(type, "NS")==0) {

       // THE COMMAND
        sprintf(nslookup_command, "nslookup -type=%s %s %s >> output.txt", type, req_domain,
            name_server);

        printf("COMMAND(TYPE-NS): %s\n", nslookup_command);

        // EXCEUTION OF COMMAND
        system(create_file);
        system(nslookup_command);

        int addr_cnt = 0;
        char addresses[100][100] = {
            0
        };

        FILE * fp = fopen("output.txt", "r");

        char token;

        storeAndPrintFile(fp);

        fclose(fp);
        system(delete_file); 
    }

    
    return;
}

int main(int argc, char * argv[]) {


    if (argc <= 1) {
        printf("Too Few Arguments!\n");
    }

    char req_domain[200];
    strcpy(req_domain, argv[1]);

    int recv_len;

    char canonical_name[100];

    char type[15] = "A";
    recursiveDNSQuery(req_domain, canonical_name, type);


    int socket = serverStart();

    while() {

        
    }



    return 0;
}