#include "VConsoleLib.h"
#define DEBUG 1

int dread(VConConn* conn, void *dest, size_t size) {
	int ret = read(conn->socketfd, dest, size);
#if DEBUG
	if(ret > 0) {
		// Write all traffic from this VConsoleConnection to a dump file; helps with debugging
		fwrite(dest,1,ret,conn->dumpfile);
		fflush(conn->dumpfile);
	}
#endif
	return ret;
}

VConConn* VCConnect(char* hostname, int port) {
	if(port == 0) {
		// Default VConConn port
		port = 29000;
	}
	if(hostname == NULL) {
		fprintf(stderr,"Attempted to connect to NULL hostname!\n");
		return NULL;
	}
	struct hostent *server;
	server = gethostbyname(hostname);
	if(server == NULL) {
		fprintf(stderr,"Couldn't resolve hostname \"%s\"!\n",hostname);
		return NULL;
	}
	VConConn *ret = (VConConn*)calloc(1,sizeof(VConConn));
	ret->server_address.sin_family = AF_INET;
	memcpy((void*)&(ret->server_address.sin_addr.s_addr), (void*)(server->h_addr), server->h_length);
	ret->server_address.sin_port = htons(port);

	ret->socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(ret->socketfd < 0) {
		fprintf(stderr,"Error opening socket\n");
		free(ret);
		return NULL;
	}
	if(connect(ret->socketfd, (struct sockaddr *) &(ret->server_address), sizeof(ret->server_address)) < 0) {
		fprintf(stderr,"Failure to connect\n");
		close(ret->socketfd);
		free(ret);
		return NULL;
	}
	int flags = fcntl(ret->socketfd, F_GETFL, 0);
	fcntl(ret->socketfd, F_SETFL, flags | O_NONBLOCK);
#if DEBUG
	ret->dumpfile = fopen("dump1.bin","w");
#endif
	return ret;
}

//#define LOADAS(x) fprintf(stderr,"Reading in a " #x "\n"); x chnk; int p = dread(conn, &chnk, sizeof(x)); while(p == EAGAIN || p == EWOULDBLOCK) { fprintf(stderr,"Info: waiting on completion of " #x " chunk\n"); sleep(1); p = dread(conn, &chnk, sizeof(x));} if(p < sizeof(x)) { fprintf(stderr,"Couldn't get whole " #x "\n"); return false;}
#define LOADAS(x) fprintf(stderr,"Interpreting a " #x "\n"); x *chunk = (x*)rawchunk;

// returns negative on error, positive on success, 0 on no data ready
int VCReadChunk(VConConn* conn) {
	if(conn == NULL) {
		fprintf(stderr,"Warning: Tried to read from non-existant VConsole Connection!\n");
		return false;
	}
	VConChunk header;
	int n = dread(conn, &header, sizeof(header));
	if(n < (int)sizeof(VConChunk)) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		fprintf(stderr,"Socket Read Error (%i)\n",n);
		return -1;
	}
	// Yeah, we don't know what it is - but we might as well convert it
	header.unknown = ntohl(header.unknown);
	header.length = ntohs(header.length);
	header.padding = ntohs(header.padding);
	// Allocate space for the incoming chunk
	char rawchunk[header.length - 12];
	int p = dread(conn, rawchunk, header.length - 12);
	if(p < header.length - 12) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		fprintf(stderr,"Socket Read Error (%i)\n",n);
		return -1;
	}
	printf("%c%c%c%c\n",header.type[0],header.type[1],header.type[2],header.type[3]);
	if(strncmp(header.type,"AINF",4)==0) {
		LOADAS(VConChunkAddonInfo);
	} else if(strncmp(header.type,"ADON",4)==0) {
		LOADAS(VConChunkAddonIdentifier);
		chunk->unknown = ntohl(chunk->unknown);
		chunk->namelen = ntohs(chunk->namelen);
		printf("%i %hi\n",sizeof(VConChunkAddonIdentifier),chunk->namelen);
		printf("%.*s\n",chunk->namelen,chunk->name);
		fflush(stdout);
	} else if(strncmp(header.type,"CHAN",4)==0) {
		LOADAS(VConChunkChannelsHeader);
		chunk->numchannels = ntohs(chunk->numchannels);
		printf("%hi channels:\n", chunk->numchannels);
		for(int i=0;i<chunk->numchannels;i++) {
			printf("%s\n",chunk->channels[i].name);
		}
	} else if(strncmp(header.type,"PRNT",4)==0) {
		LOADAS(VConChunkPrint);
		printf("PRINTING\n");
		printf("%.*s\n",(header.length-28)-1,chunk->message);
		fflush(stdout);
	} else if(strncmp(header.type,"CVAR",4)==0) {
		LOADAS(VConChunkCVar);
		//don't know what it is, but we might as well flip it
		//flip it good
		chunk->unknown = ntohl(chunk->unknown);
		//we re-order the flags just for convenience of cross-reference
		chunk->flags = ntohl(chunk->flags);
		//there really must be a better way to ntohf
		uint32_t temp;
		temp = *((uint32_t*)(&(chunk->rangemin)));
		temp = ntohl(temp);
		chunk->rangemin = *((float*)(&(temp)));
		temp = *((uint32_t*)(&(chunk->rangemax)));
		temp = ntohl(temp);
		chunk->rangemax = *((float*)(&(temp)));
		// print it all out
		printf("%.8x %.8x %f %f %s\n",chunk->unknown,chunk->flags,chunk->rangemin,chunk->rangemax,chunk->variable_name);
		fflush(stdout);
	} else if(strncmp(header.type,"CFGV",4)==0) {
		LOADAS(VConChunkCfg);
	} else if(strncmp(header.type,"PPCR",4)==0) {
		LOADAS(VConChunkPPCR);
	} else {
		fprintf(stderr,"Unknown chunk type: %c%c%c%c\n",header.type[0],header.type[1],header.type[2],header.type[3]);
		return -1;
	}
	return 1;
}
void VCExecute(VConConn* conn, char* command) {
	VConChunk header;
	header.type[0] = 'C'; header.type[1] = 'M'; header.type[2] = 'N'; header.type[3] = 'D';
	header.unknown = htonl(0x00d20000);
	header.length = htons(strlen(command)+12+1); //add header length and null terminator
	header.padding = (short)0;
	write(conn->socketfd, &header, sizeof(header));
	write(conn->socketfd, command, strlen(command)+1);
	return;
}
void VCDestroy(VConConn* conn) {
	if(conn == NULL) {
		fprintf(stderr,"Warning: Tried to destroy non-existant VConsole Connection!\n");
		return;
	}
#if DEBUG
	fclose(conn->dumpfile);
#endif
	close(conn->socketfd);
	free(conn);
	return;
}
