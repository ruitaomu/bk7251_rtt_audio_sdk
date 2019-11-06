#include <stdio.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "include.h"
#include "shell.h"
#include "NuanceWakeupAPI.h"
#include "nuance_nhxz_blob_tab.h" //"你好小智" BLOB文件
#include "utils.h"
#include "audio_device.h"
#include "board.h"
#include "test_wakeup.h"
#include "..\samples_config.h"


#define	TEST_WAKEUP_16K_RATE		16000
#define	TEST_WAKEUP_MIC_VOLUME		65

#define NWA_FRAME_MS 10
#define NWA_FRAME_SAMPLES (NWA_FRAME_MS * 16) // 16k, 16bit, mono audio data.

#define ACTION_WAKEUP_CONTINUE	1
#define ACTION_WAKEUP_CANCLE	2

#define PCM_BUFF_WAKEUP_ACC		4
#define PCM_BUFF_WAKEUP_COUNT	8
#define PCM_BUFF_WAKEUP_SIZE 	(NWA_FRAME_SAMPLES * sizeof(short))

#define PCM_MP_BLOCK_CNT 	(PCM_BUFF_WAKEUP_COUNT)
#define PCM_MP_BLOCK_SIZE (PCM_BUFF_WAKEUP_ACC*PCM_BUFF_WAKEUP_SIZE)

#ifdef WAKEUP_TEST

typedef struct record_msg
{
    uint32_t type;
    uint32_t arg;
    uint32_t len;
} record_msg_t;

typedef struct record_manager
{
	struct rt_mempool mp;
    rt_mq_t msg;
	rt_sem_t ack_sem;
	
    volatile int action;
    int mp_block_size; /* sample / 50 * 2 ==> 8k:320  16k:640*/
    int mp_cnt;

	uint8_t *pcmbuff;
	uint16_t *pAudio;
	uint16_t *elvisHeap ;
}record_manager_t;


static record_manager_t *recorder;
static uint8_t *mempool = NULL;

static int record_msg_send(record_manager_t *record, void *buffer, int type, int len)
{
    int ret = RT_EOK;
    record_msg_t msg;

    msg.type = type;
    msg.arg = (uint32_t)buffer;
    msg.len = len;

    ret = rt_mq_send(record->msg, (void *)&msg, sizeof(record_msg_t));
    if (ret != RT_EOK)
        rt_kprintf("[record]:send msg failed \r\n");
	return ret;
	
}

static void free_recorder(void)
{
	if(RT_NULL != recorder)
	{
		rt_mp_detach(&(recorder->mp));
		if(mempool)
			sdram_free(mempool);
		rt_mq_delete(recorder->msg);
		rt_sem_delete(recorder->ack_sem);


		if(recorder->pcmbuff)
			rt_free(recorder->pcmbuff);
		
		if(recorder->pAudio)
			rt_free(recorder->pAudio);

		if(recorder->elvisHeap)
			rt_free(recorder->elvisHeap);
		rt_free(recorder);
		recorder = RT_NULL;
	}
}

static int init_recorder(void)
{
	if(RT_NULL != recorder)
		return RT_ERROR;
   	recorder = rt_malloc(sizeof(record_manager_t));
    if(RT_NULL == recorder)
    {
        rt_kprintf("[record]:malloc memory for recorder manager \n");
        return RT_ERROR;
    }
    memset(recorder, 0, sizeof(record_manager_t));
    /* initialize mempool */
    recorder->mp_block_size = PCM_MP_BLOCK_SIZE;
    recorder->mp_cnt = PCM_MP_BLOCK_CNT;
    mempool = sdram_malloc(recorder->mp_block_size * recorder->mp_cnt);
	if(RT_NULL == mempool)
	{
		rt_kprintf("mp error!!\r\n");
		return RT_ERROR;
	}
    rt_mp_init(&(recorder->mp), "record_mp", mempool, recorder->mp_block_size * recorder->mp_cnt, recorder->mp_block_size);

	/* initialize msg queue */
    recorder->msg = rt_mq_create("net_msg", sizeof(record_msg_t), 30, RT_IPC_FLAG_FIFO);
	if(RT_NULL == recorder->msg)
		return RT_ERROR;
	/*initialize semaphore */
	recorder->ack_sem = rt_sem_create("ack_sema",0,RT_IPC_FLAG_FIFO);
	if(RT_NULL == recorder->ack_sem)
		return RT_ERROR;
	
	/*pcm buffer*/
	recorder->pcmbuff = sdram_malloc(PCM_BUFF_WAKEUP_ACC * PCM_BUFF_WAKEUP_SIZE);
	if( RT_NULL == recorder->pcmbuff)
	{
		return RT_ERROR;
	}

	recorder->pAudio = (uint16_t *)sdram_malloc(PCM_BUFF_WAKEUP_SIZE);
	recorder->elvisHeap = (uint16_t *)sdram_malloc(NWA_HEAP_REQUEST_SIZE);
	if ( (NULL == recorder->elvisHeap) || (NULL == recorder->pAudio) ) 
	{
        return RT_ERROR;
    }
	return RT_EOK;
}



