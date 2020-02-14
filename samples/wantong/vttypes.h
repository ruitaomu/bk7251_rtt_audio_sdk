#ifndef __VTTYPES_H__
#define __VTTYPES_H__
#include "wantong_httpclient.h"


typedef struct _detState {
    char *error;    //默认信息null
    char ret;       //检测结果是否成功
    int state;      //检测状态，1：可以请求服务器；0：忽略当前状态；-1：检测出错；-2：license过期
} DetState;

typedef struct ImageWrapper {
    unsigned char *data;    // 图像的数据
    int width;              // 宽度
    int height;             // 高度
    int channels;           // 通道数
    unsigned int size;      // 图像数据的字节数
} ImageWrapper;

typedef struct StatedetParam {
    int x, y, w, h;
    int ds_factor;          //一般赋值为4
    int in_type;            //1: 灰度图; 2: NV21的YUV图片
} StatedetParam;

//授权登录的请求回调函数, 第一个参数是url，第二个参数用于存放返回的结果（由我们控制两个指针空间的申请和释放）
typedef void (*loginRequest)(char *, LoginResult *);

#endif
