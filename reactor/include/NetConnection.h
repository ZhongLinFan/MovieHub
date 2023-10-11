#pragma once
#include "Buffer.h"
#include "Channel.h"
#include "Log.h"
#include "EventLoop.h"
#include <functional>
//后面在写客户端时发现，和TcpConnection和相似，就把他们的公共部分抽象出来了
//为了保留原来的TcpConnection的笔记，我复制过来之后，就不删除注释了，可能会导致不太对，因为当时写注释的时候完全是站在服务器的角度写的
//而且有几个函数是放在了不同的文件里（NetConnection和TCpConnection）
class TcpMsgRouter;
class Message;
class NetConnection {
public:
	using hookCallBack = std::function<void(NetConnection* conn, void* userData)>;
	//fd是为了给tcpConnect起名字，和初始化channel
	NetConnection();
	~NetConnection();
	//void* cfdConnection(void* args);
	//经过认真的思考，认为还是读写分离比较好，这里cfdConnection相当于业务层的读和业务逻辑处理和写，所以它不仅仅是读和写到buff里，而buff类是更底层的读和写，其他的不归buf管
	//cfdConnection拆分开，更能让业务单一，业务读只负责读数据，和解析，（可以通过任务队列的方式调用业务写，也可以直接调用业务写）业务写只负责组织回复的数据块和写
	//更能让业务单一是为啥考虑拆分开来的主要原因，下面两个函数和cfdConnection完成的是同一个事情
	int processRead();  //这个函数是注册给channel的，如果来了（肯定是客户端发过来的）读事件，那么就应该去调用这个，为啥这个不在channel里实现呢，
	//因为这个需要读和解析，解析出来的东西需要放到一块内存里，交给写业务处理，那么如果在channel里封装，就需要多出m_rbuffer和m_wbuffer和解析后的存储空间，
	//响应好（但未发送）的存储空间这样会层次不清晰，fd本身就应该只负责和他属性相关的东西，业务的东西他不应该管
	//那么参数应该传递什么呢，肯定是cfd对应的TcpConnection，因为我读和解析都需要TcpConnection
	int processWrite();

