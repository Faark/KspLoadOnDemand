#pragma once


using namespace System;
using namespace System::Threading;
using namespace System::Collections::Concurrent;

#include "Logger.h"


ref class Work{
private:
	static BlockingCollection<Action^>^ PriorityQueue = gcnew BlockingCollection<Action^>();
	static BlockingCollection<Action^>^ WorkQueue = gcnew BlockingCollection<Action^>();
	static BlockingCollection<Action^>^ ProcessingQueue = gcnew BlockingCollection<Action^>(3);
	static array<BlockingCollection<Action^>^>^ AnyJob;
	static Work(){
		array<BlockingCollection<Action^>^>^ AnyJobArray = { PriorityQueue, WorkQueue, ProcessingQueue };
		AnyJob = AnyJobArray;
		Thread^ worker = gcnew Thread(gcnew ThreadStart(WorkThread));
		worker->Name = "Worker Thread";
		worker->IsBackground = true;
		worker->Start();
	}
	static void WorkThread(){
		while (true)
		{
			try
			{
				Action^ currentJob;
				if (!PriorityQueue->TryTake(currentJob)){
					if (!ProcessingQueue->TryTake(currentJob))
					{
						//"work wait for any job".Log();
						BlockingCollection<Action^>::TakeFromAny(AnyJob, currentJob);
					}
					else
					{
						//"work took process job".Log();
					}
				}
				currentJob();
			}
			catch (ThreadAbortException^){
				return;
			}
			catch (Exception^ err)
			{
				Logger::LogException(err);
				Logger::crashGame = true;
				throw;
			}
		}
	}




public:
	static void ScheduleWork(Action^ work)
	{
		//"work job added {0}-{1}".Log(WorkQueue.Count, ProcessingQueue.Count);
		WorkQueue->Add(work);
	}
	static void SchedulePriority(Action^ work){
		PriorityQueue->Add(work);
	}
	static bool TryScheduleDataProcessing(Action^ work)
	{
		if (ProcessingQueue->TryAdd(work))
		{
			//"process job added".Log();
			return true;
		}
		else
		{
			//"process job adding failed".Log();
			return false;
		}
	}
};