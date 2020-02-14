#ifdef WANTONG_TEST

#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <finsh.h>
#include "vttypes.h"
#include "vtbk.h"
#include "bk_rtos_pub.h"
#include "include.h"
#include "typedef.h"
#include "arm_arch.h"
#include "uart_pub.h"
#include "error.h"
#include "video_transfer.h"
#include "..\flip_check\imgproc.h"
#include "..\flip_check\motion_detect.h"
#include "..\flip_check\jpeg_dec.h"
#include "player.h"
#include "player_app.h"
#include "wantong_httpclient.h"

#include "..\samples_config.h"

#define MAX_JPG_PIC_SIZE (30*1024)
#define PIC_WIDTH  640
#define PIC_HEIGHT 480
#define PIC_RATIO  3

#define DIFF_THRESHOLD_LEVEL 35 //parma1
#define FLIP_MINI_INTERVAL 200 //param2
#define MIN_THREASHOLD 35 //param3
#define MAX_THREASHOLD 100//param4


#define APP_EVENT_START (PLYAER_STATE_CHANGED + 10)
#define APP_EVENT_STOP (PLYAER_STATE_CHANGED + 11)
#define APP_EVENT_EXIT (PLYAER_STATE_CHANGED + 12)

typedef struct _FLIP_CHECK_ST
{
	unsigned char *jpg_src_buf;
	int jpg_file_size;
	rt_mq_t flip_mq;
	rt_mq_t flip_voice;
	rt_sem_t flip_sema;
	rt_sem_t flip_play;
	MotionDetection md;
}FLIP_CHECK_ST;

typedef struct _FLIP_MSG
{
	uint32_t type;
    unsigned char* arg;
    uint32_t size;
}FLIP_MSG;

struct music_list 
{
    int list_size;
    int cur_index;
    char *url[MAX_VOICE_URL];
};

static FLIP_CHECK_ST flip;

static rt_mq_t play_list_mq;
static struct music_list test_list;

//======================= play list ===============
static void play_music_callback(int event, void *user_data)
{
/*
/////////////////////////////////////////////////////////////////////////////////
NOTE: any function in player.h is forbidden to be called in the callback funtion
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ 
*/
    switch(event)
    {
	    case PLAYER_AUDIO_CLOSED:
	        rt_mq_send(play_list_mq, &event, sizeof(int));
	        rt_kprintf("recv event PLAYER_AUDIO_CLOSED\r\n");
	        break;
	    case PLAYER_PLAYBACK_FAILED:
	        rt_mq_send(play_list_mq, &event, sizeof(int));
	        rt_kprintf("recv event PLAYER_PLAYBACK_FAILED\r\n");
	        break;
	    case PLAYER_PLAYBACK_BREAK:
	        rt_mq_send(play_list_mq, &event, sizeof(int));
	        rt_kprintf("recv event PLAYER_PLAYBACK_BREAK\r\n");
	        break;
	    case PLAYER_PLAYBACK_STOP:
	        rt_kprintf("recv event PLAYER_PLAYBACK_STOP\r\n");
	        break;
	   
		default:
			rt_kprintf("recv event :%d\r\n",event);
			break;
    }
}

static void play_switch(void)
{
    if (test_list.cur_index < test_list.list_size)
    {
        rt_kprintf("play url: %s\r\n", test_list.url[test_list.cur_index]);
        player_set_uri(test_list.url[test_list.cur_index]);
        player_play();
        test_list.cur_index++;
    }
    else
    {
    	rt_kprintf("----list play finished---\r\n");
        player_stop();
    }
}
static void play_list_deinit(void)
{
	int i;
	if(play_list_mq)
		rt_mq_delete(play_list_mq);

	for(i=0;i<MAX_VOICE_URL;i++)
	{
		if(test_list.url[i] != RT_NULL)
			rt_free(test_list.url[i]);
	}
}
static void play_list_entry(void *param)
{
    uint8_t loop_flag = 1;
    int event;
	
	test_list.cur_index = 0;
	test_list.list_size = 0;
    while(loop_flag)
    {
        rt_mq_recv(play_list_mq, &event, sizeof(int), RT_WAITING_FOREVER);
        switch (event)
        {
	        case PLAYER_AUDIO_CLOSED:
	        case PLAYER_PLAYBACK_FAILED:
	        case PLAYER_PLAYBACK_BREAK:
			case APP_EVENT_START:
	            play_switch();
	            break;
				
			 case APP_EVENT_EXIT:
			 	player_stop();
			 	loop_flag = 0;
				break;
				
	        default:
	            break;
        }
    }
	play_list_deinit();
}

