#ifdef CHECK_SD_PLAY_TEST


#include <rtthread.h>
#include "codec_helixmp3.h" 
#include "player_init.h"
#include "player.h"
#include "test_config.h"


#include <finsh.h>
#include <stdio.h>
#include <stdlib.h>
#include "dfs_posix.h"

#include "player_app.h"
#include "player.h"




#include "gpio_pub.h"
#include "bk_rtos_pub.h"

#define	SD_ROOT "/sd"
#define MAX_SONG 1000
//static music_list_t sd_play_list = NULL;
static int event = 0;
static int postion = 0;
static int song_num = 0;
static int song_count=0;


enum{
	OFFLINE_PLAYER_PLAY = (0x1 << 0),
	OFFLINE_PLAYER_PAUSE = (0x1 << 1),
	
	OFFLINE_PLAYER_SONG_NEXT = (0x1 << 2),
	OFFLINE_PLAYER_SONG_PREV = (0x1 << 3),
	OFFLINE_PLAYER_VOLUME_ADD = (0x1 << 4),
	OFFLINE_PLAYER_VOLUME_REDUCE = (0x1 << 5),

	OFFLINE_PLAYER_SD_INSTER_EVENT = (0x1 << 6),
	OFFLINE_PLAYER_SD_REMOVE_EVENT = (0x1 << 7),
	OFFLINE_MODE_CHANGE = (0x01 << 8),
    
	OFFLINE_PLAYER_ALL_EVENT = 0x01FF,
};

////////////SD check//////////////////
#define SD_STATUS_CHECK_PIN				12 
#define SD_DEBOUNCE_COUNT 			    10
#define SD_INSTER_STATUS_CHECK_PIN_LEVEL 0
#define SD_CHECK_SCAN_INTERVAL_MS		 20

typedef enum{
	OFFLINE_STATUS = 0x00,
	ONLINE_STATUS,
}CHECK_STATUS_E;
CHECK_STATUS_E sd_status = OFFLINE_STATUS;
static beken_timer_t sd_check_handle_timer = {0};

static int testplayer_init = 0;
static 	struct rt_event offplay_env;
//static music_list_t offline_music_list = NULL;

static int cur_idx = 1;

static int continuousplay_flag=0;




#define PLAY_LIST_SIZE  3

struct music_item
{
    char *name;
    char *URL;
};

struct music_list 
{
    int list_size;
    int cur_index;
    char *url[PLAY_LIST_SIZE];
};

static rt_mq_t play_list_mq;
static struct music_list test_list = 
{  
    PLAY_LIST_SIZE, 
    0, 
    {
       "/sd/3.mp3",
       "/sd/1.mp3",
       "/sd/2.mp3",   
       //"http://183.193.243.90:9151/mp3/73865964.mp3",
       // "http://183.193.243.90:9151/mp3/108479485.mp3",
       // "http://183.193.243.90:9151/mp3/84986462.mp3",
    }
};
	
int offline_player_send_event(int event)
{	
	int ret;
	if(testplayer_init)
	{
		ret=rt_event_send(&offplay_env,event);
		if(ret==RT_EOK)
		rt_kprintf("rt_event_send OK\r\n");	
	}
	return RT_EOK;
}	
static void mount_sd(void)
{
	/* mount sd card fat partition 1 as root directory */
	if(dfs_mount("sd0", "/sd", "elm", 0, 0) == 0)
		rt_kprintf("SD Card initialized!\n");
	else

	{
		//sd_status = OFFLINE_STATUS;
		rt_kprintf("S0D Card initialzation failed! sd_status = OFFLINE_STATUS\r\n");
	}
}
static void unmount_sd(void)
{
	int ret;
	
	ret = dfs_unmount("/sd");
	rt_kprintf("unmount sd :ret =%d\r\n",ret);
	cur_idx=1;
	continuousplay_flag=0;	
	song_num = 0;
}



static int create_sd_playlist(int cur_song,char *path)

