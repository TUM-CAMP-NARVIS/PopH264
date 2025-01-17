#pragma once

#include "Array.hpp"
#include "HeapArray.hpp"
#include "SoyTime.h"
#include <functional>
#include <span>

class SoyPixelsImpl;


namespace json11
{
	class Json;
}

namespace ContentType
{
	//	fourcc content type enums
	enum Type : uint32_t
	{
		Unknown		= 0,
		Jpeg		= 'jpeg',
		EndOfFile	= 'eof!'
	};
}
		

namespace PopH264
{
	class TDecoder;
	class TDecoderParams;

	class PacketMeta_t;
	class TInputNaluPacket;
	typedef uint32_t FrameNumber_t;		//	this could be an index, or it could be time. Should essentially be a frame identifier
	constexpr FrameNumber_t FrameNumberInvalid = 0;	//	gr; should probably switch to uint32_t::max
	
	//	just shorthand names for cleaner constructors
	typedef std::function<void(const SoyPixelsImpl&,FrameNumber_t,const ::json11::Json&)> OnDecodedFrame_t;
	//	null frame number if not specific to a frame (ie. fatal decoder error)
	typedef std::function<void(const std::string&,FrameNumber_t*)> OnFrameError_t;
}

class PopH264::PacketMeta_t
{
public:
	ContentType::Type	mContentType = ContentType::Unknown;
	FrameNumber_t		mFrameNumber = 0;	//	warning, as we've split data into multiple nalu-packets per-frame, this is NOT unique
};

//	this is being deprecated for passing meta + span's and trying to keep large un-moving buffers
//	instead of holding lots of small buffers
class PopH264::TInputNaluPacket : public PopH264::PacketMeta_t
{
public:
	std::span<uint8_t>		GetData()	{	return std::span(mData);	}
public:
	std::vector<uint8_t>	mData;
};


class PopH264::TDecoderParams
{
public:
	TDecoderParams(){};
	TDecoderParams(json11::Json& Params);

	bool		StripH264EmulationPrevention()	{	return mStripEmulationPrevention;	}
	bool		PreferFramesInOrder()			{	return !mDontReorderFrames;	}

public:
	std::string	mDecoderName;
	//	gr: because unity doesn't let us initialise structs, we need to try and make
	//		all bool options default to false for our ideal default.
	bool		mVerboseDebug = false;
	bool		mAllowBuffering = false;		//	gr: this is off by default to get frames out ASAP. But we may prefer always in order!
	bool		mDontReorderFrames = false;		//	gr: preference is frames in order, but buffering may conflict with this
	bool		mDoubleDecodeKeyframe = false;
	bool		mDrainOnKeyframe = false;
	bool		mLowPowerMode = false;
	bool		mDropBadFrames = false;
	bool		mDecodeSei = false;			//	SEI on Avf gives us an error, so we skip it
	bool		mAsyncDecompression = false;	//	Avf experimental async decompression, which may or may not go on a background thread 
	bool		mStripEmulationPrevention = false;
	bool		mRequireHardwareDecoder = false;
	bool		mRequireSoftwareDecoder = false;
	
	//	on android, these are used to configure the format, and might affect input buffer sizes
	//	if zero, they're unused
	int32_t		mWidthHint = 640;
	int32_t		mHeightHint = 480;
	int32_t		mInputSizeHint = 0;
};


class PopH264::TDecoder
{
public:
	__deprecated_prefix TDecoder(OnDecodedFrame_t OnDecodedFrame,OnFrameError_t OnFrameError) __deprecated;
	TDecoder(const TDecoderParams& Params,OnDecodedFrame_t OnDecodedFrame,OnFrameError_t OnFrameError);
	
	void			Decode(std::span<uint8_t> PacketData,FrameNumber_t FrameNumber,ContentType::Type ContentType=ContentType::Unknown);

	//	gr: this has a callback because of flushing old packets. Need to overhaul the framenumber<->packet relationship
	void			PushEndOfStream();
	void			CheckDecoderUpdates();
	
protected:
	void			OnDecoderError(const std::string& Error);		//	as this isn't frame-specific, we're assuming it's fatal
	void			OnFrameError(const std::string& Error,FrameNumber_t Frame);
	void			OnDecodedFrame(const SoyPixelsImpl& Pixels,FrameNumber_t FrameNumber);
	void			OnDecodedFrame(const SoyPixelsImpl& Pixels,FrameNumber_t FrameNumber,const json11::Json& Meta);	
	void			OnDecodedEndOfStream();
	virtual bool	DecodeNextPacket()=0;	//	returns true if more data to proccess
	virtual void	CheckUpdates() {};
	
	bool			HasPendingData()		{	return !mPendingDatas.IsEmpty();	}
	void			PeekHeaderNalus(std::vector<uint8_t>& SpsBuffer,std::vector<uint8_t>& PpsBuffer);
	std::shared_ptr<TInputNaluPacket>	PopNextPacket();
	bool			PopNalu(ArrayBridge<uint8_t>&& Buffer,FrameNumber_t& FrameNumber);
	void			UnpopPacket(std::shared_ptr<TInputNaluPacket> Packet);

protected:
	TDecoderParams		mParams;
	
private:
	std::mutex				mPendingDataLock;
	//	this is currently a bit ineffecient (pool for quick fix?) but we need to keep input frame numbers in sync with data
	Array<std::shared_ptr<TInputNaluPacket>>	mPendingDatas;
	bool					mPendingDataFinished = false;	//	when we know we're at EOS
	
	OnDecodedFrame_t	mOnDecodedFrame;
	OnFrameError_t		mOnFrameError;
};