static void list_player_url(char *url[])
{	
	int i=0,j=0;
	int event = APP_EVENT_START;
	
	player_stop();
	while(url[i] != RT_NULL)
	{
		rt_kprintf("==list player url:%d\r\n",(test_list.url[i] != RT_NULL)?1:0);
		if(test_list.url[i] != RT_NULL)
		{
			rt_free(test_list.url[i]);
			test_list.url[i] = RT_NULL;
		}
		test_list.url[i] = rt_strdup(url[i]);
		j++;

		if(++i >= MAX_VOICE_URL)
			break;
	}
	test_list.cur_index = 0;
	test_list.list_size = j;
	rt_kprintf("cur:%d,size:%d\r\n",test_list.cur_index,test_list.list_size);
	rt_mq_send(play_list_mq, &event, sizeof(int));
}

static int play_list_init(void)
{
    rt_thread_t tid;

    player_set_volume(50);
    player_set_event_callback(play_music_callback, RT_NULL); 

    play_list_mq = rt_mq_create("play_list_mq", sizeof(int), 4, RT_IPC_FLAG_FIFO);
    if(RT_NULL == play_list_mq)
    {
        rt_kprintf("create play_list_mq failed");
        return -1;
    }

    tid = rt_thread_create("play_list", play_list_entry, RT_NULL, 1024, 3, 10);
    if (RT_NULL == tid)
    {
    	rt_kprintf("create playlist thread error!!\r\n");
		return -1;
    }
	rt_thread_startup(tid);
	return 0;
	
}
//====================================



static void flip_msg_send(void *buffer, int size,int type)
{
    int ret = RT_EOK;
    FLIP_MSG msg;
    msg.arg = (unsigned char*)buffer;
    msg.size = size;
	msg.type = type;
    ret = rt_mq_send(flip.flip_mq, (void *)&msg, sizeof(FLIP_MSG));
    if (ret != RT_EOK)
        rt_kprintf("send msg failed \r\n");
}

static int flip_check_init(void)
{

	flip.flip_mq = rt_mq_create("flip_mq", sizeof(FLIP_MSG), 3, RT_IPC_FLAG_FIFO);
	if(NULL == flip.flip_mq)
	{
		rt_kprintf("mq create error!!\r\n");
		return -1;
	}

	flip.flip_sema = rt_sem_create("flip_sema",0,RT_IPC_FLAG_FIFO);
	if(NULL == flip.flip_sema)
	{
		rt_kprintf("sema create error!!\r\n");
		return -1;	
	}

	if(0 != jpg_decoder_init(PIC_WIDTH,PIC_HEIGHT,PIC_RATIO))
		return -1;
	video_buffer_open();
	rt_kprintf("flip chcek init ok\r\n");
	return 0;
}

static void flip_check_deinit(void)
{
	rt_kprintf("flip chcek deinit\r\n");
	if(flip.flip_mq)
		rt_mq_delete(flip.flip_mq);
	if(flip.flip_sema)
		rt_sem_delete(flip.flip_sema);
	jpg_decoder_deinit();
	video_buffer_close();
}

static void get_frame_callbackfun()
{
	rt_sem_release(flip.flip_sema);//start get one frame jpg pic
}

