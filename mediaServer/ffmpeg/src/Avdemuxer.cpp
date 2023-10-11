#include "Avdemuxer.h"

Avdemuxer::Avdemuxer(std::string filePath) : avBitStreamFilter(av_bsf_get_by_name("h264_mp4toannexb")) {
	m_filePath = filePath;
	maxAvPacketCatch = 1000; //缓存24秒左右的解封装
	minAvPacketCatch = 240;  //当为10秒左右即可开始继续
	avFormatContext = avformat_alloc_context();  //这种初始化也行，	fmt_ctx = nullptr;这种初始化也行
	isAvailable = true; //先假定是可以使用的
	if (avFormatContext == NULL) {	//使用NULL
		std::cout << "failed to alloc format context\n" << std::endl;
		isAvailable = false;
	}
	//参考的连接：https://blog.csdn.net/weixin_43796767/article/details/116785419
	//打开媒体文件，并获得解封装上下文
	int ret = avformat_open_input(&avFormatContext, filePath.data(), NULL, NULL);
	if (ret < 0) {
		std::cout << "error:avformat_open_input" << std::endl;
		isAvailable = false;
	}
	// 读取媒体文件信息	//这一步会设置视频帧率等等 ,https://blog.csdn.net/Griza_J/article/details/126118779这里面的注释有提到
	if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
		std::cout << "failed to find stream" << std::endl;
		isAvailable = false;
	}
	av_dump_format(avFormatContext, 0, filePath.data(), 0);  //这个是输出当前文件的详细信息
	// 寻找音频流和视频流下标
	videoStreamId = -1, audioStreamId = -1;
	for (int i = 0; i < avFormatContext->nb_streams; i++) {
		if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStreamId = i;
		}
		else if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStreamId = i;
		}
	}
	if (videoStreamId < 0 || videoStreamId < 0) {
		std::cout << "failed to find stream index" << std::endl;
		isAvailable = false;
	}
	//初始化过滤器相关的东西，bsf必须在初始化列表初始化，但是还没判断有没有得到有效的过滤器，所以这里判断一下
	if (!avBitStreamFilter) {
		std::cout << "get avBitStreamFilter failed" << std::endl;
		isAvailable = false;
	}
	//bsf工作并没有完成，还需要3步
	ret = av_bsf_alloc(avBitStreamFilter, &avBSFContext);
	if (ret < 0) {
		std::cout << " av_bsf_alloc failed" << std::endl;
		isAvailable = false;
	}
	//拷贝输入流相关参数到过滤器的AVBSFContext
	ret = avcodec_parameters_copy(avBSFContext->par_in, avFormatContext->streams[videoStreamId]->codecpar);
	if (ret < 0) {
		std::cout << "copy codec params to filter failed" << std::endl;
		isAvailable = false;
	}
	//使过滤器进入准备状态。在所有参数被设置完毕后调用
	ret = av_bsf_init(avBSFContext);
	if (ret < 0) {
		std::cout << "Prepare the filter failed" << std::endl;
		isAvailable = false;
	}
}

//https://zhuanlan.zhihu.com/p/584316764
const int sampling_frequencies[] = {
		96000,  // 0x0
		88200,  // 0x1
		64000,  // 0x2
		48000,  // 0x3
		44100,  // 0x4
		32000,  // 0x5
		24000,  // 0x6
		22050,  // 0x7
		16000,  // 0x8
		12000,  // 0x9
		11025,  // 0xa
		8000   // 0xb
		// 0xc d e f是保留的
};