{
	struct dirent  *ent  = NULL;
	DIR *pDir = NULL;
	int cur_num=0;
    int num;
	char abs_path[256];
	char item_name[10];
	struct music_item item;
#if 1	
	pDir = opendir(SD_ROOT);
	if(NULL != pDir)
	{
//		sd_play_list = list_player_items_create();
//		if(sd_play_list != NULL)
		{

//			list_player_set_table_handler(sd_play_list, NULL, RT_NULL);
//		    list_player_mode_set(LISTER_LIST_ONCE_MODE);
			num=0;
			while(1)
			{
				ent = readdir(pDir);
				if(NULL == ent)
				{
					break;
				}
				if(ent->d_type & DT_REG)
				{
					if( 0 == strncasecmp(strchr(ent->d_name,'.'),".mp3",4))
					{
						if(cur_num < cur_song)
						{
							snprintf(path,128,"%s/%s",SD_ROOT,ent->d_name);
							snprintf(abs_path,sizeof(abs_path),"%s/%s",SD_ROOT,ent->d_name);
							snprintf(item_name,sizeof(item_name),"%d",cur_num);
							
							cur_num++;
							song_count++;
							rt_kprintf("====name:%s,url:%s===\r\n",item_name,abs_path);
			       
					//		list_player_item_add(sd_play_list,&item,-1);
						}
						
					}
				}
			}
		}
		closedir(pDir);
	}
#endif
	rt_kprintf("---cur_num = %d---\r\n",cur_num);	
	return cur_num;
}

static void play_switch(void)
{	
	char path[128];
    if (cur_idx< song_num+1)
    {
   //     rt_kprintf("play url: %s\r\n", test_list.url[test_list.cur_index]);
    if(0 != create_sd_playlist(cur_idx,path))
		{
			rt_kprintf("--- create_sd_playlist  ---\r\n");
		//	player_pause();
		//	player_stop();
			player_set_uri(path);
			rt_kprintf("---play url:%s  cur_idx =%d---\r\n",path,cur_idx);
		 //   rt_kprintf("---get url:%s---\r\n",player_get_uri());
			//rt_kprintf("---play state:%d---\r\n",player_get_state());
			player_play();
			cur_idx ++;
		
		}
    }
    else
    {
    	rt_kprintf("---list play is finished---\r\n");
        player_stop();
    }
}

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
	       // rt_mq_send(play_list_mq, &event, sizeof(int));
	       	offline_player_send_event(PLAYER_AUDIO_CLOSED);
	        rt_kprintf("recv event PLAYER_AUDIO_CLOSED\r\n");
			// play_switch();
	        break;
	    case PLAYER_PLAYBACK_FAILED:
	       // rt_mq_send(play_list_mq, &event, sizeof(int));
	        offline_player_send_event(PLAYER_PLAYBACK_FAILED);
	        rt_kprintf("recv event PLAYER_PLAYBACK_FAILED\r\n");
	        break;
	    case PLAYER_PLAYBACK_BREAK:
	      //  rt_mq_send(play_list_mq, &event, sizeof(int));
	       offline_player_send_event(PLAYER_PLAYBACK_BREAK);
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
int sd_msg_handler(int event)
{
	int ret;
	int i;
	char path[128];
	if(event & OFFLINE_PLAYER_SD_INSTER_EVENT)
	{
		rt_kprintf("---sd mount---\r\n");
		mount_sd();
		if(0 != create_sd_playlist(MAX_SONG,path))
		{	

		
			rt_kprintf("---create_sd_playlist0---\r\n");
			song_num=song_count;
			rt_kprintf(" song_num= %d\r\n",song_num);
			//player_set_uri(path);
			//rt_kprintf("---path=%s---\r\n",path);
		//	player_play();
			play_switch();


		}
	}

	if( event & OFFLINE_PLAYER_SD_REMOVE_EVENT )
	{
		rt_kprintf("---sd unmount---\r\n");
		player_stop();
		unmount_sd();
	}
	if( event & PLAYER_AUDIO_CLOSED )
	{ 
	    rt_kprintf("---PLAYER_AUDIO_CLOSED 0---\r\n");
		play_switch();
	}
	return RT_EOK;
}

/*static void play_switch(void)
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
    	rt_kprintf("---list play is finished---\r\n");
        player_stop();
    }
}*/


static void play_stop(void )
{
	 player_stop();
	
}
static void play_prev(void )
{
	player_pause();
	cur_idx-=2;
	if(cur_idx)
	 	play_switch();
  else
  	{
		rt_kprintf("---this is the first song ---\r\n");
		player_play();
  	} 
}
static void play_next(void )
{
	
	 player_pause();
	 play_switch();
}