static void flip_check_thread(void *parameter)
{
	FLIP_MSG msg;
	RecognitionResult recognition_result;
	unsigned char *Y_buffer = NULL;
	int ret,pic_width,pic_heigth;
	uint8_t recognize_fail_num,skip;

	for(int i=0;i<sizeof(recognition_result.voice_url)/sizeof(recognition_result.voice_url[0]);i++)
	{
		recognition_result.voice_url[i]=RT_NULL;
	}
	recognition_result.effectSound_url=NULL;
	recognition_result.bgMusic_url=NULL;

	initMotionDetection(&flip.md,MIN_THREASHOLD,MAX_THREASHOLD,DIFF_THRESHOLD_LEVEL,PIC_HEIGHT>>PIC_RATIO,FLIP_MINI_INTERVAL,rt_tick_get());
	rt_sem_release(flip.flip_sema);//start get one frame jpg pic

	skip = 0;
	while(1)
	{	
		long tmp = rt_tick_get();
		if(rt_mq_recv(flip.flip_mq, &msg, sizeof(FLIP_MSG), RT_WAITING_FOREVER) == RT_EOK)
		{
			flip.jpg_src_buf = msg.arg;
			flip.jpg_file_size = msg.size;
			if(0 != flip.jpg_file_size)
			{
				ret = jpg_decoder_fun(flip.jpg_src_buf, &Y_buffer, flip.jpg_file_size);
				/*jpg_decoder success*/
				if(0 == ret)
				{
					jpg_get_pic_size(&pic_width,&pic_heigth);
					ret = getMotionResult(&flip.md,Y_buffer,pic_width>>PIC_RATIO,pic_heigth>>PIC_RATIO,rt_tick_get());
					/* flip check success*/
					if(1 == ret || recognize_fail_num)
					{
						rt_kprintf("\r\n---flipped:%x,%x---\r\n",flip.md.lastMdTimer,flip.md.serverWaitTime);

						ret=wantong_post_resquest(&recognition_result,flip.jpg_src_buf,flip.jpg_file_size,get_frame_callbackfun);
						if(0==ret)
						{	
							if(recognition_result.voice_url[0] != NULL)
							{
								rt_kprintf("\r\n====get url:%s====\r\n\r\n",recognition_result.voice_url[0]);
							    list_player_url(recognition_result.voice_url); 
							}
							recognize_fail_num=0;
						}
						else if(1==ret)
						{
							if(++recognize_fail_num>=3)
								recognize_fail_num=0;
							rt_kprintf("---recognize_fail:%d---\r\n",recognize_fail_num);
						}
						else
						{
							rt_kprintf("recognize repect\r\n");
							recognize_fail_num=0;
						}
						skip = 1;
					}
				}	
			}
			else
			{
				rt_kprintf("---------------------------videos failed\r\n-------------------------------");
			}
			if(1 == skip)
			{
				skip = 0;
			}
			else
			{
				rt_sem_release(flip.flip_sema);
			}
			
			for(int i=0;i<sizeof(recognition_result.voice_url)/sizeof(recognition_result.voice_url[0]);i++)
			{
				if(RT_NULL!=recognition_result.voice_url[i])
				{
					free(recognition_result.voice_url[i]);
					recognition_result.voice_url[i]=RT_NULL;
				}
			}

			if(recognition_result.effectSound_url!=NULL)
			{
				rt_free(recognition_result.effectSound_url);
				recognition_result.effectSound_url=NULL;
			}
			if(recognition_result.bgMusic_url!=NULL)
			{
				rt_free(recognition_result.bgMusic_url);
				recognition_result.bgMusic_url=NULL;
			}	
			    			    	
			if(NULL != Y_buffer)
			{
				rt_free(Y_buffer);
				Y_buffer = 0;
			}

	//		rt_kprintf("delta:%x\r\n",rt_tick_get()-tmp);
		}
		else
		{
			rt_kprintf("get msg error!!\r\n");
		}
	}
}

static void get_pic_thread(void* parameter)
{
	int frame_len;
	unsigned char *jpg_buf = NULL;
	jpg_buf = rt_malloc(MAX_JPG_PIC_SIZE);
	RT_ASSERT(NULL != jpg_buf);
	while(1)
	{	
		if(RT_EOK == rt_sem_take(flip.flip_sema,RT_WAITING_FOREVER))
		{
			frame_len = video_buffer_read_frame(jpg_buf, MAX_JPG_PIC_SIZE);
			flip_msg_send(jpg_buf,frame_len,0);
		}
	}

}

