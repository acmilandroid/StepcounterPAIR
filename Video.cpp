#include "StdAfx.h"
#include "Video.h"


Video::Video(void):
	videoStream(0),
	lastSeconds(0)
{
}


Video::~Video(void)
{
}

bool Video::OpenVideoFile(const char *filename, int &ImageRows, int &ImageCols)
{
	AVCodec				*pCodec;
	int					numBytes,i;

	/* Register all formats and codecs */
	av_register_all();
	/* allocate an AVFrame structure */
	pFrameRGB=av_frame_alloc();
	
	/* Open video file */
	pFormatCtx=NULL;
	if (avformat_open_input(&pFormatCtx,filename,NULL,NULL) != 0)
		return(false);

	/* retrieve stream information */
	if (avformat_find_stream_info(pFormatCtx,NULL) < 0)
	{
		avformat_close_input(&pFormatCtx);
		return(false);
	}

	/* find the first video stream */
	videoStream = -1;
	for (i=0; i<(int)(pFormatCtx->nb_streams); i++)
	{		
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream=i;
			break;
		}
	}
		
	if (videoStream == -1)
	{
		avformat_close_input(&pFormatCtx);
		return(false);
	}
		
	/* get a pointer to the codec context for the video stream */
	pCodecCtx=pFormatCtx->streams[videoStream]->codec;
	/* find the decoder for the video stream */
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		avformat_close_input(&pFormatCtx);
		return(false);
	}

	/* open codec */
	if (avcodec_open2(pCodecCtx,pCodec,NULL) < 0)
	{
		avformat_close_input(&pFormatCtx);
		return(false);
	}
	/* determine required buffer size and allocate buffer */
	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,pCodecCtx->height);
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture
	avpicture_fill((AVPicture *)pFrameRGB,buffer,PIX_FMT_RGB24,pCodecCtx->width, pCodecCtx->height);
	pSwsCtx=sws_getContext(pCodecCtx->width,
			pCodecCtx->height, pCodecCtx->pix_fmt,
			pCodecCtx->width, pCodecCtx->height,
			PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	if (pSwsCtx == NULL)
	{
		avformat_close_input(&pFormatCtx);
		avcodec_close(pCodecCtx);
		return(false);
	}

	/* tell calling function how big images will be */
	ImageRows=pCodecCtx->height;
	ImageCols=pCodecCtx->width;

	return(true);
}

int Video::ReadVideoFrame(double TimeSeconds, unsigned char *FrameImage)
{
	int					i;
	AVPacket			packet;
	int					seek_ret;
	int					frameFinished;
	//double				time;
	int64_t				seek_ts;
	AVFrame				*pFrame;

	/* allocate video frame */
	pFrame=av_frame_alloc();

	// convert the desired frame number to the time the frame exists in seconds
	//time = (double)FrameIndex;

	seek_ts = (TimeSeconds*(pFormatCtx->streams[videoStream]->time_base.den))/(pFormatCtx->streams[videoStream]->time_base.num);

	//int64_t				seekTarget = (Seconds*pCodecCtx->time_base.den)/pCodecCtx->time_base.num;
//	int64_t				videoFrame = (int64_t)(Seconds * AV_TIME_BASE);

	// seeking backwards
	/*if(lastSeconds > Seconds)
	{
		seek_ret = av_seek_frame(pFormatCtx,videoStream,seekTarget,AVSEEK_FLAG_ANY);
	}
	else
	{
		seek_ret = av_seek_frame(pFormatCtx,videoStream,seekTarget,AVSEEK_FLAG_ANY);
	}*/
	
	/* seek to the closest key frame prior to the desired frame */

	/* normally, a movie file defines its frame rate as .num/.dev; but the cafeteria movie files only show 1/1000 */
	/* there are appx 15 frames per second in a cafeteria video but they are not evenly spaced by the decoding timestamps (dts) */
	/* this search can be super-accurate if a video file is encoded better (see next line commented out and below) */
	//seek_ret=av_seek_frame(pFormatCtx,videoStream,(seek_seconds*pCodecCtx->time_base.den)/pCodecCtx->time_base.num,AVSEEK_FLAG_BACKWARD);
	
	//seek_ret=av_seek_frame(pFormatCtx,videoStream,FrameIndex*60000.0/1001.0*1000.0,AVSEEK_FLAG_BACKWARD);
	seek_ret=av_seek_frame(pFormatCtx,videoStream,seek_ts,AVSEEK_FLAG_BACKWARD);
	if (seek_ret < 0)
	{
		av_free_packet(&packet);	/* free the packet that was allocated by av_read_frame */
		av_frame_free(&pFrame);
		return(0);	/* no frame found at that seek */
	}
	avcodec_flush_buffers(pCodecCtx);  /* reset buffers to begin anew */

	/* read frames until the desired frame is obtained */
	while (av_read_frame(pFormatCtx,&packet) >= 0)
	{
		if (packet.stream_index == videoStream)
		{
			avcodec_decode_video2(pCodecCtx,pFrame,&frameFinished,&packet);
			if (frameFinished)
			{
				//if (packet.dts < (seek_ts + (pFormatCtx->streams[videoStream]->time_base.den/pFormatCtx->streams[videoStream]->time_base.num)))
				if (packet.dts < seek_ts)
				{
					av_free_packet(&packet);
					continue;
				}
				/* convert the image from its native format to RGB */
				sws_scale(pSwsCtx,(const uint8_t * const *)pFrame->data,
						pFrame->linesize,0,pCodecCtx->height,
						pFrameRGB->data,pFrameRGB->linesize);

				/* convert frame to simple array (reverse RGB to BGR because Win32 functions assume BGR order) */
				for (i=0; i<(pCodecCtx->height)*(pCodecCtx->width); i++)
				{
					FrameImage[i*3+0]=(unsigned char)(pFrameRGB->data[0][i*3+2]);
					FrameImage[i*3+1]=(unsigned char)(pFrameRGB->data[0][i*3+1]);
					FrameImage[i*3+2]=(unsigned char)(pFrameRGB->data[0][i*3+0]);
				}
				break;
			}
		}
	}
	av_free_packet(&packet);	/* free the packet that was allocated by av_read_frame */
	av_frame_free(&pFrame);
	RotateMatrix(FrameImage, pCodecCtx->height, pCodecCtx->width);
	return(1);
}

void Video::RotateMatrix(unsigned char *input, int rows, int cols){
	unsigned char *temp;
	temp=(unsigned char *)calloc(rows*cols*3,1);

	for (int i=0; i<rows; i++)
	{		
		for (int j=0;j<cols; j++)
		{
			temp[(rows*3-1-i*3)+j*3*rows-2] = input[j*3+i*cols*3];	
			temp[(rows*3-1-i*3)+j*3*rows-1] = input[j*3+i*cols*3+1];		
			temp[(rows*3-1-i*3)+j*3*rows] = input[j*3+i*cols*3+2];
		}
	}	

	for (int i=0; i<rows; i++)
	{		
		for (int j=0;j<cols; j++)
		{
			input[j*3+i*cols*3] = temp[j*3+i*cols*3];
			input[j*3+i*cols*3+1] = temp[j*3+i*cols*3+1];		
			input[j*3+i*cols*3+2] = temp[j*3+i*cols*3+2];
		}
	}

	free(temp);
	return;
}

void Video::CloseVideoFile()
{
	av_free(buffer);	
	av_frame_unref(pFrameRGB);
	av_frame_free(&pFrameRGB);	// Free the RGB image
	avcodec_close(pCodecCtx);			// Close the codec
	avformat_close_input(&pFormatCtx);	// Close the video file
}
