#include"VConsoleLib.h"

void main() {
	VConConn* vc = VCConnect("localhost", 29000);
	sleep(1);
	while(VCReadChunk(vc)>0);
	sleep(1);
	while(VCReadChunk(vc)>0);
	fprintf(stderr,"SENDING COMMAND\n");
	VCExecute(vc,"listdemo");
	fprintf(stderr,"SENT COMMAND\n");
	while(VCReadChunk(vc)>=0) { sleep(1);}
	VCDestroy(vc);
}
