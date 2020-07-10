//	fake some linux stuff so we can compile on other platforms
#if !defined(TARGET_LINUX)
#include <stdint.h>
typedef int8_t __s8;
typedef uint8_t __u8;
typedef int16_t __s16;
typedef uint16_t __u16;
typedef int32_t __s32;
typedef uint32_t __u32;
typedef int64_t __s64;
typedef uint64_t __u64;
#endif

//#include "nvidia/samples/01_video_encode/video_encode.h"
#include "SoyPixels.h"
#include "NvidiaEncode.h"

namespace Nvidia
{
	void	IsOkay(int Result, const char* Context);
	void	Log(void *data, int i_level, const char *psz, va_list args);

	int		GetColourSpace(SoyPixelsFormat::Type Format);

	class TPacket;
}

void Nvidia::IsOkay(int Result, const char* Context)
{
	if(Result == 0)
		return;

	std::stringstream Error;
	Error << "nvidia error " << Result << " (" << Context << ")";
	throw Soy::AssertException(Error);
}

Nvidia::TEncoder::TEncoder(TEncoderParams& Params,std::function<void(PopH264::TPacket&)> OnOutPacket)
{
	//	the nvvideoencoder is a wrapper for a V4L2 device
	//	which is a standard linux file i/o stream
	//	alloc an encoder in blocking mode
	//	so the non-blocking mode is an IO mode
	//	gr: what is enc0 ? device name?
	//ctx.enc = NvVideoEncoder::createVideoEncoder("enc0", O_NONBLOCK);
	mEncoder = NvVideoEncoder::createVideoEncoder("enc0")
	if ( !mEncoder )
		throw Soy::AssertException("Failed to allocate nvidia encoder");
}

namespace V4lPixelFormat
{
	enum Type : uint32_t
	{
		YUV420M = V4L2_PIX_FMT_YUV420M,
	}
}
V4lPixelFormat::Type GetPixelFormat(SoyPixelsFormat::Type Format)
{
	switch(Format)
	{
		case SoyPixelsFormat::Yuv_844:
			return V4lPixelFormat::YUV420M;
			
		default:break;
	}
	
	std::stringstream Error;
	Error << "No conversion from " << Format << " to v4l";
	throw Soy::AssertException(Error);
}


void Nvidia::TEncoder::InitInputFormat()
{
	//	gr: nvidia demo calls this the output plane
	
	auto& Encoder = *mEncoder;
	//	the input is a "capture" plane
	SoyPixelsMeta PixelMeta(100,100,SoyPixelsFormat::Yuv_844);
	auto PixelFormat = GetPixelFormat( PixelMeta.GetFormat() );
	auto Format = V4L2_PIX_FMT_YUV444M;
	auto MaxSize = InputFormat.GetDataSize();
	auto Result = Encoder.setOutputPlaneFormat(Format, Width, Height, MaxSize );
	IsOkay(Result,"InitInpuFormat failed setOutputPlaneFormat");
}

void Nvidia::TEncoder::InitOutputFormat()
{
	//	nvidia demo calls this the capture plane
	auto& Encoder = *mEncoder;
	auto Profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE;
	auto Format = V4L2_PIX_FMT_H264;
	auto Width = 100;
	auto Height = 100;
	auto BufferSize = 1024 * 1024 * 2;
	auto Result = Encoder.setCapturePlaneFormat( OutputFormat, Width, Height, BufferSize );
	IsOkay(Result,"InitOutputFormat failed setCapturePlaneFormat");

	//	set other params
	Result = Encoder.setBitrate(ctx.bitrate);
	IsOkay(Result,"Failed to set bitrate");

	setProfile
	auto Level = V4L2_MPEG_VIDEO_H264_LEVEL_5_1;
	Result = Encoder.setLevel(Level);
	IsOkay(Result,"Failed to set level");

	
	//	setup memory read mode
	ret = ctx.enc->output_plane.setupPlane(V4L2_MEMORY_USERPTR, 10, false, true);
	ret = ctx.enc->output_plane.setupPlane(V4L2_MEMORY_MMAP, 10, true, false);
			  ret = setup_output_dmabuf(&ctx,10);
			  
	
}


	
/**
  * Set encoder context defaults values.
  *
  * @param ctx : Encoder context
  */
