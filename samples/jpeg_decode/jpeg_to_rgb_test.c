#include "include.h"
#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "arm_arch.h"
#include "uart_pub.h"
#include "bk_rtos_pub.h"
#include "error.h"
#include "tjpgd.h"
#include "flash.h"
#include "..\samples_config.h"

#ifdef JPEG_TO_RGB_TEST
#define JPG_FILE_SIZE (12*1024)
#define JPG_FILE_START_ADDR 0x1A0000
#define JPG_FILE_END_ADDR (JPG_FILE_SIZE+0x1A0000)
#define SCALE	0	/* Output scaling 0:1/1, 1:1/2, 2:1/4 or 3:1/8 */

static uint8_t *rgb_buf;
static uint32_t line_wbyte;
static uint32_t rd_ptr = JPG_FILE_START_ADDR;
static uint32_t rgb_buf_size;

#define TRANSFER_2_PC

#ifdef TRANSFER_2_PC
#include <sys/socket.h> /*使用BSD socket,要包含头文件socket.h*/
#include "netdb.h"

struct net_worker
{
    char *url;
    int port;
    int sock;
};
static char url[64];
static struct net_worker net_wk;

static int tcp_client_init(struct net_worker *client)
{
	struct linger so_linger;
	struct sockaddr_in server_addr;
    struct hostent *host;
	
    host = gethostbyname(client->url);

  	if ((client->sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        rt_kprintf("[tcp]:Socket error\n");;
        return -RT_ERROR;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(client->port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));
	
    if (connect(client->sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        rt_kprintf("[tcp]:Connect fail!\n");
        closesocket(client->sock);
        return -RT_ERROR;
    }
	 return RT_EOK;
}
static void tcp_client_close(struct net_worker *client)
{
	if(client->sock > 0)
		closesocket(client->sock);
}
#endif





/* User defined input function */
uint16_t input_func (
	JDEC* jd,		/* Decompression object */
	uint8_t* buff,	/* Pointer to the read buffer (0:skip) */
	uint16_t ndata	/* Number of bytes to read/skip */
)
{
	if(rd_ptr >= JPG_FILE_END_ADDR)
		return 0xffff;
	
	if(NULL != buff)
	{
		flash_read(buff,ndata,rd_ptr);
	}
	rd_ptr += ndata;
	return ndata;
}

/* User defined output function */

uint16_t output_func (
	JDEC* jd,		/* Decompression object */
	void* bitmap,	/* Bitmap data to be output */
	JRECT* rect		/* Rectangular region to output */
)
{
	uint32_t ny, nbx, xc, wd;
	uint8_t *src, *dst;
	
	/* Put progress indicator */
	if (rect->left == 0) 
	{
		//rt_kprintf("----%lu%%---\r\n", (rect->top << jd->scale) * 100UL / jd->height);
	}

	nbx = (rect->right - rect->left + 1) * 3;	/* Number of bytes a line of the rectangular */
	ny = rect->bottom - rect->top + 1;			/* Number of lines of the rectangular */
	src = (uint8_t*)bitmap;
	wd = line_wbyte;
	
	dst = rgb_buf + rect->top*wd +rect->left*3;
	do 
	{	/* Copy the rectangular to the frame buffer */
		xc = nbx;
		do 
		{
			*dst++ = *src++;
		} while (--xc);
		dst += wd - nbx;
	} while (--ny);
	return 1;	/* Continue to decompress */
}


#define WORK_AREA_SIZE 4096
void jpeg_dec_test(int argc, char* argv[])
{
    void *work;       /* Pointer to the decompressor work area */
    JDEC jdec;        /* Decompression object */
    JRESULT res;      /* Result code of TJpgDec API */
	int wsize;
    int ret;
	uint32_t xb, xs, ys;
	
	if(argc != 3)
		return;
	rt_kprintf("---start jpg dec----\r\n");
    /* Allocate a work area for TJpgDec */
    work = malloc(WORK_AREA_SIZE);
	if(NULL == work)
	{
		return;
	}
	rd_ptr = JPG_FILE_START_ADDR;
    /* Prepare to decompress */
    res = jd_prepare(&jdec, input_func, work, WORK_AREA_SIZE, NULL);
    if (res == JDR_OK) 
	{
#ifdef TRANSFER_2_PC
		net_wk.url = url;
		strcpy(url,argv[1]);
		net_wk.port = atoi(argv[2]);
		if(RT_EOK != tcp_client_init(&net_wk))
		{
			rt_kprintf("connect net fail\r\n");
			goto exit;
		}
#endif
        /* Ready to dcompress. Image info is available here. */
        rt_kprintf("Image dimensions: %u by %u. %u bytes used.\r\n", jdec.width, jdec.height, WORK_AREA_SIZE - jdec.sz_pool);
		xs = jdec.width >> SCALE;
		ys = jdec.height >> SCALE;
		xb = (xs * 3 + 3)&(~3);
		line_wbyte = xb;
		rgb_buf_size = xb * ys;
		
		rgb_buf = malloc(rgb_buf_size);
		rt_kprintf("xs:%x,ys:%x,xb:%x,rgb buffer size:%x",xs,ys,xb,rgb_buf_size);
		if(rgb_buf == NULL)
		{
			rt_kprintf("malloc rgb buffer error!!!\r\n");
			goto exit0;
		}
		
		rt_kprintf("start decode tick:%x\r\n",rt_tick_get());
        res = jd_decomp(&jdec, output_func, 0); /* Start to decompress with 1/1 scaling */
		rt_kprintf("end decode tick:%x\r\n",rt_tick_get());
        if (res == JDR_OK)
		{
	#ifdef TRANSFER_2_PC
			ret = send(net_wk.sock, rgb_buf, rgb_buf_size, 0);
			if(ret != rgb_buf_size)
			{
				rt_kprintf("transmit error!!!!\r\n");
			}
			else
	#endif
			 /* Decompression succeeded. You have the decompressed image in the frame buffer here. */
            	rt_kprintf("OK  \r\n");

        } 
		else 
		{
            rt_kprintf("Failed to decompress: rc=%d\r\n", res);
        }

        free(rgb_buf);    /* Discard frame buffer */

    } 
	else 
	{
        rt_kprintf("Failed to prepare: rc=%d\r\n", res);
    }
exit0:
#ifdef TRANSFER_2_PC
	tcp_client_close(&net_wk);
exit:
#endif
    free(work);             /* Discard work area */

}

MSH_CMD_EXPORT(jpeg_dec_test,jpeg dec test);
#endif
