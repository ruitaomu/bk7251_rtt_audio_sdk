使用测试文件test.jpg(240*120)
打包工具的json文件参考如下：

{
    "magic": "RT-Thread",
    "version": "0.1",
    "count": 2,
    "section": [
        {
            "firmware": "bk_bootloader_up_all_test.bin",
            "version": "2M.1220",
            "partition": "bootloader",
            "start_addr": "0x00000000",
            "size": "65280"
        },
        {
            "firmware": "../../rtthread.bin",
            "version": "2M.1220",
            "partition": "app",
            "start_addr": "0x00011000",
            "size": "1500K"
        }
        ,
        {
            "firmware": "test.jpg",
            "version": "2M.1220",
            "partition": "fs",
            "flash_name":"beken_onchip",
            "start_addr": "0x0001a0000",
            "size": "12K"
        }
    ]
}
