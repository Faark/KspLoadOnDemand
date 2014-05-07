#pragma once 


using namespace System;
using namespace System::Collections::Concurrent;



/*
Yes, this an ultra easy memory allocation system. Always wanted to write and hopefully soon optimize sth like that.

While not mentioned otherwise, allocation should only be done from a single thread while other threads can free it as well
*/
ref class BufferMemory{
public:
	interface class ISegment
	{
		property array<Byte>^ EntireBuffer{array<Byte>^ get(); }
		property int SegmentStartsAt{int get(); }
		property int SegmentLength{int get(); }
		void Free();
	};
private:
	ref class Segment : public ISegment{
	public:
		enum class SegmentState{ Free, Releasing, Used, Unreferenced };
		int mSegmentIndex;
		BufferMemory^ mHost;
		int mSegmentLength;
		int mSegmentStartsAt;
		array<Byte>^ mEntireBuffer;
		int mNextSegment;
		SegmentState mState;

		property array<Byte>^ EntireBuffer{virtual array<Byte>^ get(){ return mEntireBuffer; }}
		property int SegmentStartsAt{virtual int get(){ return mSegmentStartsAt; }}
		property int SegmentLength { virtual int get(){ return mSegmentLength; }}
		virtual void Free(){
			mState = SegmentState::Releasing;
			mHost->mSegmentsToFree->Enqueue(this);
		}
		Segment(BufferMemory^ host, array<Byte>^ entireBuffer){
			mSegmentIndex = 0;
			mHost = host;
			mSegmentLength = mEntireBuffer->Length;
			mSegmentStartsAt = 0;
			mEntireBuffer = entireBuffer;
			mNextSegment = -1;
			mState = SegmentState::Free;
		}
		Segment(BufferMemory^ host, array<Byte>^ entireBuffer, int segmentIndex){
			mSegmentIndex = segmentIndex;
			mHost = host;
			mSegmentLength = -1;
			mSegmentStartsAt = -1;
			mEntireBuffer = entireBuffer;
			mNextSegment = -1;
			mState = SegmentState::Unreferenced;
		}
		bool TrySplit(int size){
			auto unusedId = mHost->firstUnusedSegment;
			if (unusedId == -1){
				return false;
			}
			else{
				auto unusedSegm = mHost->mSegments[unusedId];
				mHost->firstUnusedSegment = unusedSegm->mNextSegment;
				unusedSegm->mNextSegment = mNextSegment;
				mNextSegment = unusedId;
				unusedSegm->mSegmentLength = mSegmentLength - size;
				unusedSegm->mSegmentStartsAt = mSegmentStartsAt + size;
				mSegmentLength = size;
				return true;
			}
		}
	};
	ConcurrentQueue<Segment^>^ mSegmentsToFree; // todo: Exchange against "indexed" queue
	array<Byte>^ mBuffer = nullptr;
	array<Segment^>^ mSegments = nullptr;
	int firstSegment = -1;
	int firstUnusedSegment = -1;
	void Init(int bufferSize){
		int segment_count = mSegments->Length;
		mBuffer = gcnew array<Byte>(0);
		mSegments[0] = gcnew Segment(this, mBuffer);
		for (int i = 1; i < segment_count; i++){
			auto segment = gcnew Segment(this, mBuffer, i);
			segment->mNextSegment = ((i == segment_count - 1) || (i == 0)) ? -1 : i + 1;
			mSegments[i] = segment;
		}
		firstSegment = 0;
		firstUnusedSegment = segment_count > 1 ? 1 : -1;
		mSegmentsToFree = gcnew ConcurrentQueue<Segment^>();
	}
	void addSegmentToUnused(Segment^ seg, int segId){
		seg->mNextSegment = firstUnusedSegment;
		firstUnusedSegment = segId;
	}
public:

	property int BufferSize{int get(){return mBuffer->Length;}}

	BufferMemory(int max_segments){
		if (max_segments < 1){
			throw gcnew ArgumentOutOfRangeException("max_segments");
		}
		mSegments = gcnew array<Segment^>(max_segments);
		Init(0);
	}


	/// Do not run while buffer segments are still allocated
	void ResizeBuffer(int new_size){
		mBuffer = nullptr;
		for (auto i = 0; i < mSegments->Length; i++){
			mSegments[i] = nullptr;
		}
		mSegmentsToFree = nullptr;

		GC::Collect();

		Init(new_size);
	}
	
	void TryCollectUnusedSegments(){
		Segment^ segmentToFree;
		while (mSegmentsToFree->TryDequeue(segmentToFree)){
			auto segmId = segmentToFree->mSegmentIndex;
			auto curEl = firstSegment;
			auto lastId = -1;
			segmentToFree->mState = Segment::SegmentState::Free;
			while (curEl != -1){
				if (segmId == curEl){
					auto nextId = segmentToFree->mNextSegment;
					if (lastId != -1){
						auto lastSegm = mSegments[lastId];
						if (lastSegm->mState == Segment::SegmentState::Free){
							addSegmentToUnused(segmentToFree, segmId);
							lastSegm->mNextSegment = segmId = nextId;
							segmentToFree = lastSegm;
						}
					}
					/* IF LAST .... */
					if (nextId != -1){
						auto nextSegm = mSegments[nextId];
						if (nextSegm->mState == Segment::SegmentState::Free){
							segmentToFree->mNextSegment = nextSegm->mNextSegment;
							addSegmentToUnused(nextSegm, nextId);
						}
					}
					break;
				}
				curEl = mSegments[curEl]->mNextSegment;
			}
		}
	}
	bool TryAlloc(int size, ISegment^% allocatedBuffer){
		TryCollectUnusedSegments();

		auto curId = firstSegment;
		while (curId != -1){
			auto curSegm = mSegments[curId];
			if ((curSegm->mState == Segment::SegmentState::Free) && (curSegm->mSegmentLength >= size)){
				if ((curSegm->mSegmentLength == size) || curSegm->TrySplit(size)){
					curSegm->mState = Segment::SegmentState::Used;
					allocatedBuffer = curSegm;
					return true;
				}
			}
			curId = curSegm->mNextSegment;
		}
		return false;
	}
};