static int wantong_test(int argc,char* argv[])
{
	StatedetParam para = {0, 0, 160, 120, 1, 1};
	rt_err_t result;
	rt_uint8_t number = 0;
	rt_thread_t tid1 = RT_NULL;
	rt_thread_t tid2 = RT_NULL;
	rt_thread_t tid3 = RT_NULL;

    vt_init(&para);
	//接下来要进行各种设置
	vt_setLicenseText("NkIxQzk3MTlDQjBBRDM5MTc3RUY5OUQwMTI1NzhEOTU3NDFEMTlFRjFDM0M5QzRGOUE0RkFCOTdDMkQ1NDlCODYxMUQ5NDNFQTU2MjZDMDA4NEVFNDdDMzA5Q0Y3Q0M5MDA5Rjc5N0VEQkYyNkMwQ0IxMUMxQ0I0NEU5NkJDRjlFQjgxNDhBRUM0QzRCMzQwMjE5RTkzOTZEQjc5MEUwNDc5RkZDMDFFMTJGQkREREZBMjY1RDkyRDFCQ0UzQzE3ODcwRURGMTczRTlBMzk2QThGREQxQTk1ODcyMDVFMUY5MTE3OUNFQzEwODZCMEZCNzJEMTFENzI2NkY1MzU3QjM1NkQxRkZCRDMzQjYwNTkwRUM0NEM2OUY5NzFCQ0Y4RERFMEE5MzU5MDk3NzhBM0Y5RjlCODBFOEVFRDJDRjMwRDM2NzVBOTdDNzI3QUJBQkQ2RDFCNzgyREJBMTg3MkI0NTk1NjI1MTc4OUMwRjI1NkVBMTA0QTE1OUQ1M0E5RkUyRDFEMkU2MTc2Q0ZERUVBMjNBMkY4NEE1ODc5NDRFNUI1RkIzQTkzQkQ0NzA5MkIzNTk2Q0JGM0ZFNzMxMEYwMEZGNTU0RDk1MTlBOEZFRjcwQkFFNjY0QjE4MTkxMDAwQ0NCQjkzRjZBOTcxRDc5RjRDMUI3QzAyQTFBQkQ4Q0ZDRUQ1NzVEQzgzMDZGQ0YxNTg0QkNBRTMzMzMzQThDMUMzNzYyREU5MjAyN0UxQTBERjkxOUI5QzY4MTEz");
	vt_setDeviceID("DamB4dtHjfne");//唯一的设备ID
	vt_setModelType("rtos_test");//设备机型，由玩瞳提供，请联系玩瞳业务员
	//注意这里，要先实现登录函数，然后利用这个设置接口把函数注册进来
	vt_setLoginRequestFunc(customLoginRequest);
	int ret = vt_login();//开始登录
	rt_kprintf("ret:%d Login Result: %s\n", ret,(ret == 0) ? "Success" : "Failed");

	if(ret)
		goto _exit;
	/*释放 解析 url内存*/
	vt_destroy();

	if(RT_EOK != flip_check_init())
		goto _exit1;

	if(0 != play_list_init())
		goto _exit2;
	
     /* create flip thread */
    tid1 = rt_thread_create("flip",
                           flip_check_thread,
                           RT_NULL,
                           1024*4,
                           19,
                           10);
   
    /* create get-jpg-pic thread */
    tid2 = rt_thread_create("get_pic",
                           get_pic_thread,
                           RT_NULL,
                           512,
                           20,
                           10);
    
    if ((RT_NULL == tid1)||(RT_NULL == tid2))
    {
    	rt_kprintf("error!!\r\n");
		if(tid1)
			rt_thread_delete(tid1);
		if(tid2)
			rt_thread_delete(tid2);
		goto _exit2;
    }
	else
	{
        rt_thread_startup(tid1);
		rt_thread_startup(tid2);
		return 0;
	}
_exit2:
	play_list_deinit();
_exit1:
	flip_check_deinit(); 
_exit:
	rt_kprintf("---------------------------\n\n");	
    return 0;

}

MSH_CMD_EXPORT(wantong_test, wantong_test);
#endif
