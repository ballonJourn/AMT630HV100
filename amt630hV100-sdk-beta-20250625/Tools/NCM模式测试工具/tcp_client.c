#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>

#define FRAME_HEADER_MAGIC	0xA5FF5AFF
#define FRAME_TAIL_MAGIC	0x5AFFA5FF
#define BUF_LEN			65536
#define VERSION_LEN		64
#define FILENAME_LEN	128
#define MD5_LEN			32
#define MAX_PATH		256

#define FRAME_MAX_LEN		0x100000
#define FRAME_LEN		1024000

typedef struct {
	uint32_t header;
	uint32_t type;
	uint32_t sub_type;
	uint32_t len;
} HUFrame;

typedef struct {
	char hard_partnumber[VERSION_LEN];
	char hard_version[VERSION_LEN];
	char soft_partnumber[VERSION_LEN];
	char soft_version[VERSION_LEN];
} ReqVersionPosiRsp;

typedef struct {
	char file_name[FILENAME_LEN];
	uint32_t file_length;
	char file_md5[MD5_LEN];
} FileWriteReq;

typedef FileWriteReq FileCheckReq;
typedef FileWriteReq FileCheckRsp;

typedef struct {
	char file_name[FILENAME_LEN];
	uint32_t file_length;
	char file_md5[MD5_LEN];
	uint32_t offset;
} ReqFileWriteRsp;

static char update_file_name[FILENAME_LEN];
static char update_file_md5[MD5_LEN];
static int update_file_length;
static int update_file_offset;
static int sockfd;
static int g_exit = 0;

static int net_rev_state = 0;
static int net_state_len = 0;
static int net_rev_len = 0;
static uint32_t net_frame_type = 0;
static uint32_t net_frame_subtype = 0;
static uint32_t net_frame_rspflag = 0;
static uint32_t net_frame_len = 0;
static int net_toolong_frame = 0;
static uint8_t *net_frame_buf;
static char cur_file_md5[32];
static char cur_file_name[MAX_PATH];

static void netSendFrame(uint32_t type, uint32_t subtype, void *data, uint32_t len)
{
	HUFrame *huframe = NULL;
	uint8_t *tmp;

	printf("send frame type=0x%x, subtype=0x%x, len=%d.\n", type, subtype, len);
	huframe = (HUFrame*)malloc(sizeof(HUFrame) + len + 4);
	if (huframe) {
		huframe->header = FRAME_HEADER_MAGIC;
		huframe->type = type;
		huframe->sub_type = subtype;
		huframe->len = len;
		tmp = (uint8_t*)huframe + sizeof(HUFrame);
		if (len) {
			memcpy(tmp, data, len);
			tmp += len;
		}
		*(uint32_t *)tmp = FRAME_TAIL_MAGIC;
	}

	send(sockfd, huframe, sizeof(HUFrame) + len + 4, 0);
	free(huframe);
}

