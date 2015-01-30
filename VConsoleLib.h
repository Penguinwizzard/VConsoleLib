#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#ifdef WIN32
#include<WinSock2.h>
#include<stdint.h>

#define SHUT_RDWR SD_BOTH
#else
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#endif
#include<stdbool.h>
#include<errno.h>
#include<fcntl.h>

typedef struct {
	int socketfd;
	socklen_t sockaddrlen;
	struct sockaddr server_address;
#if DEBUG
	FILE* dumpfile; //Used with the DEBUG flag set
#endif
} VConConn;

typedef struct {
	VConConn header;
	uint8_t body[0];
} parsedchunk;

// Generic chunk
#ifdef WIN32
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((__packed__)) {
#endif
	char type[4];
	uint32_t version; // always 0x00d20000?
	uint16_t length; // includes header length
	uint16_t pipe_handle; // always 0x0000?
} VConChunk;

#ifdef WIN32
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((__packed__)) {
#endif
	// 58-byte struct
	uint32_t channel_id;
	uint32_t unknown1; // only seen to be 0, 1, or 2
	uint32_t unknown2; // only seen to be 0, 1, 2, or 8
	uint32_t verbosity_default;
	uint32_t verbosity_current;
	uint32_t text_RGBA_override;
	char name[34];
} VConChannel;

#ifdef WIN32
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((__packed__)) {
#endif
	//ADON
	uint16_t unknown; //nbo
	uint16_t namelen; //nbo
	char name[1]; // variable-length
} VConChunkAddonIdentifier;

#ifdef WIN32
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((__packed__)) {
#endif
	//AINF
	uint32_t unknown1;
	uint32_t unknown2;
	uint32_t unknown3;
	uint32_t unknown4;
	uint32_t unknown5;
	uint32_t unknown6;
	uint32_t unknown7;
	uint32_t unknown8;
	uint32_t unknown9;
	uint32_t unknown10;
	uint32_t unknown11;
	uint32_t unknown12;
	uint32_t unknown13;
	uint32_t unknown14;
	uint32_t unknown15;
	uint32_t unknown16;
	uint32_t unknown17;
	uint32_t unknown18;
	uint32_t unknown19;
	uint8_t padding;
} VConChunkAddonInfo;

#ifdef WIN32
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((__packed__)) {
#endif
	//CHAN
	uint16_t numchannels; //nbo
	VConChannel channels[1]; //is actually many, just using this notation
} VConChunkChannelsHeader;

#ifdef WIN32
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((__packed__)) {
#endif
	//PRNT
	uint32_t channel_id; //What channel to print the message on
	uint8_t unknown[24];
	uint8_t message[1]; // variable-length
} VConChunkPrint;

