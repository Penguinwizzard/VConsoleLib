#include"VConsoleLib.h"
#include<unistd.h>

void main() {
	VConConn* vc = VCConnect("localhost", 29000);
	if(vc == NULL) {
		fprintf(stderr,"Error: Failure to connect to VConsole port\n");
		return;
	}
	sleep(1);
	parsedchunk* pc;
	while(VCReadChunk(vc,(char**)&pc)>0) {
		free(pc);
	}
	sleep(1);
	while(VCReadChunk(vc,(char**)&pc)>0) {
		free(pc);
	}
	fprintf(stderr,"SENDING COMMAND\n");
	VCExecute(vc,"listdemo");
	fprintf(stderr,"SENT COMMAND\n");
	while(VCReadChunk(vc,(char**)&pc)>=0) {
		sleep(1);
		if(pc != NULL)
			free(pc);
	}
	VCDestroy(vc);
}
