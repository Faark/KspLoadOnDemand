#pragma once 


using namespace System;
using namespace System::Collections::Concurrent;
using namespace System::IO;

//#define BUFFERMEMORY_DEBUG 1


namespace LodNative{
	/*
	Yes, this an ultra easy memory allocation system. Always wanted to write and hopefully soon optimize sth like that.

	While not mentioned otherwise, allocation should only be done from a single thread while other threads can free it as well
	*/
	ref class BufferMemory{
	public:
		// Todo: Can we make this a struct without exposing anything additional?
		interface class ISegment
		{
			property array<Byte>^ EntireBuffer{array<Byte>^ get(); }
			property int SegmentStartsAt{int get(); }
			property int SegmentLength{int get(); }
			void Free();
			MemoryStream^ CreateStream();
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
				mHost->fireOnFree();
			}
			virtual MemoryStream^ CreateStream(){
				return gcnew MemoryStream(mEntireBuffer, mSegmentStartsAt, mSegmentLength, false);
			}
			Segment(BufferMemory^ host, array<Byte>^ entireBuffer){
				mSegmentIndex = 0;
				mHost = host;
				mEntireBuffer = entireBuffer;
				mNextSegment = -1;
				mSegmentLength = mEntireBuffer->Length;
				mSegmentStartsAt = 0;
				mState = SegmentState::Free;
			}
			Segment(BufferMemory^ host, array<Byte>^ entireBuffer, int segmentIndex){
				mSegmentIndex = segmentIndex;
				mHost = host;
				mEntireBuffer = entireBuffer;
				mNextSegment = -1;
				mSegmentLength = -1;
				mSegmentStartsAt = -1;
				mState = SegmentState::Unreferenced;
			}
			bool TrySplit(int size){
				auto unusedId = mHost->firstUnusedSegment;
				if (unusedId == -1){
					return false;
				}
				else{
					auto spiltSegm = mHost->mSegments[unusedId];
					mHost->firstUnusedSegment = spiltSegm->mNextSegment;
					spiltSegm->mNextSegment = mNextSegment;
					mNextSegment = unusedId;
					spiltSegm->mSegmentLength = mSegmentLength - size;
					spiltSegm->mSegmentStartsAt = mSegmentStartsAt + size;
					spiltSegm->mState = mState;
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
			mBuffer = gcnew array<Byte>(bufferSize);
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
		void fireOnFree(){
			/*OnFreeCallback^ onFree = OnFree;
			if (onFree != nullptr){
			onFree();
			}
			*/
			ElementFreed();
		}
	public:
		delegate void OnFreeCallback();
		event OnFreeCallback^ ElementFreed;

		property int BufferSize{int get(){ return mBuffer->Length; }}

		BufferMemory(int max_segments, int buffer_size){
			if (max_segments < 1){
				throw gcnew ArgumentOutOfRangeException("max_segments");
			}
			mSegments = gcnew array<Segment^>(max_segments);
			Init(buffer_size); //BUG
		}


		/// Do not run while buffer segments are still allocated
		bool CanResize(){
			TryCollectUnusedSegments();
			auto s = mSegments[firstSegment];
			return s->mNextSegment == -1 && s->mState == Segment::SegmentState::Free;
		}
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
#if BUFFERMEMORY_DEBUG 
						auto prevState = ToString();
#endif
						auto nextId = segmentToFree->mNextSegment;
						if (lastId != -1){
							auto lastSegm = mSegments[lastId];
							if (lastSegm->mState == Segment::SegmentState::Free){
								lastSegm->mSegmentLength += segmentToFree->mSegmentLength;
								addSegmentToUnused(segmentToFree, segmId);
								lastSegm->mNextSegment = segmId = nextId;
								curEl = lastId;
								segmentToFree = lastSegm;
							}
						}
						/* IF LAST .... */
						if (nextId != -1){
							auto nextSegm = mSegments[nextId];
							if (nextSegm->mState == Segment::SegmentState::Free){
								segmentToFree->mNextSegment = nextSegm->mNextSegment;
								segmentToFree->mSegmentLength += nextSegm->mSegmentLength;
								addSegmentToUnused(nextSegm, nextId);
							}
						}
#if BUFFERMEMORY_DEBUG 
						Logger::LogText("Collected Segment" + segmId + ", " + prevState + " to " + ToString());
#endif
						break;
					}
					lastId = curEl;
					curEl = mSegments[curEl]->mNextSegment;
				}
			}
		}
		bool TryAlloc(int size, ISegment^% allocatedBuffer){
			TryCollectUnusedSegments();


#if BUFFERMEMORY_DEBUG 
			auto prevState = ToString();
#endif

			auto curId = firstSegment;
			while (curId != -1){
				auto curSegm = mSegments[curId];
				if ((curSegm->mState == Segment::SegmentState::Free) && (curSegm->mSegmentLength >= size)){
					if ((curSegm->mSegmentLength == size) || curSegm->TrySplit(size)){
						curSegm->mState = Segment::SegmentState::Used;
						allocatedBuffer = curSegm;
#if BUFFERMEMORY_DEBUG 
						Logger::LogText("Allocated " + curId + " with " + size + " bytes, " + prevState + " to " + ToString());
#endif
						return true;
					}
				}
				curId = curSegm->mNextSegment;
			}
			return false;
		}
		bool TryAllocSecondary(int secondarySize, int primarySize, ISegment^% allocatedSecondaryBuffer){
			if (BufferSize < secondarySize + primarySize){
				return false; // it won't fit..
			}

			TryCollectUnusedSegments();

#if BUFFERMEMORY_DEBUG 
			auto prevState = ToString();
#endif

			auto curId = firstSegment;
			while (curId != -1){
				auto curSegm = mSegments[curId];
				if (curSegm->SegmentStartsAt > primarySize){
					if ((curSegm->mState == Segment::SegmentState::Free) && (curSegm->mSegmentLength >= secondarySize)){
						if ((curSegm->mSegmentLength == secondarySize) || curSegm->TrySplit(secondarySize)){
							curSegm->mState = Segment::SegmentState::Used;
							allocatedSecondaryBuffer = curSegm;
#if BUFFERMEMORY_DEBUG
							Logger::LogText("Allocated " + curId + " with " + secondarySize + " bytes [" + primarySize + " bytes safe], " + prevState + " to " + ToString());
#endif
							return true;
						}
					}
				}
				else{
					// Not enough space for primary before this block starts, but we might be able to use the later part of it...
					if ((curSegm->mState == Segment::SegmentState::Free) && ((secondarySize + primarySize) <= (curSegm->mSegmentStartsAt + curSegm->mSegmentLength))){
						if (curSegm->TrySplit(curSegm->mSegmentLength - secondarySize)){
							curSegm = mSegments[curSegm->mNextSegment];
							curSegm->mState = Segment::SegmentState::Used;
							allocatedSecondaryBuffer = curSegm;
#if BUFFERMEMORY_DEBUG 
							Logger::LogText("Allocated " + curId + " with " + secondarySize + " bytes [" + primarySize + " bytes safe], " + prevState + " to " + ToString());
#endif
							return true;
						}
					}
				}
				curId = curSegm->mNextSegment;
			}
			return false;
		}
		String^ ToString() override {
			auto sb = gcnew StringBuilder();
			sb->Append("Buffer[")->Append(mBuffer->Length)->Append("]: ");
			bool first = true;
			int segmId = firstSegment;
			while (segmId != -1){
				auto segm = mSegments[segmId];
				if (first){
					first = false;
				}
				else{
					sb->Append(", ");
				}
				sb->Append(segm->SegmentLength)->Append("=>")->Append(segm->mState.ToString());
				segmId = segm->mNextSegment;
			}
			return sb->ToString();
		}
	};
}