// ConVar flags - there's probably a better list somewhere, this is just what I figured out in an hour or so
// icvar: entries are taken from iconvar.h - thanks for the tip, psychonic
// since those are source1 mappings, there's also a little mark next to them: + if high confidence, ? if unknown, - if doubted
#define VCCVAR_FLAG_CLICEX 0x80000000 /* clientcmd_can_execute */
#define VCCVAR_FLAG_UNK6   0x40000000 /* as seen in quit */
#define VCCVAR_FLAG_UNK5   0x20000000 /* icvar?: server_cannot_query; as seen in rcon_address */
#define VCCVAR_FLAG_SRVCEX 0x10000000 /* icvar+: server_can_execute flag, as seen in slot9 */
#define VCCVAR_FLAG_UNK4   0x08000000
#define VCCVAR_FLAG_UNK3   0x04000000 /* as seen in r_gpu_mem_stats */
#define VCCVAR_FLAG_ACFRTR 0x02000000 /* icvar?: accessible from threads; used for debugging material system thread convars (pwiz: this mapping I feel unsure about; it may not have been preserved exactly; as seen in net_queue_trace) */
#define VCCVAR_FLAG_SS     0x01000000 /* splitscreen (thanks asherkin!) */
#define VCCVAR_FLAG_MTSYTR 0x00800000 /* icvar?: this cvar is read by the material system's thread */
#define VCCVAR_FLAG_NOCONN 0x00400000 /* icvar?: cannot be changed by a client connected to a server */
#define VCCVAR_FLAG_RELTEX 0x00200000 /* icvar?: reload textures if value changes */
#define VCCVAR_FLAG_RELMAT 0x00100000 /* icvar?: reload materials if value changes */
#define VCCVAR_FLAG_CVAR   0x00080000 /* pwiz: is a variable that can take on values, not a command */
#define VCCVAR_FLAG_UNK2   0x00040000 /* icvar-: USED TO BE SS_ADDED, CURRENT USE UNKNOWN */
#define VCCVAR_FLAG_NODEMO 0x00020000 /* icvar+: don't save this cvar in demos; as seen in listRecentNPCSpeech */
#define VCCVAR_FLAG_DEMO   0x00010000 /* icvar+: save this cvar in demos; as seen in dsp_dist_max */
#define VCCVAR_FLAG_UNK1   0x00008000 /* icvar-: USED TO BE SS, CURRENT USE UNKNOWN */
#define VCCVAR_FLAG_CHEAT  0x00004000 /* cheat */
#define VCCVAR_FLAG_REPLIC 0x00002000 /* replicated */
#define VCCVAR_FLAG_NOPRNT 0x00001000 /* icvar+: never try to print this cvar. (pwiz: as seen in mat_texture_list_all) */
#define VCCVAR_FLAG_NOLOG  0x00000800 /* icvar?: if this is a server cvar, don't log changes to the log file */
#define VCCVAR_FLAG_PRONLY 0x00000400 /* icvar?: printableonly; cannot contain unprintable characters */
#define VCCVAR_FLAG_USER   0x00000200 /* changes client's info string */
#define VCCVAR_FLAG_NOTIFY 0x00000100 /* notifies players when changed */
#define VCCVAR_FLAG_ARCHIV 0x00000080 /* saved */
#define VCCVAR_FLAG_SPONLY 0x00000040 /* icvar?: cannot be changed by clients on a multiplayer server */
#define VCCVAR_FLAG_PROTEC 0x00000020 /* icvar+: protected; it's a server CVar, but the contents aren't sent because it's a password or similar. Sends 1 if non-zero, 0 otherwise. example is tv_password */
#define VCCVAR_FLAG_HIDDEN 0x00000010 /* icvar+: hidden; doesn't appear in find or autocomplete. pwiz note: yes, it does appear in autocomplete. */
#define VCCVAR_FLAG_CLIENT 0x00000008 /* icvar+: defined by client dll */
#define VCCVAR_FLAG_GAME   0x00000004 /* icvar+: defined by game dll */
#define VCCVAR_FLAG_DEVELP 0x00000002 /* icvar+: Hidden in released products. Flag removed automatically if ALLOW_DEVELOPMENT_CVARS is defined. */
#define VCCVAR_FLAG_UNREG  0x00000001 /* icvar?: If this flag is set, don't add to linked list, etc */

#ifdef WIN32
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((__packed__)) {
#endif
	char variable_name[64];
	uint32_t unknown;
	uint32_t flags; //see VCCVAR_FLAG_*
	float rangemin;
	float rangemax;
	uint8_t padding;
} VConChunkCVar;

#ifdef WIN32
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((__packed__)) {
#endif
	uint8_t unknown[129];
} VConChunkCfg;

#ifdef WIN32
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((__packed__)) {
#endif
	uint8_t unknown[14];
	uint16_t messagelen;
	char message[21];
} VConChunkPPCR;

VConConn* VCConnect(char* hostname, char* port);
int VCReadChunk(VConConn* conn, char** outputbuf);
void VCExecute(VConConn* conn, char* command);
void VCDestroy(VConConn* conn);