int Avdemuxer::adts_header(uint8_t* const p_adts_header, const int data_length,
	const int profile, const int samplerate,
	const int channels) {

	int sampling_frequency_index = 3; // 默认使用48000hz
	int adtsLen = data_length + 7;

	// 匹配采样率
	int frequencies_size = sizeof(sampling_frequencies) / sizeof(sampling_frequencies[0]);
	int i = 0;
	for (i = 0; i < frequencies_size; i++) {
		if (sampling_frequencies[i] == samplerate) {
			sampling_frequency_index = i;
			break;
		}
	}
	if (i >= frequencies_size) {
		std::cout << "没有找到支持的采样率" << std::endl;
		return -1;
	}

	p_adts_header[0] = 0xff;         //syncword:0xfff                          高8bits
	p_adts_header[1] = 0xf0;         //syncword:0xfff                          低4bits
	p_adts_header[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
	p_adts_header[1] |= (0 << 1);    //Layer:0                                 2bits
	p_adts_header[1] |= 1;           //protection absent:1                     1bit

	p_adts_header[2] = (profile) << 6;            //profile:profile               2bits
	p_adts_header[2] |=
		(sampling_frequency_index & 0x0f) << 2; //sampling frequency index:sampling_frequency_index  4bits
	p_adts_header[2] |= (0 << 1);             //private bit:0                   1bit
	p_adts_header[2] |= (channels & 0x04) >> 2; //channel configuration:channels  高1bit
	p_adts_header[3] = (channels & 0x03) << 6; //channel configuration:channels 低2bits
	p_adts_header[3] |= (0 << 5);               //original：0                1bit
	p_adts_header[3] |= (0 << 4);               //home：0                    1bit
	p_adts_header[3] |= (0 << 3);               //copyright id bit：0        1bit
	p_adts_header[3] |= (0 << 2);               //copyright id start：0      1bit
	p_adts_header[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits
	p_adts_header[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
	p_adts_header[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
	p_adts_header[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
	p_adts_header[6] = 0xfc;      //11111100       //buffer fullness:0x7ff 低6bits

	return 0;
}

//解封装
void Avdemuxer::produceAvPacket() {
	if (!isAvailable) {
		std::cout << "Avdemuxer is unavailable" << std::endl;
		return;
	}
	std::string videoOutTest = "./test.h264";
	std::string audioOutTest = "./test.aac";
	FILE* fvideo = fopen(videoOutTest.data(), "w+");
	FILE* faudio = fopen(audioOutTest.data(), "w+");
	while (1) {
		int i = 1;	//为了打印东西定义的
		{	//注意这个括号很重要
			std::unique_lock<std::mutex> locker(mutex);
			//条件变量，当视频帧满的时候（注意此时音频帧肯定满）阻塞在这里，不继续生产
			while (full()) {
				std::cout << "队列已满，请等待消费" << std::endl;
				m_full.wait(locker);
			}
			//最终的解析方案，按照测试文件的来
			//整个时间片都在解析，如果解析满队列，通知消费，如果解析还没满，那么等到醒来，继续解析
			//while (!full()) {
			AVPacket* packet = av_packet_alloc(); //必须放在里面，如果放在外面，最终只有一个包，那么rtp消耗掉释放时肯定会出错
			if (av_read_frame(avFormatContext, packet) < 0) {
				std::cout << "isAvailable设为false" << std::endl;
				isAvailable = false;
				return;
			}
			if (packet->stream_index == audioStreamId) {
				// adts 头是7个字节，也有可能是9个字节
				//注意，我打算将这7个字节先临时存储到
				//目前想到的有三种方式存储这个头文件，第一个是使用av_packet_from_data，这个需要多拷贝一份音频数据，
				// 第二个是audioFrame的类型定义为一个结构体，这样操作更简单，并且不需要多余的拷贝
				//第三个是av_packet_new_side_data来存储附加信息，这个其实也可以，但是这个有点误导人，并且可能会存在潜在性的危险，因为ffmpeg会调取一些av_packet_side_data
				// 不过目前来说是安全的，因为我的目的仅仅是临时存储这些数据
				// 我打算采取第三种，当作熟悉ffmpeg的接口了
				uint8_t* adts_header_buf = av_packet_new_side_data(packet, AV_PKT_DATA_NEW_EXTRADATA, 7);//AV_PKT_DATA_NEW_EXTRADATA这个本质上是存储视频的sps等信息的，但是这里就先借用了
				adts_header(adts_header_buf, packet->size,
					avFormatContext->streams[audioStreamId]->codecpar->profile,
					avFormatContext->streams[audioStreamId]->codecpar->sample_rate,
					avFormatContext->streams[audioStreamId]->codecpar->channels);
				//在写入到rtp的时候，先写adts头，有些是解封装出来就带有adts头的比如ts
				//将adts头和av_read_frame得到的帧组合成一个新的avpacket，上面就相当于组合了
#if 0
		//写入文件简单测试，看是否能正常播放
		//fwrite(adts_header_buf, 1, 7, faudio);
				fwrite(packet->data, 1, packet->size, faudio);
#endif

#if 0
				std::cout << "当前帧：" << i << "\t当前帧长：packet.size" << packet->size << std::endl;
				for (int k = 0; k < packet->size; k++) {
					printf("%x ", packet->data[k]);
				}
				std::cout << std::dec << std::endl;
				//打印添加adts头
				std::cout << "当前帧更新后的帧：" << i << "\t当前帧长：packet.size + 7：" << packet.size + 7 << std::endl;
				for (int k = 0; k < 7; k++) {
					printf("%x ", adts_header_buf[k]);
				}
				for (int k = 0; k < packet->size; k++) {
					printf("%x ", packet->data[k]);
				}
				std::cout << std::dec << std::endl;
#endif
				audioFrame.push(packet);
				//std::cout << "当前音频帧缓存个数：" << audioFrame.size() << std::endl;
				}
			else if (packet->stream_index == videoStreamId) {
				//注意这里是关键步骤，关键帧前加入sps等信息
#if 0
		//不加入sps简单测试，结果：打开不了
				fwrite(packet->data, 1, packet->size, fvideo);
#endif

#if 0
				std::cout << "当前帧：" << i << "\t当前帧长：packet.size" << packet->size << std::endl;
				for (int k = 0; k < packet->size; k++) {
					printf("%x ", packet->data[k]);
					//std::cout << std::hex << packet->data[k] << " ";这个输出不了，必须使用printf（https://blog.csdn.net/feng125452/article/details/32910689）
				}
				std::cout << std::dec << std::endl;
				int j = 1;
#endif
				if (av_bsf_send_packet(avBSFContext, packet) == 0) {
					while (av_bsf_receive_packet(avBSFContext, packet) == 0) {
#if 0
						std::cout << "\t当前帧：" << i++ << "产生的帧：" << j++ << "的帧长:packet.size" << packet->size << std::endl;
						for (int m = 0; m < packet->size; m++) {
							printf("%x ", packet->data[m]);
							//std::cout << std::hex << packet->data[m] << " ";
						}
						std::cout << std::dec << std::endl;
#endif
#if 0 
						//加入sps等简单测试，结果：可以正常播放！！！！
						fwrite(packet->data, 1, packet->size, fvideo);
#endif
						videoFrame.push(packet);
						//std::cout << "当前视频帧缓存个数：" << videoFrame.size() << std::endl;
						}
				}
			}
			//av_packet_unref(packet);
			//生产出来一个指定的帧之后，需要通知消费者消费吗？，需要唤醒，这个notify只是唤醒阻塞在这个条件变量上的线程，如果没有阻塞在这上面那就没影响
			//唤醒阻塞在m_empty条件变量上的线程（其实就是while(videoFrame.empty())），如果没有阻塞的线程就啥都没有，因为消费者会主动获取getVideoFrame
			//根据测试程序的结果，每生产一个唤醒一下，让消费者赶紧去消费比较好，而不至于生产满再去消费
			}
		m_empty.notify_one();
		std::this_thread::sleep_for(std::chrono::microseconds(1));	//注意这个是为了让其他线程抢到的，很重要
		//}
	}
}

//可能是视频也可能是音频，（发现取视频和取音频流程是一样的，所以和一起了）
AVPacket* Avdemuxer::getFrame(std::queue<AVPacket*>& frameCatch) {
	//rtp线程调用
	//如果队列空了，这种情况是不存在的，因为如果小于min就要开始生产了，但是也可能存在所以这里wait一下
	std::unique_lock<std::mutex> locker(mutex);
	if (isAvailable == false && frameCatch.empty()) return nullptr; //如果不可用，说明已经解码完了，并且当前缓存也消耗完了，直接返回nullptr
	//如果到这，说明没有消耗完，但是队列为空，这个时候需要等待，一般不会进入这里面（不对，我在测试文件里测试了类似的功能，发现很多空的情况。。。。。）
	while(frameCatch.empty()) {  //这里需要用到while
		std::cout << "视频缓存队列为空，稍等一会" << std::endl;
		//阻塞当前线程
		m_empty.wait(locker);
	}
	//到这说明可以正常取，取出一个返回
	AVPacket* packet = frameCatch.front();
	frameCatch.pop();
	//如果当前视频帧或者音频帧过少，则发出信号生产，
	if (few()) {
		m_full.notify_one();	//阻塞在full上的线程该醒来生产了，如果没有阻塞在这个条件变量上，说明切换到produceAvPacket之后不会阻塞，而是直接生产
	}
	return packet;
}