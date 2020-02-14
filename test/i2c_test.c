#define I2C_EEPROM_DEBUG
//#undef I2C_EEPROM_DEBUG


#ifdef I2C_EEPROM_DEBUG
#define I2C_EEPROM_PRT      os_printf
#define I2C_EEPROM_WARN     warning_prf
#define I2C_EEPROM_FATAL    fatal_prf
#else
#define I2C_EEPROM_PRT      null_prf
#define I2C_EEPROM_WARN     null_prf
#define I2C_EEPROM_FATAL    os_printf
#endif


#include "include.h"
#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <string.h>
#include "icu_pub.h"
#include "i2c_pub.h"
#include "drv_model_pub.h"
#include "target_util_pub.h"
#include "test_config.h"

#ifdef I2C_TEST

/*************************************************************
 * I2C1_write_eeprom
 * Description: I2C1 write FT24C02 eeprom
 * Parameters:  op_addr: operate address
 *              pData: data point
 *              len: data len
 * return:      unsigned long
 * error:       none
 */
unsigned long I2C1_write_eeprom(unsigned char op_addr, unsigned char *pData, unsigned char len)
{
    unsigned char i;
    DD_HANDLE i2c_hdl;
    unsigned int status;
    unsigned int oflag;
    I2C_OP_ST i2c1_op;

    I2C_EEPROM_PRT("----- I2C1_write_eeprom start -----\r\n");

    oflag = 0;
    i2c_hdl = ddev_open(I2C1_DEV_NAME, &status, oflag);

    i2c1_op.op_addr  = op_addr;
    i2c1_op.salve_id = 0x50;

    do
    {
        status = ddev_write(i2c_hdl, pData, len, (unsigned long)&i2c1_op);
    } while (status != 0);

    ddev_close(i2c_hdl);

    I2C_EEPROM_PRT("----- I2C1_write_eeprom over  -----\r\n");
    return 0;
}

/*************************************************************
 * I2C1_read_eeprom
 * Description: I2C1 read FT24C02 eeprom
 * Parameters:  op_addr: operate address
 *              pData: data point
 *              len: data len
 * return:      unsigned long
 * error:       none
 */
unsigned long I2C1_read_eeprom(unsigned char op_addr, unsigned char *pData, unsigned char len)
{
    unsigned char i;
    DD_HANDLE i2c_hdl;
    unsigned int status;
    unsigned int oflag;
    I2C_OP_ST i2c1_op;

    I2C_EEPROM_PRT("----- I2C1_read_eeprom start -----\r\n");

    oflag = 0;
    i2c_hdl = ddev_open(I2C1_DEV_NAME, &status, oflag);

    i2c1_op.op_addr  = op_addr;
    i2c1_op.salve_id = 0x73;

    do
    {
        status = ddev_read(i2c_hdl, pData, len, (unsigned long)&i2c1_op);
    } while (status != 0);

    for (i=0; i<8; i++)
    {
        I2C_EEPROM_PRT("pData[%d] = 0x%x\r\n", i, pData[i]);
    }

    ddev_close(i2c_hdl);

    I2C_EEPROM_PRT("----- I2C1_read_eeprom over  -----\r\n");
    return status;
}

/*************************************************************
 * I2C1_test_eeprom
 * Description: I2C1 test FT24C02 eeprom
 * Parameters:  none
 * return:      unsigned long
 * error:       none
 */
unsigned long i2c1_test_eeprom(void)
{
    int i, j;
    DD_HANDLE i2c_hdl;
    unsigned char pReadData[8];
    unsigned char pWriteData[8];

    I2C_EEPROM_PRT("----- I2C1_test_eeprom start -----\r\n");

//    memset(pReadData, 0, 8);
//    I2C1_read_eeprom(0x10, pReadData, 8);

    for (j=0; j<100; j++)
    {
        delay_ms(100);

        for (i=0; i<8; i++)
        {
            pWriteData[i] = (i << 2) + 0x01 + j;
        }
        I2C1_write_eeprom(0x00+j*8, pWriteData, 8);

        delay_ms(100);

        memset(pReadData, 0, 8);
        I2C1_read_eeprom(0x00+j*8, pReadData, 8);

        if (memcmp(pReadData, pWriteData, 8) == 0)
        {
            os_printf("I2C1_test_eeprom: memcmp %d ok!\r\n", j);
        }
        else
        {
            I2C_EEPROM_FATAL("I2C1_test_eeprom: memcmp %d error!\r\n", j);
            for (i=0; i<8; i++)
            {
                I2C_EEPROM_FATAL("pReadData[%d]=0x%x, pWriteData[%d]=0x%x\r\n",
                                i, pReadData[i], i, pWriteData[i]);
            }
        }
    }

    I2C_EEPROM_PRT("----- i2c1_test_eeprom over  -----\r\n");
    return 0;
}



