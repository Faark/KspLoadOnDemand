#pragma once


using namespace System;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Collections::Concurrent;
using namespace System::Collections::Generic;

#include "Logger.h"
#include "Work.h"
#include "ThreadPriority.h"
#include "BufferMemory.h"
/*
This is the much more convinient C# version of that class.


public static class Disk
{
static System.Collections.Concurrent.BlockingCollection<Func<Func<bool>>> PriorityJobs = new System.Collections.Concurrent.BlockingCollection<Func<Func<bool>>>();
static System.Collections.Concurrent.BlockingCollection<Func<Func<bool>>> WriteJobs = new System.Collections.Concurrent.BlockingCollection<Func<Func<bool>>>(5);
static System.Collections.Concurrent.BlockingCollection<Func<Func<bool>>> ReadJobs = new System.Collections.Concurrent.BlockingCollection<Func<Func<bool>>>();
static System.Collections.Concurrent.BlockingCollection<Func<Func<bool>>>[] AllJobs;
static Disk()
{
AllJobs = new[] { PriorityJobs, WriteJobs, ReadJobs };
var worker = new System.Threading.Thread(LoadThread);
worker.Name = "DiskIO Thread";
worker.IsBackground = true;
worker.Start();
}
static void LoadThread()
{
//"disk worker started".Log();
while (true)
{
Func<Func<bool>> currentJob;
if (!WriteJobs.TryTake(out currentJob))
{
if (!PriorityJobs.TryTake(out currentJob))
{
//"disk waiting for any job".Log();
System.Collections.Concurrent.BlockingCollection<Func<Func<bool>>>.TakeFromAny(AllJobs, out currentJob);
//"disk took any job".Log();
}
else
{
//"disk took priority job".Log();
}
}
else
{
//"disk took write job".Log();
}
try
{
var resultWriter = currentJob();
while (!resultWriter())
{
//"cannot submit job, waiting".Log();
if (WriteJobs.TryTake(out currentJob, TimeSpan.FromMilliseconds(200)))// Todo: any better solution than sleeping 200 ms?
{
//"disk took unblock write job".Log();
currentJob()();
}
}
}
catch (Exception err)
{
err.Log();
}
}
}

public static void RequestFile(String file, Action<byte[]> dataProcessor, bool isHighPriorityLoad = false)
{
RequestFileEx(file, data =>
{
return Work.TryScheduleDataProcessing(() =>
{
dataProcessor(data);
});
}, isHighPriorityLoad);
}
public static void RequestFileEx(String file, Func<byte[], bool> syncConsumer, bool highPriority = false)//?
{
Func<Func<bool>> loadJob = () =>
{
using (var fs = new System.IO.FileStream(file, System.IO.FileMode.Open, System.IO.FileAccess.Read, System.IO.FileShare.Read, 8 * 1024, System.IO.FileOptions.SequentialScan | System.IO.FileOptions.Asynchronous))
{
var buffer = new byte[fs.Length];
if (fs.Read(buffer, 0, buffer.Length) == buffer.Length)
{
return () =>
{
return syncConsumer(buffer);
};
}
else
{
throw new Exception("IO failed?");
}
}
};
//"disk adding read job".Log();
(highPriority ? PriorityJobs : ReadJobs).Add(loadJob);
}
public static void WriteFile(String file, byte[] data)
{
//"disk adding write job".Log();
WriteJobs.Add(() =>
{
System.IO.File.WriteAllBytes(file, data);// todo: good perf?
return () => true;
});
}
}
*/