static void RevFrameHandler(uint32_t type, uint32_t subtype, uint32_t rspflag, uint8_t *data, uint32_t len)
{
	printf("rev frame type=0x%x, subtype=0x%x, rspflag=0x%x, len=%d.\n", type, subtype, rspflag, len);
	switch (type) {
	case 0x70:
		if (subtype == 0x01) {
			ReqVersionPosiRsp *rsp = (ReqVersionPosiRsp*)data;
			printf("get version info:\n");
			printf("hard_partnumber=%s.\n", rsp->hard_partnumber);	
			printf("hard_version=%s.\n", rsp->hard_version);
			printf("soft_partnumber=%s.\n", rsp->soft_partnumber);
			printf("soft_version=%s.\n", rsp->soft_version);
			
			FileWriteReq req = {0};
			strncpy(req.file_name, update_file_name, FILENAME_LEN);
			memcpy(req.file_md5, update_file_md5, MD5_LEN);
			req.file_length = update_file_length;
			netSendFrame(0x71, 0x01, &req, sizeof(req));
		}
		break;
	case 0x71:
		if (subtype == 0x01) {
			ReqFileWriteRsp *rsp = (ReqFileWriteRsp*)data;
			if (!rspflag || len != sizeof(ReqFileWriteRsp)) {
				g_exit = 1;
				break;
			}
			if (!strcmp(rsp->file_name, update_file_name) && !memcmp(rsp->file_md5, update_file_md5, MD5_LEN) &&
					rsp->file_length == update_file_length) {
				update_file_offset = 0;
				FILE *fp = fopen(update_file_name, "rb");
				if (!fp) {
					g_exit = 1;
					break;								
				}
				unsigned char *buf = malloc(FILENAME_LEN + 12 + FRAME_LEN);
				if (!buf) {
					g_exit = 1;
					break;
				}
				int size = update_file_length > FRAME_LEN ? FRAME_LEN : update_file_length;
				memcpy(buf, update_file_name, FILENAME_LEN);
				*(uint32_t *)(buf + FILENAME_LEN) = update_file_length;
				*(uint32_t *)(buf + FILENAME_LEN + 4) = size;
				*(uint32_t *)(buf + FILENAME_LEN + 8) = 0;
				fseek(fp, update_file_offset, SEEK_SET);
				fread(buf + FILENAME_LEN + 12, 1, size, fp);
				netSendFrame(0x71, 0x02, buf, FILENAME_LEN + 12 + size);
				free(buf);
				fclose(fp);
			} else {
				g_exit = 1;
				break;			
			}
		} else if (subtype == 0x02) {
			ReqFileWriteRsp *rsp = (ReqFileWriteRsp*)data;
			if (!rspflag || len != sizeof(ReqFileWriteRsp)) {
				printf("frame write fail\n");
				/* 模拟中控收到否定应答时会发送一个长度和md5清零的帧 */
				FileWriteReq req = {0};
				memset(req.file_name, 0, FILENAME_LEN);
				memset(req.file_md5, 0, MD5_LEN);
				req.file_length = 0;
				netSendFrame(0x71, 0x01, &req, sizeof(req));	
				g_exit = 1;
				break;						
			}
			if (!strcmp(rsp->file_name, update_file_name) && !memcmp(rsp->file_md5, update_file_md5, MD5_LEN) &&
					rsp->file_length == update_file_length && rsp->offset == update_file_offset) {
				update_file_offset += FRAME_LEN;
				if (update_file_offset >= update_file_length) {
					printf("file transfer finished.\n");
					netSendFrame(0x71, 0x03, NULL, 0);
				} else {
					int size = update_file_length - update_file_offset;
					size = size > FRAME_LEN ? FRAME_LEN : size;
					FILE *fp = fopen(update_file_name, "rb");
					if (!fp) {
						g_exit = 1;
						break;								
					}
					unsigned char *buf = malloc(FILENAME_LEN + 12 + size);
					if (!buf) {
						g_exit = 1;
						break;
					}
					memcpy(buf, update_file_name, FILENAME_LEN);
					*(uint32_t *)(buf + FILENAME_LEN) = update_file_length;
					*(uint32_t *)(buf + FILENAME_LEN + 4) = size;
					*(uint32_t *)(buf + FILENAME_LEN + 8) = update_file_offset;
					fseek(fp, update_file_offset, SEEK_SET);
					fread(buf + FILENAME_LEN + 12, 1, size, fp);
					netSendFrame(0x71, 0x02, buf, FILENAME_LEN + 12 + size);
					free(buf);
					fclose(fp);
				}
			} else {
				g_exit = 1;
				break;			
			}			
		} else if (subtype == 0x03) {
			FileCheckReq req = {0};
			strcpy(req.file_name, update_file_name);
			memcpy(req.file_md5, update_file_md5, MD5_LEN);
			req.file_length = update_file_length;
			printf("req file check\n");
			netSendFrame(0x71, 0x04, &req, sizeof(req));	
		} else if (subtype == 0x04) {
			FileCheckRsp *rsp = (FileCheckRsp*)data;
			if (!rspflag || len != sizeof(FileWriteReq)) {
				printf("file check fail\n");
				g_exit = 1;
				break;
			}
			if (!strcmp(rsp->file_name, update_file_name) && !memcmp(rsp->file_md5, update_file_md5, MD5_LEN) &&
					rsp->file_length == update_file_length) {
				printf("req install condition check\n");
				netSendFrame(0x73, 0x01, NULL, 0);
			}
		}
		break;
	case 0x73:
		if (subtype == 0x01) {
			if (!rspflag || len != 256) {
				printf("install condition check fail\n");
				g_exit = 1;
				break;
			}
			uint32_t install_type = 0;
			netSendFrame(0x74, 0x01, &install_type, sizeof(install_type));
			printf("wait for install finished...\n");					
		}
		break;
	case 0x74:
		if (subtype == 0x01) {
			if (!rspflag || len != 256) {
				printf("install fail\n");
				g_exit = 1;
				break;
			}
			printf("install ok\n");
			g_exit = 1;		
		}
		break;		
	}
}

