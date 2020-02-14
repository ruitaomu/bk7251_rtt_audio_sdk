#ifndef _WANTONG_HTTPCLIENT_H_
#define _WANTONG_HTTPCLIENT_H_
#define MAX_VOICE_URL		8

//picture recogoniton result
typedef struct _json_result {
	int  code;
    int  bookId; 
    int  physicalIndex;        
    char *voice_url[MAX_VOICE_URL];    
    char *effectSound_url;
    char *bgMusic_url;
} RecognitionResult;


//login result
typedef struct _login_result {
    int code;              //the login result status code returned by server
    char *verification;    //the auth-string returned by server
    char *token;
    char *Cookie;
} LoginResult;


void customLoginRequest(char *requestStr, LoginResult *result);
void wantong_http_login(const char *uri,LoginResult *result);
int wantong_post_resquest(RecognitionResult *result,char *imag_buf,int image_size,void (*fn)());

#endif