#define I2C_TEST_LEGNTH 		32
#define I2C_TEST_EEPROM_LEGNTH 	8

uint32 i2c2_test(int argc, char** argv)
{
	int i, j, ret;
    DD_HANDLE i2c_hdl;
    unsigned int status;
    unsigned int oflag;
    I2C_OP_ST i2c2_op;
	i2c_op_message i2c_msg_config;
	
    bk_printf(" i2c2_test start  \r\n");

	i2c_msg_config.pData = (UINT8*)malloc(I2C_TEST_LEGNTH);
	if(i2c_msg_config.pData == NULL)
	{
		bk_printf("malloc fail\r\n");
		goto exit;
	}	

	i2c_msg_config.InnerAddr  = 0x00; 
	i2c_msg_config.AddrFlag   = 0;
	i2c_msg_config.TransDone  = 0;
	i2c_msg_config.ack_check  = 1;
	i2c_msg_config.CurrentNum = 0;
	i2c_msg_config.AllDataNum = I2C_TEST_LEGNTH;

	oflag = 0;
	i2c_hdl = ddev_open(I2C2_DEV_NAME, &status, oflag);
	
	if(strcmp(argv[1], "master_wr") == 0)
	{
		bk_printf("i2c2 master write\r\n");

		for (i=0; i<I2C_TEST_LEGNTH; i++)
		{
			i2c_msg_config.pData[i] = (i << 2) + 0x01 ;
		}		
			
		i2c_msg_config.WkMode =   0 & (~I2C2_MSG_WORK_MODE_RW_BIT)   // write
                     		    	& (~I2C2_MSG_WORK_MODE_MS_BIT)   // master
                     		    	& (~I2C2_MSG_WORK_MODE_AL_BIT)   // 7bit address
                                	& (~I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address
	
		i2c_msg_config.SalveID	  = 0x73;
		i2c_msg_config.SendAddr   = 0x72;	//send slave address
		
		ddev_control(i2c_hdl, I2C2_CMD_GET_MESSAGE, (i2c_op_message*)&i2c_msg_config);  //set message
		
		do
	    {
	        status = ddev_write(i2c_hdl, i2c_msg_config.pData, I2C_TEST_LEGNTH, 0);
	    } while (status != 0);
	}
	if(strcmp(argv[1], "master_rd") == 0)
	{	
		bk_printf("i2c2 master read\r\n");

		i2c_msg_config.WkMode =  (0 | (I2C2_MSG_WORK_MODE_RW_BIT))   // read
                     		    	& (~I2C2_MSG_WORK_MODE_MS_BIT)   // master
                     		    	& (~I2C2_MSG_WORK_MODE_AL_BIT)   // 7bit address
                                	& (~I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address
	
		i2c_msg_config.SalveID	  = 0x73;
		i2c_msg_config.SendAddr   = 0x72;	// send slave address
		
		ddev_control(i2c_hdl, I2C2_CMD_GET_MESSAGE, (i2c_op_message*)&i2c_msg_config);  //set message
		
		do
		{
			status = ddev_read(i2c_hdl, i2c_msg_config.pData, I2C_TEST_LEGNTH, 0);
		} while (status != 0);
	
	}
	if(strcmp(argv[1], "slave_wr") == 0)
	{
		
		bk_printf("i2c2 slave write\r\n");

		for (i=0; i<I2C_TEST_LEGNTH; i++)
		{
			i2c_msg_config.pData[i] = (i << 2) + 0x01 ;
		}
		
		i2c_msg_config.WkMode =  (0 | (I2C2_MSG_WORK_MODE_MS_BIT))   // slave
                     		    	& (~I2C2_MSG_WORK_MODE_RW_BIT)   // write
                     		    	& (~I2C2_MSG_WORK_MODE_AL_BIT)   // 7bit address
                                	& (~I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address

		
		i2c_msg_config.SalveID	  = 0x72;
		i2c_msg_config.SendAddr   = 0x73;	// send slave address
		
		ddev_control(i2c_hdl, I2C2_CMD_GET_MESSAGE, (i2c_op_message*)&i2c_msg_config);  //set message
		
		do
		{
			status = ddev_write(i2c_hdl, i2c_msg_config.pData, I2C_TEST_LEGNTH, 0);
		} while (status != 0);
	
	}
	if(strcmp(argv[1], "slave_rd") == 0)
	{
		
		bk_printf("i2c2 slave read\r\n");

		i2c_msg_config.WkMode =  (0 | (I2C2_MSG_WORK_MODE_MS_BIT)    // slave
                     		    	| (I2C2_MSG_WORK_MODE_RW_BIT))   // read
                     		    	& (~I2C2_MSG_WORK_MODE_AL_BIT)   // 7bit address
                                	& (~I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address
		
		i2c_msg_config.SalveID	  = 0x72;	//slave address
		i2c_msg_config.SendAddr   = 0x73;	//only master send address
		
		ddev_control(i2c_hdl, I2C2_CMD_GET_MESSAGE, (i2c_op_message*)&i2c_msg_config);  //set message
		
		do
		{
			status = ddev_read(i2c_hdl, i2c_msg_config.pData, I2C_TEST_LEGNTH, 0);
		} while (status != 0);		
	}

	for(i=0; i<I2C_TEST_LEGNTH; i++)
	{
		bk_printf("pData[%d]=0x%x\r\n",i,i2c_msg_config.pData[i]);
	}
	
    ddev_close(i2c_hdl);

    bk_printf(" i2c2 test over\r\n");
	
exit:

    if (RT_NULL != i2c_msg_config.pData)
    {
        free(i2c_msg_config.pData);
        i2c_msg_config.pData= RT_NULL;
    }	
}

uint32 i2c2_test_eeprom(int argc, char** argv)
{
	int i, j, ret;
    DD_HANDLE i2c_hdl;
    unsigned int status;
    unsigned int oflag;
    I2C_OP_ST i2c2_op;
	i2c_op_message i2c_msg_config;
	
    bk_printf(" i2c2_test_eeprom start  \r\n");
	
	i2c_msg_config.pData = (UINT8*)malloc(I2C_TEST_EEPROM_LEGNTH);
	if(i2c_msg_config.pData == NULL)
	{
		bk_printf("malloc fail\r\n");
		goto exit;
	}

	i2c_msg_config.InnerAddr  = 0x20; 
	i2c_msg_config.AddrFlag   = 0;
	i2c_msg_config.TransDone  = 0;
	i2c_msg_config.ack_check  = 1;
	i2c_msg_config.CurrentNum = 0;
	i2c_msg_config.AllDataNum = I2C_TEST_EEPROM_LEGNTH;

	oflag = 0;
	i2c_hdl = ddev_open(I2C2_DEV_NAME, &status, oflag);
	
	if(strcmp(argv[1], "write_eeprom") == 0)
	{
		bk_printf("eeprom write\r\n");

		for (i=0; i<I2C_TEST_EEPROM_LEGNTH; i++)
		{
			i2c_msg_config.pData[i] = (i << 2) + 0x05 ;
		}		
			
		i2c_msg_config.WkMode =  (0 & (~I2C2_MSG_WORK_MODE_RW_BIT)   // write
                     		    	& (~I2C2_MSG_WORK_MODE_MS_BIT)   // master
                     		    	& (~I2C2_MSG_WORK_MODE_AL_BIT))   // 7bit address
                                	| (I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address
	
		i2c_msg_config.SalveID	  = 0x73;
		i2c_msg_config.SendAddr   = 0x50;	//send slave address
		
		ddev_control(i2c_hdl, I2C2_CMD_GET_MESSAGE, (i2c_op_message*)&i2c_msg_config);  //set message
		
		do
	    {
	        status = ddev_write(i2c_hdl, i2c_msg_config.pData, I2C_TEST_EEPROM_LEGNTH, 0);
	    } while (status != 0);
	}
	if(strcmp(argv[1], "read_eeprom") == 0)
	{
		bk_printf("eeprom read\r\n");

		i2c_msg_config.WkMode =  (0 & (~I2C2_MSG_WORK_MODE_RW_BIT)   // write
                     		    	& (~I2C2_MSG_WORK_MODE_MS_BIT))   // master
                     		    	| ( I2C2_MSG_WORK_MODE_AL_BIT)   // 7bit address
                                	| ( I2C2_MSG_WORK_MODE_IA_BIT);  // without inner address
	
		i2c_msg_config.SalveID	  = 0x73;
		i2c_msg_config.SendAddr   = 0x50;	//send slave address
		
		ddev_control(i2c_hdl, I2C2_CMD_GET_MESSAGE, (i2c_op_message*)&i2c_msg_config);  //set message
		
		do
	    {
	        status = ddev_read(i2c_hdl, i2c_msg_config.pData, I2C_TEST_EEPROM_LEGNTH, 0);
	    } while (status != 0);
	}

	for(i=0; i<I2C_TEST_EEPROM_LEGNTH; i++)
	{
		bk_printf("pData[%d]=0x%x\r\n",i,i2c_msg_config.pData[i]);
	}
	
    ddev_close(i2c_hdl);

    bk_printf(" i2c2 test over\r\n");
	
exit:

    if (RT_NULL != i2c_msg_config.pData)
    {
        free(i2c_msg_config.pData);
        i2c_msg_config.pData= RT_NULL;
    }	
}

	
MSH_CMD_EXPORT(i2c1_test_eeprom, i2c1_test_eeprom);
MSH_CMD_EXPORT(i2c2_test_eeprom, i2c2_test_eeprom);
MSH_CMD_EXPORT(i2c2_test, i2c2_test);

#endif