static void RevDataHandler(uint8_t *buf, int32_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		switch (net_rev_state) {
		case 0:		//head receive
			if (net_state_len == 0 && buf[i] == 0xff) {
				net_state_len++;
			} else if (net_state_len == 1 && buf[i] == 0x5a) {
				net_state_len++;
			} else if (net_state_len == 2 && buf[i] == 0xff) {
				net_state_len++;
			} else if (net_state_len == 3 && buf[i] == 0xa5) {
				net_rev_state++;
				net_state_len = 0;
				net_frame_type = 0;
				net_frame_subtype = 0;
				net_frame_rspflag = 0;
				net_frame_len = 0;
			} else if (buf[i] == 0xff) {
				net_state_len = 1;
			} else {
				net_state_len = 0;
			}
			break;
		case 1:		//type receive
			net_frame_type |= buf[i] << (8 * net_state_len);
			if (++net_state_len == 4) {
				net_rev_state++;
				net_state_len = 0;
			}
			break;
		case 2:		//sub_type receive
			net_frame_subtype |= buf[i] << (8 * net_state_len);
			if (++net_state_len == 4) {
				net_rev_state++;
				net_state_len = 0;
			}
			break;
		case 3:		//rsp_flag receive
			net_frame_rspflag |= buf[i] << (8 * net_state_len);
			if (++net_state_len == 4) {
				net_rev_state++;
				net_state_len = 0;
			}
			break;
		case 4:		//data len receive
			net_frame_len |= buf[i] << (8 * net_state_len);
			if (++net_state_len == 4) {
				if (net_frame_len > FRAME_MAX_LEN) {
					printf("Invalid NCM frame len 0x%x.\n", net_frame_len);
					net_toolong_frame = 1;
				} else {
					net_toolong_frame = 0;
				}
				if (net_frame_len == 0)
					net_rev_state += 2;
				else
					net_rev_state++;
				net_rev_len = 0;
				net_state_len = 0;
			}
			break;
		case 5:		//data receive
			if (net_rev_len < FRAME_MAX_LEN)
				net_frame_buf[net_rev_len] = buf[i];
			if (++net_rev_len == net_frame_len)
				net_rev_state++;
			break;
		case 6:		//tail receive
			if (net_state_len == 0 && buf[i] == 0xff) {
				net_state_len++;
			} else if (net_state_len == 1 && buf[i] == 0xa5) {
				net_state_len++;
			} else if (net_state_len == 2 && buf[i] == 0xff) {
				net_state_len++;
			} else if (net_state_len == 3 && buf[i] == 0x5a) {
				RevFrameHandler(net_frame_type, net_frame_subtype, net_frame_rspflag, net_frame_buf, net_frame_len);
				net_rev_state = 0;
				net_state_len = 0;
			} else {
				net_state_len = 0;
			}
			break;
		default:
			break;
		}
	}
}