static void
set_defaults(context_t * ctx)
{
	// tsdk: These are carried over from the nvidia example
	memset(ctx, 0, sizeof(context_t));

	ctx->raw_pixfmt = V4L2_PIX_FMT_YUV420M;
	ctx->bitrate = 4 * 1024 * 1024;
	ctx->peak_bitrate = 0;
	ctx->profile = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE;
	ctx->ratecontrol = V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
	ctx->iframe_interval = 30;
	ctx->externalRPS = false;
	ctx->enableGDR = false;
	ctx->enableROI = false;
	ctx->bnoIframe = false;
	ctx->bGapsInFrameNumAllowed = false;
	ctx->bReconCrc = false;
	ctx->enableLossless = false;
	ctx->nH264FrameNumBits = 0;
	ctx->nH265PocLsbBits = 0;
	ctx->idr_interval = 256;
	ctx->level = -1;
	ctx->fps_n = 30;
	ctx->fps_d = 1;
	ctx->gdr_start_frame_number = 0xffffffff;
	ctx->gdr_num_frames = 0xffffffff;
	ctx->gdr_out_frame_number = 0xffffffff;
	ctx->num_b_frames = (uint32_t) -1;
	ctx->nMinQpI = (uint32_t)QP_RETAIN_VAL;
	ctx->nMaxQpI = (uint32_t)QP_RETAIN_VAL;
	ctx->nMinQpP = (uint32_t)QP_RETAIN_VAL;
	ctx->nMaxQpP = (uint32_t)QP_RETAIN_VAL;
	ctx->nMinQpB = (uint32_t)QP_RETAIN_VAL;
	ctx->nMaxQpB = (uint32_t)QP_RETAIN_VAL;
	ctx->use_gold_crc = false;
	ctx->pBitStreamCrc = NULL;
	ctx->externalRCHints = false;
	ctx->input_metadata = false;
	ctx->sMaxQp = 51;
	ctx->stats = false;
	ctx->stress_test = 1;
	ctx->output_memory_type = V4L2_MEMORY_DMABUF;
	ctx->copy_timestamp = false;
	ctx->start_ts = 0;
	ctx->max_perf = 0;
	ctx->blocking_mode = 1;
	ctx->startf = 0;
	ctx->endf = 0;
	ctx->num_output_buffers = 6;
	ctx->num_frames_to_encode = -1;
}

// Initialisation
// Create Video Encoder
void Nvidia::TEncoder::AllocEncoder(const SoyPixelsMeta& Meta)
{
	// Create encoder context.
	context_t ctx;

}

// Reference encode_proc in the nvidia_encode_main example
void Nvidia::TEncoder::Encode(const SoyPixelsImpl& Luma, const SoyPixelsImpl& ChromaU, const SoyPixelsImpl& ChromaV, context_t& ctx)
{
	// Create a var that holds a return value that is written over
	int ret = 0;
	// Any Error codes
	int error = 0;
	// End Of Stream, this runs on a loop until this becomes true
	bool eos = false;

	/* Set default values for encoder context members. */
	set_defaults(&ctx);

	/* Set thread name for encoder Output Plane thread. */
	pthread_setname_np(pthread_self(),"EncOutPlane");

	if (ctx.endf) {
		IsOkay(ctx.startf > ctx.endf, "End frame should be greater than start frame");
		ctx.num_frames_to_encode = ctx.endf - ctx.startf + 1;
	}

	/* Open input file for raw yuv, where does this file come from?*/
	ctx.in_file = new std::ifstream(ctx.in_file_path);
	IsOkay(!ctx.in_file->is_open(), "Could not open input file");

		if (!ctx.stats)
	{
		/* Open output file for encoded bitstream */
		ctx.out_file = new std::ofstream(ctx.out_file_path);
		IsOkay(!ctx.out_file->is_open(), "Could not open output file");
	}

	// Create NvVideoEncoder object in blocking mode
	ctx.enc = NvVideoEncoder::createVideoEncoder("enc0");

	IsOkay(!ctx.enc, "Could not create encoder");

/* Set encoder capture plane format.
NOTE: It is necessary that Capture Plane format be set before Output Plane
format. It is necessary to set width and height on the capture plane as well */
	ret = ctx.enc->setCapturePlaneFormat(V4L2_PIX_FMT_H264, ctx.width,
									  ctx.height, 2 * 1024 * 1024);
	IsOkay(ret < 0, "Could not set capture plane format");
}

// tsdk: V4L2 is coming in here 
int NvVideoEncoder::setCapturePlaneFormat(uint32_t pixfmt, uint32_t width, uint32_t height, uint32_t sizeimage)
{
	struct v4l2_format format;

	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	switch (pixfmt)
	{
		case V4L2_PIX_FMT_H264:
		case V4L2_PIX_FMT_H265:
		case V4L2_PIX_FMT_VP8:
		case V4L2_PIX_FMT_VP9:
			capture_plane_pixfmt = pixfmt;
			break;
		default:
			ERROR_MSG("Unknown supported pixel format for encoder " << pixfmt);
			return -1;
	}

	format.fmt.pix_mp.pixelformat = pixfmt;
	format.fmt.pix_mp.width = width;
	format.fmt.pix_mp.height = height;
	format.fmt.pix_mp.num_planes = 1;
	format.fmt.pix_mp.plane_fmt[0].sizeimage = sizeimage;

	return capture_plane.setFormat(format);
}
