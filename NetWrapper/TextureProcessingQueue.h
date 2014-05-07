#pragma once


using namespace System::Collections::Concurrent;

ref class TextureProcessingQueue{
private:
public:

	static bool GetLoadRequest(int %textureId){
		auto en = LoadQueue->GetEnumerator();
		try{
			if (en->MoveNext()){
				textureId = en->Current.Key;
				return true;
			}
			else{
				return false;
			}
		}
		finally{
			en->Dispose();
			delete en;
		}
	}
	static void RequestLoad(int textureId){
		if (!LoadQueue->TryAdd(textureId, true)){
			throw gcnew System::Exception("This should be impossible.");
		}
	}
	static bool CancelLoadRequest(int textureId){
		bool value;
		return LoadQueue->TryRemove(textureId, value);
	}
};