	//主动发包函数，里面必须要包含，往m_wbuffer写数据以及激活写事件的逻辑，否则主动发包机制需要开辟独立的道路
	//在processRead里也有简单介绍这个函数的作用
	//更清晰的理解可以认为一个包的处理流程包括收取，解析（需要解析出客户端想要的东西，然后找到想要的东西，放到m_response），
	//组织响应数据块到outbuf（sendMsg）， 发送outbuf到socket
	int sendMsg(bool copyResponse=true);  //这里提供一个是否拷贝到写缓冲区的选项，如果拷贝，就不需要在sendMsg拷贝，而是判断是否需要开启写事件
	//各自的释放函数（客户端和服务器的链接），必须要有，这个是m_evloop->addTask(this->m_evloop, m_channel, TaskType::DEL);进行触发的
	virtual bool destroy() = 0;

public:
	char name[32];
	EventLoop* m_evloop;
	Channel* m_channel;
	//不能放在这里，因为客户端不需要内存池
	//ReadBuffer* m_rbuffer;
	//WriteBuffer* m_wbuffer;
	Message* m_request;
	Message* m_response;
	ReadBuffer* m_rbuffer;  //这两个由于server使用内存池client不适用，所以需要在子类初始化
	WriteBuffer* m_wbuffer;
	struct sockaddr_in* m_sOrcAddr; //客户端或者服务器地址
	int m_addrLen;   //地址长度
	char* msg[23] = { "123456578909076588958695756097560978560978000679567094569456094569546054",
		"435645", 
		"43",
		"lalaalallalalalallalalalalalal",
		"你好世界，我爱你", 
		"那为啥不能用TcpConncetion类呢，确实很像，可以把他们的共同部分抽象出来，然后client和TcpConnection继承",
		"1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是",
		"12893849865469075675894568",
		"3",
		"3422",
		"说明建立连接成功，关闭写事件，添加读事件检测吗",
		//"我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X589539845ljgkdwaer.ity54u[sdvm,发了多少价格吗，从v上了飞机我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X589539845ljgkdwaer.ity54u[sdvm,发了多少价格吗，从v上了飞机我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X589539845ljgkdwaer.ity54u[sdvm,发了多少价格吗，从v上了飞机我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X589539845ljgkdwaer.ity54u[sdvm,发了多少价格吗，从v上了飞机我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X589539845ljgkdwaer.ity54u[sdvm,发了多少价格吗，从v上了飞机我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X589539845ljgkdwaer.ity54u[sdvm,发了多少价格吗，从v上了飞机我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X589539845ljgkdwaer.ity54u[sdvm,发了多少价格吗，从v上了飞机我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X589539845ljgkdwaer.ity54u[sdvm,发了多少价格吗，从v上了飞机我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5我爱你的他们的将出现吉林省地方109389435830423-*（Z&*……Z%￥再4(*&Z90*Z(0^&X*Z%^^^^^^$&X5",
		"hsdfvkszcklkeptietfjkfk;dflsdfsvxc vlkwerjwoep45459rlefgldflsd;sdjkxcnvjefgh",
		"jS#-jSL&Eh7W8bAS77IOy_96MjJxUd-W@&r",
		"iUezJsYIf^Ij0o$hb9h*iOt*GtNoclWFF1f5+eftL8Q",
		"aRPg%=ObrE7*9k4%R90FoGR!G1a3Bo&0r&&80B0#sv6Ld!Cte$=_gi8^xAbI+CNjn5!Qn54_%C4xYBvhLp%M",
		"iDeG75Ct=SuJVp2Y%B8L5DI+KBYfkMJKx+=qC=WOJ%tC-OZMgFdYV67M*4",
		"0pZV3wG@7^h",
		"8^xs=ky5bg=NNZTU@LGRxd",
		"lQYl1oYAEESqqWB7aniYUE@*6cnS%NeyE9l7f$_Rovt4XKX",
		"Ux3oyjWS9tvn6rCTcxEegs1fjRMPk4Owc7mfoG69YhhWXYcx9lPtGTee60ymUXrXO7cmMAzHB5jtJuDvVOaXEIU1BrC7pzk5WXw3thjed0jEgonFB9sKEyydo4XE9LkdLEPncML7wnvMvfgegcJEMU4J7K45VNDHJCKLHEkLHgS2XgOCmtrhrXNB6frKHVoiiCJHMAnASJrz4AnawBOITPUKeNyfJUGPB9B8oBBEfS0m79R9eO6vcWSkaGEMYXFKO2Iw1aGmkbDezdkSJoUJoTLMMVIW2XfBcnmYtBbxC8WTpRBUFJUa9kjKbjvUd4Pa6291FAPKy9eRKOIhuF9S91YTaCKCOtCrSisu3axNfMCG33gXj9RRA2oYd2iCX7bnAtIhHv5TbfYkS55LjSrl9DH34rF6IDmM7UnCjVwoUgYdDzsI4h6FHYGoWlVer8A1cG1313pCF6zXdwr6nsczjBhKNxY0WmfUl2UBJbgzzVgsruAap3WM9foClNl3OKoGxY33Y7gVYDc5xmWKJAUjJW0dyBcSdo1fx4gxE6D1EGgIASPeFGaONPS2u3PAUO1IpeADx3X4hUz1MnPT27kLgbmhmWeiOb3Oa09VwwOEHvoB",
		"zVKnY1mHXBTfH7ONje7EolnFkd7PSsbghFd2FbNETjHspbJFwzXdkPk8Eb0VDvoiEvIcCofA6ms8XRvYYvhAktTwluoNOsafFPJsK48Hb4hklNzHiRzcCwWTflcKDX2Y0w2V0WdLjSsj0mDcFRuHb7BRRTVy8bKMkoXveRDVxzhkItkhGr8yjSs1LChmfud7TWy423nRP2iCiGF804WB4xkyj06vvBsFzGSBp4XgXFPVUJjWEfXOmhAgkjBEPWtIMwF7DDUmiNOcv6U6AzzIgP1eEva6ifmzpFMUJWgyOnYG779PpclEpAYI7Ocke3mXx1yfuHulaN6UKPpMzUfdkSOWE5V6KSm5e5AckztphOfBDGVWCRPXkxN8TYRpMDnPFXsJESUR6i3h17oCFxdDAyFwsHya6jIwPTaYn6myRAWnLjWz2YM1mCET3byatIY0c5ATsv17fWU924yCTIcTgJpg6kitIuy2IlzomGRCBVtX0FNO4YWk9iD8N2zMi2TDj5IRE1DymrGi7g228o5l4f1xNhCev7silMGTCaRTGy9ya4E220iFH5hJWJxcokmdTiLTNBhbh5xP3tPd9JvnMii3SSSE86EVtR9kyK82yIHzUcEnso2xBpB4vMv768wsISAgsRwlcBXWhD673KxT8DYWxw1sskxEPu9V9esnDXBPENJFcCw8uyNmcCEM4noSjmwJJt95bA3iAGm7HbDswk5T0iBowmarRe16i2BUshIKUzNhh2S2oc01e2CSvM7wSpOO1DGKeGholuGJDDR56kkDplFBuEgG1pmwHRGotwP9WhfmGPragHE6hP",
		"我爱你行吗"
	};
	//用来标记当前的router是谁，因为如果不标记，在查找对应的回调函数的时候，不知道查找的对象是谁，如果服务器和客户端用一套体系，所以需要使用这个属性明确一下，
	TcpMsgRouter* m_msgRouter;

	//这个参数的作用主要是通过一个回调函数传递给另一个回调函数
	void* m_dymaicParam;
};