namespace LodNative{
	ref class Disk
	{
	public:
		value struct IoRequest{
		private:
			String^ file;
			Action<BufferMemory::ISegment^>^ consumer;
		public:
			property String^ File{String^ get(){ return file; }}
			property Action<BufferMemory::ISegment^>^ Consumer{Action<BufferMemory::ISegment^>^ get(){ return consumer; }}
			IoRequest(String^ f, Action<BufferMemory::ISegment^>^ c){
				file = f;
				consumer = c;
			}
		};
		interface class IoRequestProvider{
			bool TryGetIoRequest(IoRequest% request);
		};
	private:
		value struct ImminentRequest{
		public:
			IoRequest Request;
			FileStream^ Stream;
			int Size;
			ImminentRequest(IoRequest req, FileStream^ stream, int size){
				Request = req;
				Stream = stream;
				Size = size;
			}
		};
		ref class DefaultIoRequestQueue : public IoRequestProvider{
			ConcurrentQueue<IoRequest>^ PriorityJobs = gcnew ConcurrentQueue<IoRequest>();
			ConcurrentQueue<IoRequest>^ ReadJobs = gcnew ConcurrentQueue<IoRequest>();
		public:
			virtual bool TryGetIoRequest(IoRequest% request){
				if (!PriorityJobs->TryDequeue(request)){
					if (!ReadJobs->TryDequeue(request)){
						return false;
					}
				}
				return true;
			}
			void AddRequest(IoRequest request, bool isHighPriority){
				(isHighPriority ? PriorityJobs : ReadJobs)->Enqueue(request);
			}
		};

		static BufferMemory^ mReadBuffer = gcnew BufferMemory(5, 32*1024*1024); // todo: make default buffer size customizable via cfg?
		static DefaultIoRequestQueue^ mDefaultIoRequestQueue = gcnew DefaultIoRequestQueue();
		static array<IoRequestProvider^>^ mIoRequestProviders;
		static BlockingCollection<Tuple<String^, array<Byte>^>^>^ WriteQueue = gcnew BlockingCollection<Tuple<String^, array<Byte>^>^>(2);

		static LinkedList<ImminentRequest>^ imminentRequests = gcnew LinkedList<ImminentRequest>();
		static LinkedList<ImminentRequest>^ emptyRequestNodes = gcnew LinkedList<ImminentRequest>();

		static AutoResetEvent^ gotNewWork = gcnew AutoResetEvent(false);

		static Disk(){
			mReadBuffer->ElementFreed += gcnew LodNative::BufferMemory::OnFreeCallback(&LodNative::Disk::OnElementFreed);
			mIoRequestProviders = gcnew array<IoRequestProvider^>{ mDefaultIoRequestQueue };
			for (int i = 0; i < 5; i++){
				//emptyRequestNodes->AddLast(ImminentRequest()); Todo: Find out why this line doesn't work...
				auto lln = gcnew LinkedListNode<ImminentRequest>(ImminentRequest());
				emptyRequestNodes->AddLast(lln);
			}
			Thread^ worker = gcnew Thread(gcnew ThreadStart(DiskThread));
			worker->Name = "DiskIO Thread";
			worker->IsBackground = true;
			worker->Start();
		}
		static void LodNative::Disk::OnElementFreed()
		{
			gotNewWork->Set();
		}

		static bool TryRunNextWriteJob(){
			Tuple<String^, array<Byte>^>^ writeJob;
			if (WriteQueue->TryTake(writeJob)){
				File::WriteAllBytes(writeJob->Item1, writeJob->Item2);
				return true;
			}
			return false;
		}

		enum class ReadJobStatus{ Ok, NoMemoryOrJob, DelayForResize };
		ref class ConsumeReadDataScope{
		public:
			BufferMemory::ISegment^ segment;
			Action<BufferMemory::ISegment^>^ consumer;
			void Run(){
				consumer(segment);
			}
			ConsumeReadDataScope(BufferMemory::ISegment^ s, Action<BufferMemory::ISegment^>^ c){
				segment = s;
				consumer = c;
			}
		};
		static void RunReadJob(ImminentRequest% request, BufferMemory::ISegment^ segment){
			if (request.Stream->Read(segment->EntireBuffer, segment->SegmentStartsAt, request.Size) != request.Size){
				delete request.Stream;
				throw gcnew Exception("IO failed?");
			}
			delete request.Stream;
			Work::ScheduleDataProcessing(gcnew Action(gcnew ConsumeReadDataScope(segment, request.Request.Consumer), &ConsumeReadDataScope::Run));
			/*
			FileStream^ fs = gcnew FileStream(file, FileMode::Open, FileAccess::Read, FileShare::Read, 8 * 1024, FileOptions::SequentialScan | FileOptions::Asynchronous);
			int len = (int)fs->Length;
			buffer = gcnew array<Byte>(len);
			if (fs->Read(buffer, 0, len) == len){
			delete fs;
			return gcnew Func<bool>(this, &RequestFileExScope::CompletionCheck);
			}
			else{
			delete fs;
			throw gcnew Exception("IO failed?");
			}*/
			//Work::TryScheduleDataProcessing
		}
		static ReadJobStatus TryRunReadJob(ImminentRequest% request, int % reserved_size){
			// get buffer (is this first? then any loc, else with reserved space)
			// Btw: Don't execute consumers on this thread!


			if (reserved_size == -1){
				// first request
				if (request.Size > mReadBuffer->BufferSize)
				{
					if (!mReadBuffer->CanResize()){
						return ReadJobStatus::DelayForResize;
					}
					else{
						mReadBuffer->ResizeBuffer(request.Size);
					}
				}

				BufferMemory::ISegment^ segment;
				if (mReadBuffer->TryAlloc(request.Size, segment)){
					RunReadJob(request, segment);
					return ReadJobStatus::Ok;
				}
				else{
					reserved_size = request.Size;
					return ReadJobStatus::NoMemoryOrJob;
				}
			}
			else{
				// secondary request

				BufferMemory::ISegment^ segment;
				if (mReadBuffer->TryAllocSecondary(request.Size, reserved_size, segment)){
					RunReadJob(request, segment);
					return ReadJobStatus::Ok;
				}
				else{
					return ReadJobStatus::NoMemoryOrJob;
				}
			}


		}
		static ReadJobStatus TryRunAnyReadJob(int % reserved_size){
			// try execute read job from "already requested" queue
			auto current = imminentRequests->First;
			while (current != nullptr){
				switch (TryRunReadJob(current->Value, reserved_size)){
				case ReadJobStatus::Ok:
					imminentRequests->Remove(current);
					emptyRequestNodes->AddLast(current);
					return ReadJobStatus::Ok;
				case ReadJobStatus::DelayForResize:
					return ReadJobStatus::DelayForResize;
				case ReadJobStatus::NoMemoryOrJob:
					// lets try the next one...
					current = current->Next;
					continue;
				default:
					throw gcnew InvalidOperationException();
				}
			}
			return ReadJobStatus::NoMemoryOrJob;
		}
		static ReadJobStatus TryRequestAndRunReadJobsFromProviders(int %reserved_size){
			for each(auto requestProvider in mIoRequestProviders){
				auto freeImminentRequest = emptyRequestNodes->First;
				while (freeImminentRequest != nullptr){
					ImminentRequest req;
					if (requestProvider->TryGetIoRequest(req.Request)){
						req.Stream = gcnew FileStream(req.Request.File, FileMode::Open, FileAccess::Read, FileShare::Read, 8 * 1024, FileOptions::SequentialScan | FileOptions::Asynchronous);
						req.Size = (int)req.Stream->Length;


						switch (TryRunReadJob(req, reserved_size)){
						case ReadJobStatus::Ok:
							return ReadJobStatus::Ok;
						case ReadJobStatus::DelayForResize:
							return ReadJobStatus::DelayForResize;
						case ReadJobStatus::NoMemoryOrJob:{
							auto nextFree = freeImminentRequest->Next;
							emptyRequestNodes->Remove(freeImminentRequest);
							freeImminentRequest->Value = req;
							imminentRequests->AddLast(freeImminentRequest);
							freeImminentRequest = nextFree;
							break;
						}
						default:
							throw gcnew InvalidOperationException();
						}
						// try it... if mem, add to queue and get next. if ok, return accordingly.
					}
					else{
						break;
					}
				}
				if (freeImminentRequest == nullptr){
					return ReadJobStatus::NoMemoryOrJob;
				}
			}
			return ReadJobStatus::NoMemoryOrJob;

			// try get a job
			// try execute
			// break if job queue is getting to long...

			// also break if we have a resize request?
			// also what about some general queue size limit? Don't rly like it so far...
			// we don't need a limited callback queue anymore, since the amount of data loaded is limited by out buffer anyway... 
		}
		static void DiskThread(){
			LodNative::ThreadPriority::SetCurrentToBackground();
			try{
				while (true){
					gotNewWork->Reset();
					while (TryRunNextWriteJob()){}
					int reserved_size = -1;
					auto status = TryRunAnyReadJob(reserved_size);
					if (status == ReadJobStatus::NoMemoryOrJob){
						status = TryRequestAndRunReadJobsFromProviders(reserved_size);
					}
					switch (status){
					case ReadJobStatus::Ok:
						// we did sth, insta-restart...
						continue;
					case ReadJobStatus::NoMemoryOrJob:
						if (emptyRequestNodes->First == nullptr){

							gotNewWork->WaitOne();
						}
						else{
							gotNewWork->WaitOne(200);
						}
						continue;
						// todo:
						// No job => wait timed (since Prov dont have events...)
						// No mem => wait til event.
						// => We should split them or make providers event-based!
					case ReadJobStatus::DelayForResize:
						// wait usually means sleep or WaitEvent?
						gotNewWork->WaitOne();
						continue;
					}
					// if all failed... wait?!
					// A reset event would be great, i guess. Should be set diretly after WriteJob loop! Also signal on write request... ? We cant signal on RequestProvider, i'm afraid
					// Worst case: Wait for 200ms or sth like that...

					// more fine-grained reset events might be an idea... so waitForResize isn't woken by AddRead...
				}
			}
			catch (ThreadAbortException^){
			}
			/*catch (Exception^ err){
				// Should be caught by out unhandled exception handler...
				Logger::LogException(err);
				Logger::crashGame = true;
				throw;
				}*/
			finally{
				LodNative::ThreadPriority::ResetCurrentToNormal();
			}
		}

	public:
		static property IEnumerable<IoRequestProvider^>^ RequestProviders {
			IEnumerable<IoRequestProvider^>^ get(){ return mIoRequestProviders; }
			void set(IEnumerable<IoRequestProvider^>^ value){ mIoRequestProviders = System::Linq::Enumerable::ToArray(value); }
		}
		static void RequestFile(String^ file, Action<BufferMemory::ISegment^>^ dataConsumer, bool isHighPriorityLoad)
		{
			mDefaultIoRequestQueue->AddRequest(IoRequest(file, dataConsumer), isHighPriorityLoad);
			gotNewWork->Set();
		}
		static void RequestFile(String^ file, Action<BufferMemory::ISegment^>^ dataConsumer){
			RequestFile(file, dataConsumer, false);
		}
		static void WriteFile(String^ file, array<Byte>^ data)
		{
			WriteQueue->Add(Tuple::Create(file, data));
			gotNewWork->Set();
		}
	};
}