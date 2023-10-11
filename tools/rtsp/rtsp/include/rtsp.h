#pragma once
#include <string>

//突然意识到客户端也需要rtsp协议，那么就不应该把这个rtsp放到mediaServer这里了，因为客户端到时候也需要索引这个文件夹，应该放到tools那里进行操作

//负责发送控制协议rtsp
class RTSP {
#if 0
    //处理OPTINOS请求
    void options(char* sbuffer, int CSeq) {
        sprintf(sbuffer, "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
            "\r\n",
            CSeq);
    }

    //处理DESCRIBE请求
    void describe(char* sbuffer, int CSeq, const char* url) {
        char sdp[500];
        char localIp[100];

        sscanf(url, "rtsp://%[^:]:", localIp);

        sprintf(sdp, "v=0\r\n"
            "o=- 9%ld 1 IN IP4 %s\r\n"
            "t=0 0\r\n"
            "a=control:*\r\n"
            "m=video 0 RTP/AVP 96\r\n"
            "a=rtpmap:96 H264/90000\r\n"
            "a=control:track0\r\n",
            time(NULL), localIp);

        sprintf(sbuffer, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
            "Content-Base: %s\r\n"
            "Content-type: application/sdp\r\n"
            "Content-length: %zu\r\n\r\n"
            "%s",
            CSeq,
            url,
            strlen(sdp),
            sdp);
    }

    //处理SETUP请求
    void setup(char*& sbuffer, int CSeq, int RtpPort) {
        sprintf(sbuffer, "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
            "Session: 66334873\r\n"
            "\r\n",
            CSeq,
            RtpPort,
            RtpPort + 1,
            SERVER_RTP_PORT,
            SERVER_RTCP_PORT);
    }


    //处理paly请求
    void play(char*& sbuffer, int CSeq) {
        sprintf(sbuffer, "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Range: npt=0.000-\r\n"
            "Session: 66334873; timeout=10\r\n\r\n",
            CSeq);
    }
#endif
public:
    //当前服务器支持的功能 options
    std::string optionsList;  //不能和上面的函数重名
};