static void record_thread_entry(void *parameter)
{
    rt_device_t device = RT_NULL;
    int ret = RT_EOK;
    rt_uint8_t *buffer;
    rt_uint32_t read_bytes = 0;
	rt_uint32_t read_len = PCM_MP_BLOCK_SIZE;

	
	device = rt_device_find("mic");
	int i=0;
	
    if (RT_NULL == device)
    {
    	if(rt_sem_take(recorder->ack_sem,RT_WAITING_FOREVER) == RT_EOK)
		{
			rt_kprintf("error:stop record \r\n");
		}
        goto exit;
    }

	audio_device_mic_open();
	audio_device_mic_set_channel(1);
	audio_device_mic_set_rate(TEST_WAKEUP_16K_RATE);
	audio_device_mic_set_volume(TEST_WAKEUP_MIC_VOLUME);

	//discard first data
	rt_thread_mdelay(20);
    rt_kprintf("[record]:start record, tick %d \n", rt_tick_get());
    while (1)
    {
        buffer = rt_mp_alloc(&(recorder->mp), RT_WAITING_NO);
        if(RT_NULL == buffer)
        {
            rt_kprintf("[record]malloc memory for mempool failed \r\n");
            rt_thread_delay(40);
			continue;
        }
		read_bytes = 0;
		while(read_bytes < read_len)
        {
            /* read data from sound device */
            read_bytes += rt_sound_read(device, 0, buffer, read_len-read_bytes);
			if(read_bytes < (read_len>>1))
			{
				rt_thread_delay(10);
			}
        }
	
		record_msg_send(recorder, buffer, ACTION_WAKEUP_CONTINUE, read_bytes);
        /* send stop cmd */
        if (0 == recorder->action)
        {
            record_msg_send(recorder, 0, ACTION_WAKEUP_CANCLE, 1);
            /* wait ack */
			if(rt_sem_take(recorder->ack_sem, RT_WAITING_FOREVER)== RT_EOK)
			{
	            rt_kprintf("[record]:stop record, tick = %d \n", rt_tick_get());
	            break;
			}
        }
    }
    rt_device_close(device);
exit:
	free_recorder();
    rt_kprintf("[record]:exit record thread, tick = %d \n", rt_tick_get());
}

