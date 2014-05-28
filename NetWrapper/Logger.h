#pragma once

using namespace System;
using namespace System::Collections::Concurrent;
using namespace System::IO;
using namespace System::Text;

#include "Lock.h"
#include "ThreadPriority.h"

namespace LodNative{
	ref class Logger{
		static String^ logFile = "LoadOnDemand.log";
#if _DEBUG
		static const bool Async = false;
#else
		static const bool Async = true;
#endif
		static ConcurrentQueue<Object^>^ MessageQueue = gcnew ConcurrentQueue<Object^>();
	public:


		static property String^ LogFile{ String^ get(){ return logFile; } }
		static void Setup(String^ cache_directory){
			logFile = System::IO::Path::Combine(cache_directory, LogFile);
			if (Async){
				System::IO::File::WriteAllText(LogFile, "Logging Started at " + DateTime::Now.ToString() + " (Async)" + Environment::NewLine);
				auto thread = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(&LogThread));
				thread->Name = "LogThread";
				thread->IsBackground = true;
				thread->Start();
			}
			else{
				System::IO::File::WriteAllText(LogFile, "Logging Started at " + DateTime::Now.ToString() + " (Sync)" + Environment::NewLine);
			}
		}


		static void LogThread_CommitData(StringBuilder^ sb){
			if (sb->Length > 0){
				File::AppendAllText(LogFile, sb->ToString());
				sb->Clear();
			}
		}
		static void LogThread(){
			LodNative::ThreadPriority::SetCurrentToBackground();
			try{
				auto sb = gcnew StringBuilder();
				while (true){
					Object^ current;
					if (MessageQueue->TryDequeue(current)){
						auto currentLogElement = dynamic_cast<Tuple<String^, DateTime>^>(current);
						if (currentLogElement != nullptr){
							sb->Append(currentLogElement->Item2.ToString())->Append(": ")->AppendLine(currentLogElement->Item1);
						}
						else{
							auto currentEvent = dynamic_cast<AutoResetEvent^>(current);
							LogThread_CommitData(sb);
							currentEvent->Set();
						}
					}
					else{
						LogThread_CommitData(sb);
						System::Threading::Thread::Sleep(500);
					}
				}
			}
			catch (Exception^ err){
				if (dynamic_cast<ThreadAbortException^>(err) == nullptr){
					File::AppendAllText(LogFile, DateTime::Now.ToString() + ": Logger crashed. " + err->ToString());
				}
			}
			finally{
				LodNative::ThreadPriority::ResetCurrentToNormal();
			}
		}



		static String^ ToLogInfo(IDirect3DTexture9*textureToGetInfosFrom){
			D3DSURFACE_DESC srfDesc;
			textureToGetInfosFrom->GetLevelDesc(0, &srfDesc);
			int fmt = srfDesc.Format;
			int pool = srfDesc.Pool;
			int type = srfDesc.Type;
			int mst = srfDesc.MultiSampleType;
			return "{ Format: " + fmt + ", Pool : " + pool + ", Type : " + type + ", Usage : " + srfDesc.Usage
				+ ", MST: " + mst + ", MSQ: " + srfDesc.MultiSampleQuality + ", Resolution: " + srfDesc.Width + "x" + srfDesc.Height + "}";
		}
		static void LogException(Exception^ error){
			LogText(error->ToString());
			if (Async){
				// Todo7: This can potentially wait a long time (500ms, atm)... think about reducing it (though exceptions shouldn't happen anyway)
				// Todo7: Atm, this will wait indefinetivly one a crashed log thread...
				auto resetEvent = gcnew AutoResetEvent(false);
				MessageQueue->Enqueue(resetEvent);
				resetEvent->WaitOne();
			}
		}

		static void LogText(String^ text){
			if (Async){
				MessageQueue->Enqueue(gcnew Tuple<String^, DateTime>(text, DateTime::Now));
			}
			else{
				Lock lock;
				try{
					lock.Take(MessageQueue);
					System::IO::File::AppendAllText(LogFile, System::DateTime::Now.ToString() + ": " + text + Environment::NewLine);
				}
				finally{
					lock.TryExit();
				}
			}
		}
		static void LogStack(String^ infoText){
			LogText(infoText + " @ " + Environment::NewLine + System::Environment::StackTrace);
		}
		static void LogTrace(String^ text){
			LogText(text);
		}

	};
}