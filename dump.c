#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdbool.h>

int main() {
	struct hostent *server = gethostbyname("localhost");
	if(server == NULL) {
		printf("problem resolving address\n");
		return;
	}
	struct sockaddr_in server_address;
	memset(&server_address,0,sizeof(server_address));
	server_address.sin_family = AF_INET;
	memcpy((void*)&(server_address.sin_addr.s_addr), (void*)(server->h_addr),server->h_length);
	server_address.sin_port = htons(29000);
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd < 0) {
		printf("socket error\n");
		return;
	}
	if(connect(sfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		printf("Connect error\n");
		return;
	}
	char temp;
	FILE* out = fopen("dump.bin","w");
	printf("about to read...\n");
	int counter = 0;
	while(read(sfd, &temp, 1)>0) {
		if(counter%0x100 ==0)
			printf("0x%.8x\n",counter);
		counter++;
		fwrite(&temp,1,1,out);
		fflush(out);
	}
	fclose(out);
}
