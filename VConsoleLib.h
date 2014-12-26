#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdbool.h>
#include<errno.h>
#include<fcntl.h>

typedef struct {
	int socketfd;
	struct sockaddr_in server_address;
	FILE* dumpfile; //Used with the DEBUG flag set
} VConConn;

// Generic chunk
typedef struct __attribute__((__packed__)) {
	char type[4];
	uint32_t unknown; // always 0x00d20000?
	uint16_t length; // includes header length
	uint16_t padding; // always 0x0000?
} VConChunk;

typedef struct __attribute__((__packed__)) { // 58-byte struct
	uint32_t channel_id;
	uint8_t header[20];
	char name[34];
} VConChannel;

typedef struct __attribute__((__packed__)) { //ADON
	uint32_t unknown; //nbo
	uint16_t namelen; //nbo
	char name[1]; // variable-length
} VConChunkAddonIdentifier;

typedef struct __attribute__((__packed__)) { //AINF
	uint8_t unknown[77];
} VConChunkAddonInfo;

typedef struct __attribute__((__packed__)) { //CHAN
	uint16_t numchannels; //nbo
	VConChannel channels[1]; //is actually many, just using this notation
} VConChunkChannelsHeader;

typedef struct __attribute__((__packed__)) { //PRNT
	uint32_t channel_id; //What channel to print the message on
	uint8_t unknown[24];
	uint8_t message[1]; // variable-length
} VConChunkPrint;

// ConVar flags - there's probably a better list somewhere, this is just what I figured out in an hour or so
#define VCCVAR_FLAG_CLICEX 0x80000000 /* clientcmd_can_execute */
#define VCCVAR_FLAG_UNK5   0x40000000 /* as seen in quit */
#define VCCVAR_FLAG_UNK3   0x20000000 /* as seen in rcon_address */
#define VCCVAR_FLAG_SRVCEX 0x10000000 /* server_can_execute flag and slot9 */
#define VCCVAR_FLAG_UNK0   0x04000000 /* as seen in r_gpu_mem_stats */
#define VCCVAR_FLAG_UNK8   0x02000000 /* as seen in net_queue_trace */
#define VCCVAR_FLAG_SS     0x01000000 /* splitscreen (thanks asherkin!) */
#define VCCVAR_FLAG_CVAR   0x00080000
#define VCCVAR_FLAG_UNK1   0x00020000 /* as seen in listRecentNPCSpeech */
#define VCCVAR_FLAG_UNK6   0x00010000 /* as seen in dsp_dist_max and dsp_facingaway */
#define VCCVAR_FLAG_CHEAT  0x00004000
#define VCCVAR_FLAG_REPLIC 0x00002000
#define VCCVAR_FLAG_UNK4   0x00001000 /* as seen in mat_texture_list_all */
#define VCCVAR_FLAG_USER   0x00000200
#define VCCVAR_FLAG_NOTIFY 0x00000100
#define VCCVAR_FLAG_ARCHIV 0x00000080
#define VCCVAR_FLAG_UNK7   0x00000020 /* as seen in tv_password */
#define VCCVAR_FLAG_UNK2   0x00000010 /* as seen in dota_camera_map_bounds_shrink_at_max_zoom and nav_analyze_scripted and vprof_loadhitstore_scale - also hides command from 'find' list*/
#define VCCVAR_FLAG_CLIENT 0x00000008
#define VCCVAR_FLAG_GAME   0x00000004
#define VCCVAR_FLAG_NOFIND 0x00000002 /* command doesn't show up in find results, but does autocomplete */

typedef struct __attribute__((__packed__)) {
	char variable_name[64];
	uint32_t unknown;
	uint32_t flags; //see VCCVAR_FLAG_*
	float rangemin;
	float rangemax;
	uint8_t padding;
} VConChunkCVar;

typedef struct __attribute__((__packed__)) {
	uint8_t unknown[129];
} VConChunkCfg;

typedef struct __attribute__((__packed__)) {
	uint8_t unknown[14];
	uint16_t messagelen;
	char message[21];
} VConChunkPPCR;

VConConn* VCConnect(char* hostname, int port);
int VCReadChunk(VConConn* conn);
void VCExecute(VConConn* conn, char* command);
void VCDestroy(VConConn* conn);
