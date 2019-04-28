#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "board.h"
#include "audio_device.h"


#define MICPHONE_TEST

#ifdef MICPHONE_TEST

#define TEST_BUFF_LEN 60*1024
#define READ_SIZE 1024

static uint8_t *test_buf;

void record_and_play(int argc,char *argv[])
{
	int mic_read_len = 0;
	int actual_len,i;
	int dac_wr_len=0;
	uint16_t *buffer = NULL;

	int vad_on;
	
#if CONFIG_SOUND_MIXER
    mixer_pause();
#endif

    vad_on = atoi(argv[1]);
		
	test_buf = sdram_malloc(TEST_BUFF_LEN);
	if(test_buf == NULL)
	{
		rt_kprintf("===not enough memory===\r\n");
		return;
	}
	
	audio_device_init();

	audio_device_mic_open();
	audio_device_mic_set_channel(1);
	audio_device_mic_set_rate(16000);

	if (vad_on)
	{
	    rt_kprintf("Vad is ON !!!!!!!!\r\n");
		wb_vad_enter();
	}
	
	while(1)
	{
	    if (vad_on)
		    rt_thread_delay(5);
		else
			rt_thread_delay(20);

		int chunk_size = wb_vad_get_frame_len();//320
		char *val = NULL;
		
		if(mic_read_len > TEST_BUFF_LEN - READ_SIZE)
			break;

		if (!vad_on)
		{
		    actual_len = audio_device_mic_read(test_buf+mic_read_len,READ_SIZE);
		}
		else
		{
            actual_len = audio_device_mic_read(test_buf+mic_read_len,chunk_size);
		    if(wb_vad_entry(test_buf+mic_read_len, actual_len))
		    {
		        rt_kprintf("Vad Detected !!!!!!!!\r\n");
			    break;
		    }
		}

		mic_read_len += actual_len;
	}

	if (vad_on)
	{
		wb_vad_deinit();
	}

	rt_kprintf("mic_read_len is %d\r\n", mic_read_len);
	audio_device_mic_close();

	audio_device_open();
	audio_device_set_rate(8000);


	while(1)
	{
		buffer = (uint16_t *)audio_device_get_buffer(RT_NULL);
		if(dac_wr_len >= mic_read_len)
		{
			audio_device_put_buffer(buffer);
			break;
		}

		memcpy(buffer,test_buf+dac_wr_len,READ_SIZE);
		dac_wr_len += READ_SIZE;

        audio_device_write((uint8_t *)buffer, READ_SIZE);
	}
	audio_device_close();

	if(test_buf)
		sdram_free(test_buf);

#if CONFIG_SOUND_MIXER
    mixer_replay();
#endif
}
MSH_CMD_EXPORT(record_and_play, record play);
#endif
