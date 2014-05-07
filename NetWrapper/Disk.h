#pragma once


using namespace System;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Collections::Concurrent;

#include "Logger.h"
#include "Work.h"
#include "ThreadPriority.h"
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

ref class Disk
{
private:
	static BlockingCollection<Func<Func<bool>^>^>^ PriorityJobs = gcnew BlockingCollection<Func<Func<bool>^>^>();
	static BlockingCollection<Func<Func<bool>^>^>^ WriteJobs = gcnew BlockingCollection<Func<Func<bool>^>^>(5);
	static BlockingCollection<Func<Func<bool>^>^>^ ReadJobs = gcnew BlockingCollection<Func<Func<bool>^>^>();
	static array<BlockingCollection<Func<Func<bool>^>^>^>^ AllJobs;
	static Disk()
	{
		array<BlockingCollection<Func<Func<bool>^>^>^>^ AllJobsArray = { PriorityJobs, WriteJobs, ReadJobs };
		AllJobs = AllJobsArray;
		Thread^ worker = gcnew Thread(gcnew ThreadStart(LoadThread));
		worker->Name = "DiskIO Thread";
		worker->IsBackground = true;
		worker->Start();
	}
	static void LoadThread()
	{
		LodNative::ThreadPriority::SetCurrentToBackground();
		//"disk worker started".Log();
		try{
			while (true)
			{
				Func<Func<bool>^>^ currentJob;
				if (!WriteJobs->TryTake(currentJob))
				{
					if (!PriorityJobs->TryTake(currentJob))
					{
						//"disk waiting for any job".Log();
						BlockingCollection<Func<Func<bool>^>^>::TakeFromAny(AllJobs, currentJob);
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
#if NDEBUG
				try
				{
#endif
					Func<bool>^ resultWriter = currentJob();
					while (!resultWriter())
					{
						//"cannot submit job, waiting".Log();
						if (WriteJobs->TryTake(currentJob, TimeSpan::FromMilliseconds(200)))// Todo: any better solution than sleeping 200 ms?
						{
							//"disk took unblock write job".Log();
							currentJob()();
						}
					}
#if NDEBUG
				}
				catch (ThreadAbortException^){
					return;
				}
				/*catch (Exception^ err)
				{
				Logger::LogException(err);
				Logger::crashGame = true;
				throw;
				}*/
#endif
			}
		}
		finally{
			LodNative::ThreadPriority::ResetCurrentToNormal();
		}
	}

	ref class RequestFileExScope{
	public:
		String ^ file;
		Func<array<Byte>^, bool>^ syncConsumer;
		array<Byte>^ buffer;

		bool CompletionCheck(){
			return syncConsumer(buffer);
		}
		Func<bool>^ DoJobAndReturnCompletionChecker(){
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
			}
		}
		RequestFileExScope(String ^f, Func<array<Byte>^, bool>^ sc){
			file = f;
			syncConsumer = sc;
		}

	};
public:
	static void RequestFileEx(String^ file, Func<array<Byte>^, bool>^ syncConsumer, bool highPriority)//?
	{
		//BlockingCollection<Func<Func<bool>^>^>^ currentJobList = ;

		(highPriority ? PriorityJobs : ReadJobs)->Add(gcnew Func<Func<bool>^>(gcnew RequestFileExScope(file, syncConsumer), &RequestFileExScope::DoJobAndReturnCompletionChecker));

	}

	static void RequestFile(String^file, Action<array<Byte>^>^ dataProcessor){
		RequestFile(file, dataProcessor, false);
	}
private:
	ref class RequestFileScope{
	public:
		Action<array<Byte>^>^ dataProcessor;
		array<Byte>^ loadedData;
		String^ File;
		RequestFileScope(Action<array<Byte>^>^ dp, String^ file) : dataProcessor(dp), File(file) { }
		bool Consumer(array<Byte>^ data){
			loadedData = data;
			return Work::TryScheduleDataProcessing(gcnew Action(this, &RequestFileScope::RunProcessor));
		}
		void RunProcessor(){
#if _DISABLED_NDEBUG
			try{
#endif
				dataProcessor(loadedData);
#if _DISABLED_NDEBUG
			}
			catch (Exception^ err){
				throw gcnew Exception("Failed processing data from file: " + File, err);
			}
#endif
		}
	};
public:
	static void RequestFile(String^ file, Action<array<Byte>^>^  dataProcessor, bool isHighPriorityLoad)
	{
		RequestFileEx(file, gcnew Func<array<Byte>^, bool>(gcnew RequestFileScope(dataProcessor, file), &RequestFileScope::Consumer), isHighPriorityLoad);
	}
private:
	ref class WriteFileScope{
	public:
		String^ file;
		array<Byte>^ data;
		WriteFileScope(String^ f, array<Byte>^ d){
			file = f;
			data = d;
		}
		static bool CompletionCheck(){
			return true;
		}
		Func<bool>^ DoJobAndReturnCompletionCheck(){
			File::WriteAllBytes(file, data);
			return gcnew Func<bool>(&WriteFileScope::CompletionCheck);
		}
	};
public:
	static void WriteFile(String^ file, array<Byte>^ data)
	{
		WriteJobs->Add(gcnew Func<Func<bool>^>(gcnew WriteFileScope(file, data), &WriteFileScope::DoJobAndReturnCompletionCheck));
	}
	static bool WriteFile_Callback_Callback(){
		return true;
	}
};