static void wakeup_thread_entry(void *parameter)
{
    int ret, cmd;
	int cnt=0;
    record_msg_t msg;
	int i= 0;

	int eStat = 0;
    int isSpeech = 0;
    int wakeUpSeen = 0;
    int numWakeups = 0;
    unsigned int frames = 0;
    unsigned short *phrase;
    int phraseLen, startFramesBack, lenFrames, startFrame;
    int wakeupScore;
    int delayFrames = 0;
#ifdef X871LIB_WAKEUP
	eStat = nwa_createObject((void *)Nuance_nhxz_blob_tab, sizeof(Nuance_nhxz_blob_tab),
									recorder->elvisHeap, NWA_HEAP_REQUEST_SIZE, NWA_FRAME_SAMPLES);
#else
    eStat = nwa_createObject_decode((void *)Nuance_nhxz_blob_tab, sizeof(Nuance_nhxz_blob_tab),
                                   recorder->elvisHeap, NWA_HEAP_REQUEST_SIZE, NWA_FRAME_SAMPLES);
#endif

    if (eStat) 
	{
        rt_kprintf("Error creating nwa object [%d]\r\n", eStat);
        goto exit;
    }

    // Start wakeup mode.
    eStat = nwa_startWakeup(0);
    if (eStat) 
	{
        rt_kprintf("Error starting [%d]\r\n", eStat);
        goto exit;
    }

    while(1)
    {
        if (rt_mq_recv(recorder->msg, &msg, sizeof(record_msg_t), 3000) == RT_EOK)
        {
            if(ACTION_WAKEUP_CONTINUE == msg.type)
            {
			/* read pcm data */
				rt_memcpy((void *)recorder->pcmbuff, (void *)msg.arg, PCM_BUFF_WAKEUP_ACC*PCM_BUFF_WAKEUP_SIZE);
				rt_mp_free((void *)msg.arg);


				i = 0;
				while(i < PCM_BUFF_WAKEUP_ACC)
				{
					rt_memcpy((void *)recorder->pAudio, (void *)recorder->pcmbuff+i*PCM_BUFF_WAKEUP_SIZE, PCM_BUFF_WAKEUP_SIZE);
					eStat = nwa_scanAudioFrameEx((short * )recorder->pAudio, NWA_FRAME_SAMPLES, NWA_20MHZ,&isSpeech, &wakeUpSeen);
					frames++;
					i++;
					
					if (eStat) 
					{
						rt_kprintf("[frame: %d]eState = %x\r\n", frames, eStat);
					}
				
					if (wakeUpSeen) 
					{
						numWakeups++;
						wakeupScore = -1;
						nwa_getWakeupInfo(&phrase, &phraseLen, &startFramesBack,&lenFrames);
						nwa_getWakeupScore(&wakeupScore);
				
						// Stop wakeup here.
						nwa_stopWakeup();
				
						startFrame = frames - startFramesBack;
						delayFrames = startFramesBack - lenFrames;
				
						// Restart wakeup mode for next wakeup word recognition.
						nwa_startWakeup(0);
				
						rt_kprintf("Found wakeup at time [%d] ms, score = %d, starts at %dms, length = %dms, delay = %dms\r\n",
							frames * NWA_FRAME_MS, wakeupScore,
							startFrame * NWA_FRAME_MS, lenFrames * NWA_FRAME_MS,
						   	delayFrames * NWA_FRAME_MS);

						recorder->action = 0;
						break;
					}
				}
				
            }
            else if(ACTION_WAKEUP_CANCLE== msg.type)
            {
				break;
            }
        }
		else
		{
			rt_kprintf("no message\r\n");
			recorder->action = 0;
			break;
		}
    }
exit:
    nwa_stopWakeup();
    // Destroy MxE.
    nwa_destroyObject();
	
	rt_sem_release(recorder->ack_sem);
}



static void wakeup_test(int argc, char **argv)
{
    rt_thread_t tid1 = RT_NULL;
	rt_thread_t tid2 = RT_NULL;
	

    if (strcmp(argv[1], "stop") == 0)
    {
        recorder->action = 0;
		return;
    }
    else if (strcmp(argv[1], "start") == 0)
    {
    	if(RT_NULL != recorder)
    	{
    		rt_kprintf("tread is ongoing\r\n");
			return;
    	}
		if(RT_ERROR == init_recorder())
			goto exit;
        recorder->action = 1;

        tid1 = rt_thread_create("record",
                               record_thread_entry,
                               RT_NULL,
                               1024 * 4,
                               16,
                               10);

        tid2 = rt_thread_create("wakeup",
                               wakeup_thread_entry,
                               RT_NULL,
                               1024 * 4,
                               15,
                               10);
        if ((RT_NULL == tid1)||(RT_NULL == tid2))
        {
        	rt_kprintf("error!!\r\n");
			if(tid1)
				rt_thread_delete(tid1);
			if(tid2)
				rt_thread_delete(tid2);
			goto exit;
        }
		else
		{
			msh_exec("free", strlen("free"));
            rt_thread_startup(tid1);
			rt_thread_startup(tid2);
			return;
		}
    }
    else
    {
        rt_kprintf("invalid arg!\r\n");
		return;
    }
exit:
	free_recorder(); 
}
MSH_CMD_EXPORT(wakeup_test,  record wakeup test);
#endif

