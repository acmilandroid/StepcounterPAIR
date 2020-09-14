#pragma once
#define inline __inline		  /* needed for FFmpeg library */

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
}
class Video
{
public:
	Video(void);
	~Video(void);
	bool OpenVideoFile(const char*, int&, int&);
	void CloseVideoFile(void);
	 /* time in video to retrieve from (e.g. 1033 = 1.033 seconds = 32nd frame of 30Hz video */
	int ReadVideoFrame(double, unsigned char*);

private:
	void RotateMatrix(unsigned char *input, int rows, int cols);
	AVFormatContext		*pFormatCtx;
	AVCodecContext		*pCodecCtx;
	uint8_t				*buffer;
	AVFrame				*pFrameRGB;
	int					videoStream;
	int					lastSeconds;
	struct SwsContext	*pSwsCtx;
};

