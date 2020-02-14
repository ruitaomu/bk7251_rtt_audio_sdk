//
//该头文件使用于RTos系统
//集成了玩瞳方端上授权登录和翻页检测算法，不依赖于任何三方库，
//与服务器相关的请求和语音播放逻辑由sdk使用方实现。
//

#ifndef _VTBK_H__
#define _VTBK_H__

#include "vttypes.h"

int vt_init(StatedetParam *stateParam);                  //初始化
int vt_destroy();                                        //释放内存
int vt_setLoginRequestFunc(loginRequest fun);            //授权登录的请求函数
int vt_setLicenseText(const char *text);                 //license文件内容
int vt_setDeviceID(const char *deviceId);                //设备ID,唯一序列，如MAC或SN号
int vt_setModelType(const char *model);                  //设备模型名称，玩瞳提供
int vt_login();                                          //使用上面解析后的信息授权登录
DetState vt_stateDetect(ImageWrapper img);                //对一帧图片进行状态检测

#endif