void print_hex(unsigned char *data, int len, const char* tag)
{
    unsigned long i, j, l;
    unsigned char tmp_str[140];
    unsigned char tmp_str1[10];

    for (i = 0; i < len; i += 16) {
        int n ;
        tmp_str[0] = '\0';
        n = i ;

        for (j = 0; j < 4; j++) {
            l = n % 16;

            if (l >= 10)
                tmp_str[3 - j] = (unsigned char)('A' + l - 10);
            else
                tmp_str[3 - j] = (unsigned char)(l + '0');

            n >>= 4 ;
        }

        tmp_str[4] = '\0';
        strcat((char *) tmp_str, ": ");

        /*
           Output the hex bytes
         */
        for (j = i; j < (i + 16); j ++) {
            int m ;

            if (j < len) {
                m = ((unsigned int)((unsigned char) * (data + j))) / 16 ;

                if (m >= 10)
                    tmp_str1[0] = 'A' + (unsigned char) m - 10;
                else
                    tmp_str1[0] = (unsigned char) m + '0';

                m = ((unsigned int)((unsigned char) * (data + j))) % 16 ;

                if (m >= 10)
                    tmp_str1[1] = 'A' + (unsigned char) m - 10;
                else
                    tmp_str1[1] = (unsigned char) m + '0';

                tmp_str1[2] = '\0';
                strcat((char *) tmp_str, (char *) tmp_str1);
                strcat((char *) tmp_str, " ");
            } else {
                strcat((char *) tmp_str, "   ");
            }
        }

        strcat((char *) tmp_str, "  ");
        l = strlen((char *) tmp_str);

        /* Output the ASCII bytes */
        for (j = i; j < (i + 16); j++) {
            if (j < len) {
                char c = * (data + j);

                if (c < ' ' || c > 'z') {
                    c = '.';
                }

                tmp_str[l ++] = c;
            } else {
                tmp_str[l ++] = ' ';
            }
        }

        tmp_str[l ++] = '\r';
        tmp_str[l ++] = '\n';
        tmp_str[l ++] = '\0';

        printf("%s\r\n", (const char *) tmp_str);
    }
}

unsigned long get_file_size(const char *path)  
{  
    unsigned long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}

int main(int argc,char *argv[])
{
    char sendbuffer[BUF_LEN];
    char recvbuffer[BUF_LEN];
    struct sockaddr_in server_addr;
    struct hostent *host;
    int portnumber,nbytes;
	int len;

    if (argc != 4) {
		fprintf(stderr,"Usage :%s hostname portnumber filename\a\n", argv[0]);
		exit(1);
    }

    if ((host = gethostbyname(argv[1])) == NULL) {
		herror("Get host name error\n");
		exit(1);
    }

    if ((portnumber = atoi(argv[2])) < 0)
    {
		fprintf(stderr,"Usage:%s hostname portnumber\a\n", argv[0]);
		exit(1);
    }

	strncpy(update_file_name, argv[3], FILENAME_LEN);
	update_file_length = get_file_size(update_file_name);
	char cmdstring[16 + FILENAME_LEN] = "md5sum ";
	strcat(cmdstring, update_file_name) ;
	FILE *fpipe = popen(cmdstring, "r");
	fread(update_file_md5, 1, MD5_LEN,  fpipe);
	pclose(fpipe);

    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
		fprintf(stderr,"Socket Error:%s\a\n", strerror(errno));
		exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portnumber);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr,"Connect error:%s\n",strerror(errno));
		exit(1);
    }

	net_frame_buf = malloc(FRAME_MAX_LEN);
	if (!net_frame_buf) {
		fprintf(stderr,"malloc fail:%s\n",strerror(errno));
		exit(1);	
	}
	
	netSendFrame(0x70, 0x01, NULL, 0);
    while (!g_exit) {
  		len = recv(sockfd, recvbuffer, BUF_LEN, 0);
		//print_hex(recvbuffer, len, NULL);
		RevDataHandler(recvbuffer, len);
	}

	free(net_frame_buf);
	close(sockfd);
	exit(0);
}
