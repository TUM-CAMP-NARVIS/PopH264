#include "TDecoder.h"



void PopH264::TDecoder::Decode(ArrayBridge<uint8_t>&& PacketData,std::function<void(const SoyPixelsImpl&,SoyTime)> OnFrameDecoded)
{
	{
		std::lock_guard<std::mutex> Lock(mPendingDataLock);
		mPendingData.PushBackArray(PacketData);
	}
	
	while ( true )
	{
		//	keep decoding until no more data to process
		if ( !DecodeNextPacket( OnFrameDecoded ) )
			break;
	}
}

bool PopH264::TDecoder::PopNalu(ArrayBridge<uint8_t>&& Buffer)
{
	std::lock_guard<std::mutex> Lock( mPendingDataLock );
	auto* PendingData = &mPendingData[mPendingOffset];
	auto PendingDataSize = mPendingData.GetDataSize()-mPendingOffset;
	
	auto GetNextNalOffset = [&]
	{
		//	todo: handle 001 as well as 0001
		for ( int i=3;	i<PendingDataSize;	i++ )
		{
			if ( PendingData[i+0] != 0 )	continue;
			if ( PendingData[i+1] != 0 )	continue;
			if ( PendingData[i+2] != 0 )	continue;
			if ( PendingData[i+3] != 1 )	continue;
			return i;
		}
		
		//	gr: to deal with fragmented data (eg. udp) we now wait
		//		for the next complete NAL
		//	we should look for EOF packets to handle this though
		//	assume is complete...
		//return (int)mPendingData.GetDataSize();
		return 0;
	};
	
	auto DataSize = GetNextNalOffset();
	//	no next nal yet
	if ( DataSize == 0 )
		return false;
	
	auto* Data = PendingData;
	auto PendingDataArray = GetRemoteArray( Data, DataSize );
	
	Buffer.Copy( PendingDataArray );
	RemovePendingData( DataSize );
	return true;
}

void PopH264::TDecoder::RemovePendingData(size_t Size)
{
	//	this function is expensive because of giant memmoves when we cut a small amount of data
	//	we should use a RingArray, but for now, have a start offset, and remove when the offset
	//	gets over a certain size

	//	only called from this class, so should be locked
	///std::lock_guard<std::mutex> Lock(mPendingDataLock);
	mPendingOffset += Size;
	static int KbThreshold = 1024 * 5;
	if ( mPendingOffset > KbThreshold * 1024 )
	{
		mPendingData.RemoveBlock(0, mPendingOffset);
		mPendingOffset = 0;
	}
}


