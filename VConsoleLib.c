// Enable debug code
#define DEBUG 1
#include "VConsoleLib.h"

int dread(VConConn* conn, void *dest, size_t size) {
	int ret = recv(conn->socketfd, dest, size, 0);
#if DEBUG
	if(ret > 0) {
		// Write all traffic from this VConsoleConnection to a dump file; helps with debugging
		fwrite(dest,1,ret,conn->dumpfile);
		fflush(conn->dumpfile);
	}
#endif
	return ret;
}

VConConn* VCConnect(char* hostname, char* port) {
	if(port == NULL) {
		// Default VConConn port
		port = "29000";
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
	if(ret == NULL) {
		fprintf(stderr,"calloc failure\n");
		return NULL;
	}
	struct addrinfo* res;
	if(getaddrinfo(hostname,port,NULL,&res)!=0) {
		fprintf(stderr,"Failure to getaddrinfo for hostname \"%s\"\n",hostname);
		return NULL;
	}
	memcpy(&(ret->server_address),res->ai_addr,res->ai_addrlen);
	ret->sockaddrlen = res->ai_addrlen;
	freeaddrinfo(res);

	ret->socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(ret->socketfd < 0) {
		fprintf(stderr,"Error opening socket\n");
		free(ret);
		return NULL;
	}
	if(connect(ret->socketfd, &(ret->server_address), ret->sockaddrlen) < 0) {
		fprintf(stderr,"Failure to connect\n");
		shutdown(ret->socketfd, SHUT_RDWR);
		free(ret);
		return NULL;
	}
#ifdef WIN32
	unsigned long iMode = 1;
	ioctlsocket(ret->socketfd, FIONBIO, &iMode);
#else
	int flags = fcntl(ret->socketfd, F_GETFL, 0);
	fcntl(ret->socketfd, F_SETFL, flags | O_NONBLOCK);
#endif

#if DEBUG
	ret->dumpfile = fopen("dump1.bin","w");
#endif
	return ret;
}

//#define LOADAS(x) fprintf(stderr,"Reading in a " #x "\n"); x chnk; int p = dread(conn, &chnk, sizeof(x)); while(p == EAGAIN || p == EWOULDBLOCK) { fprintf(stderr,"Info: waiting on completion of " #x " chunk\n"); sleep(1); p = dread(conn, &chnk, sizeof(x));} if(p < sizeof(x)) { fprintf(stderr,"Couldn't get whole " #x "\n"); return false;}
#define LOADAS(x) fprintf(stderr,"Interpreting a " #x "\n"); x *chunk = (x*)&(((parsedchunk*)*outputbuf)->body);

// returns -1 on error, positive on success, 0 if no data ready
// if positive, *outputbuf is now a pointer to a parsedchunk.
// otherwise, it is NULL.
int VCReadChunk(VConConn* conn, char** outputbuf) {
	if(outputbuf == NULL) {
		fprintf(stderr,"Warning: outputbuf is null!\n");
		return -1;
	}
	*outputbuf = NULL;
	if(conn == NULL) {
		fprintf(stderr,"Warning: Tried to read from non-existant VConsole Connection!\n");
		return -1;
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
	header.version = ntohl(header.version);
	header.length = ntohs(header.length);
	header.pipe_handle = ntohs(header.pipe_handle);
	// Allocate space for the incoming chunk
	(*outputbuf) = (char*)malloc(header.length);
	if(*outputbuf == NULL) {
		fprintf(stderr,"Error: Malloc failure while parsing chunk!\n");
		return -1;
	}
	// Read in the incoming chunk
	int p = dread(conn, &(((parsedchunk*)(*outputbuf))->body), header.length - 12);
	if(p < header.length - 12) {
		free(*outputbuf);
		*outputbuf = NULL;
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		fprintf(stderr,"Socket Read Error (%i)\n",n);
		return -1;
	}
	// If we managed to read in the incoming chunk, copy the header
	memcpy(&(((parsedchunk*)(*outputbuf))->header),&header,sizeof(VConChunk));
	// Debug
	printf("%c%c%c%c\n",header.type[0],header.type[1],header.type[2],header.type[3]);
	if(strncmp(header.type,"AINF",4)==0) {
		//LOADAS(VConChunkAddonInfo);
	} else if(strncmp(header.type,"ADON",4)==0) {
		LOADAS(VConChunkAddonIdentifier);
		chunk->unknown = ntohs(chunk->unknown);
		chunk->namelen = ntohs(chunk->namelen);
		printf("%hi %hi\n",sizeof(VConChunkAddonIdentifier),chunk->namelen);
		printf("%.*s\n",chunk->namelen,chunk->name);
		fflush(stdout);
	} else if(strncmp(header.type,"CHAN",4)==0) {
		LOADAS(VConChunkChannelsHeader);
		chunk->numchannels = ntohs(chunk->numchannels);
		printf("%hi channels:\n", chunk->numchannels);
		for(int i=0;i<chunk->numchannels;i++) {
			chunk->channels[i].channel_id = ntohl(chunk->channels[i].channel_id);
			chunk->channels[i].unknown1 = ntohl(chunk->channels[i].unknown1);
			chunk->channels[i].unknown2 = ntohl(chunk->channels[i].unknown2);
			chunk->channels[i].verbosity_default = ntohl(chunk->channels[i].verbosity_default);
			chunk->channels[i].verbosity_current = ntohl(chunk->channels[i].verbosity_current);
			chunk->channels[i].text_RGBA_override = ntohl(chunk->channels[i].text_RGBA_override);
			printf("%.8x (%i %i %i %i %.8x): %s\n",chunk->channels[i].channel_id,chunk->channels[i].unknown1,chunk->channels[i].unknown2,chunk->channels[i].verbosity_default,chunk->channels[i].verbosity_current,chunk->channels[i].text_RGBA_override,chunk->channels[i].name);
		}
	} else if(strncmp(header.type,"PRNT",4)==0) {
		LOADAS(VConChunkPrint);
		chunk->channel_id=ntohl(chunk->channel_id);
		printf("%.8x : %.*s\n",chunk->channel_id,(header.length-28)-1,chunk->message);
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
		chunk->padding = ntohs(chunk->padding);
		// print it all out
		printf("%.8x %.8x %.4x %f %f %s\n",chunk->unknown,chunk->flags,chunk->padding,chunk->rangemin,chunk->rangemax,chunk->variable_name);
		fflush(stdout);
	} else if(strncmp(header.type,"CFGV",4)==0) {
		//LOADAS(VConChunkCfg);
	} else if(strncmp(header.type,"PPCR",4)==0) {
		//LOADAS(VConChunkPPCR);
	} else {
		fprintf(stderr,"Unknown chunk type: %c%c%c%c\n",header.type[0],header.type[1],header.type[2],header.type[3]);
		free(*outputbuf);
		*outputbuf = NULL;
		return -1;
	}
	return 1;
}
void VCExecute(VConConn* conn, char* command) {
	VConChunk header;
	header.type[0] = 'C'; header.type[1] = 'M'; header.type[2] = 'N'; header.type[3] = 'D';
	header.version = htonl(0x00d20000);
	header.length = htons(strlen(command)+12+1); //add header length and null terminator
	header.pipe_handle = (short)0;
	send(conn->socketfd, &header, sizeof(header), 0);
	send(conn->socketfd, command, strlen(command)+1, 0);
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
	shutdown(conn->socketfd, SHUT_RDWR);
	free(conn);
	return;
}