static void play_list_entry(void *param)
{	
	uint32_t recv_evt;
	int ret;
    rt_err_t result;
    int event;
	rt_thread_delay(500);

  //  play_switch();
    while(1)
    {
		ret = rt_event_recv(&offplay_env,OFFLINE_PLAYER_ALL_EVENT | PLYAER_STATE_CHANGED,
								RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,RT_WAITING_FOREVER, &recv_evt);

		if(ret == RT_EOK)
			{
				rt_kprintf("recv_evt:%x\n",recv_evt);
				sd_msg_handler(recv_evt);
			}
		else
			rt_kprintf("recv_evt failed\r\n");

	/*	result = rt_mq_recv(play_list_mq, &event, sizeof(int), RT_WAITING_FOREVER);
        switch (event)
        {
        case PLAYER_AUDIO_CLOSED:
        case PLAYER_PLAYBACK_FAILED:
        case PLAYER_PLAYBACK_BREAK:
            play_switch();
            break;
            
        default:
            break;
        }*/
    }
}

uint8_t get_sd_check_pin_status(void)
{
	return gpio_input(SD_STATUS_CHECK_PIN);
}

static void sd_check_handle_timer_callback( void * arg )  

{
	int level,ret,count1,count2;
	static uint16 cnt_online = 0,cnt_offline = 0;
	static uint16 linein_online = 0;
	
	if(get_sd_check_pin_status() == SD_INSTER_STATUS_CHECK_PIN_LEVEL)
	{	
		
		
		
	if(OFFLINE_STATUS == sd_status)
		{ 
		  rt_kprintf("sd_status=%d\r\n",sd_status);
			if (cnt_online < SD_DEBOUNCE_COUNT)
	        {
	            cnt_online ++;
			
	        }
		 
			else
			{	count2++;
				sd_status = ONLINE_STATUS;
				rt_kprintf(" sd_status = ONLINE_STATUS\r\n");
				offline_player_send_event(OFFLINE_PLAYER_SD_INSTER_EVENT);
			}
		}
	}
	else 
	{
		if(ONLINE_STATUS == sd_status)
		{ 
			if (cnt_offline < SD_DEBOUNCE_COUNT)
		    
					cnt_online = 0;
					sd_status = OFFLINE_STATUS;
					rt_kprintf(" sd_status = OFFLINE_STATUS\r\n");
					offline_player_send_event(OFFLINE_PLAYER_SD_REMOVE_EVENT);
			
		}
	}
}

int sd_is_online(void)
{
	return (sd_status == ONLINE_STATUS)?1:0;
}

void sd_check_init(void)
{
	int  err;
	
	gpio_config(SD_STATUS_CHECK_PIN, GMODE_INPUT_PULLUP);
	
	
	err = bk_rtos_init_timer(&sd_check_handle_timer, 
							SD_CHECK_SCAN_INTERVAL_MS, 
							sd_check_handle_timer_callback, 
							NULL);
	err = bk_rtos_start_timer(&sd_check_handle_timer);
}

static void play_list(int argc, char **argv)
{
    rt_thread_t tid;
	sd_check_init();

    player_set_volume(50);
    player_set_event_callback(play_music_callback, RT_NULL); 

//	mount_sd();

	rt_event_init(&offplay_env, "env", RT_IPC_FLAG_FIFO);
   play_list_mq = rt_mq_create("play_list_mq", sizeof(int), 10, RT_IPC_FLAG_FIFO);
    if(RT_NULL == play_list_mq)
    {
        rt_kprintf("create play_list_mq failed");
        return;
    }

    tid = rt_thread_create("play_list", play_list_entry, RT_NULL, 1024 * 4, 3, 10);
    if (RT_NULL != tid)
    {
        rt_thread_startup(tid);
		testplayer_init = 1;
    }
}
MSH_CMD_EXPORT(play_list, play_list);
MSH_CMD_EXPORT(play_stop,play_stop);
MSH_CMD_EXPORT(play_next,play_next);
MSH_CMD_EXPORT(play_prev,play_prev);

#endif

