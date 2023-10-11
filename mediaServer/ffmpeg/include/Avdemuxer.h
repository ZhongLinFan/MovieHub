#pragma once
#include "ffmpeg.h"
#include <queue>
#include <string>
#include <mutex>
#include <thread>  //休眠
#include <chrono>	//休眠
#include <iostream>
#include <iomanip> //hex输出
#include <condition_variable>

//音视频的解封装器
//负责得到各个h264帧，并存储到队列中
class Avdemuxer {
public:
	Avdemuxer(std::string filePath);
	//音频帧添加adts头的函数
	int adts_header(uint8_t* const p_adts_header, const int data_length,
		const int profile, const int samplerate,
		const int channels);
	//解封装
	//怎么设计这个生产者消费者模型，两个队列的生产者，其实会互相影响，比如假如音频队列帧满了，但是视频队列帧空了，这个时候会阻塞在读到的音频帧上，
	//因为音频和视频帧是顺序读出的，你这个音频帧不消耗掉，下一个视频帧就没法av_read_frame，这该怎么办，消费音频帧的速度会快一点，生产音频帧的速度也会快一点
	//那就以音频帧为基准，通过打印av_read_frame的读取音视频顺序发现，不固定，但是到最后音频帧的个数远远多于视频帧，目前的矛盾点其实在于avreadFrame是不固定的
	//但是我又想根据消费的速度限制解封装速度，这该怎么办，那就以视频帧为基础，如果视频帧不够了，那就直到生产一个视频帧为止
	// （视频帧不够之前，会不会音频帧早就不够了呢），那就改成如果音频帧或者视频帧不够了，直到生产一个所需的帧为止（这个时候无视满不满的原则）
	//如果视频帧满了，音频帧肯定更多，因为rtp不可能只消耗音频帧，不消耗视频帧，而且消耗的音频帧还是更快的
	// 然后实现的时候发现没法实现指定的帧，因为条件变量不同，然后我看了一下，打印的音视频顺序，打算每次产生2个音频帧和一个视频帧
	// （有的时候可能一个音频帧和一个视频帧，回导致多生产一个音频帧）
	//生产帧的函数（Avdemuxer所在的线程）
	//注意上面的线程同步方式，见主要bug18，那里描述了不同的同步方式的效果
	void produceAvPacket();
	//消费帧的线程（rtp所在的线程）
	AVPacket* getFrame(std::queue<AVPacket*>& frameCatch);
	//队列是否满
	bool full() {
		return videoFrame.size() >= maxAvPacketCatch;  //注意这里为啥只判断视频帧大小
	}
	//队列是否很少，需要立刻生产出该类型的帧（这个时候需要无视满不满的条件）
	bool few() {
		return videoFrame.size() <= minAvPacketCatch || audioFrame.size() <= minAvPacketCatch;
	}
	void run() {
		//启动一个线程，解封装
		//注意这里不需要定时，可以看测试程序
		std::thread productAvPacket(&Avdemuxer::produceAvPacket, this);
		productAvPacket.detach();
	}
public:
	std::string m_filePath;	//必须要有这个，因为关闭的时候你需要自己关闭
	AVFormatContext* avFormatContext;
	AVBSFContext* avBSFContext;			//过滤器的上下文
	const AVBitStreamFilter* avBitStreamFilter;	//确定流过滤器，
	//注意在类中声明变量为const类型，但是不可以初始化 const常量的初始化必须在构造函数初始化列表中初始化，而不可以在构造函数函数体内初始化
	//参考https://www.cnblogs.com/kaituorensheng/p/3244910.html，但是好像还是不能调用函数
	
	//还需要存储音频流和视频流的索引
	int videoStreamId;
	int audioStreamId;
	//mp4文件解封装之后的h264缓存，注意，解封装之后的数据可能不可以直接用https://blog.csdn.net/leixiaohua1020/article/details/11800877
	//需要加上startCode，sps，pps等信息，所以解封装时需要额外的进行操作
	std::queue<AVPacket*> videoFrame;
	//然后发现需要将h264Frame分为音频帧和视频帧，24ms左右发送一帧底层取一帧数据到音频帧，40ms左右底层取一帧视频帧
	std::queue<AVPacket*> audioFrame;
	//demuxing定时解封装数据，如果队列为空，应该立即解封装数据，如果队列满了，应该停止解封装
	int maxAvPacketCatch; //最大的缓存数量
	int minAvPacketCatch; //到这一步就必须要生产了
	//两个队列的互斥锁，不能使用两个互斥锁，使用一个，因为我生产的时候是一起生产的，一起加锁就行了
	std::mutex mutex;
	std::condition_variable m_empty;  //空的条件变量，过少的话可以取的，所以不需要阻塞，也就是不需要设置条件变量，但是空的话，必须要等待
	std::condition_variable m_full;	//满条件变量

private:
	//需要增加一个变量判断当前Avdemuxer有没有正常初始化，如果没有正常初始化，后续解封装等是没法正常进行的，如果解封装完成，会继续设置为不可用状态
	bool isAvailable;
};