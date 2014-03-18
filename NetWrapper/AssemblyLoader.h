#pragma once


using namespace System;
using namespace System::IO;
using namespace System::Reflection;


// A streamlined version of what loaded assemblies when this project was mostly implemented in C#
// Keep in mind that those Assemblies have to be loaded before any method referencing them is called (in case this project explicity links them)
static ref class AssemblyLoader{

private:
	static Assembly ^ OnAssemblyResolve(Object ^sender, ResolveEventArgs ^args)
	{
		array<Assembly^, 1>^ loaded_assemblies = AppDomain::CurrentDomain->GetAssemblies();
		for (int i = 0; i < loaded_assemblies->Length; i++){
			Assembly^ current_assembly = loaded_assemblies[i];
			if (current_assembly->FullName == args->Name){
				return current_assembly;
			}
		}
		return nullptr;
	}
	static AssemblyLoader(){
		AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(&OnAssemblyResolve);
	}
public:
	static void LoadEmbeddedAssembly(String^ manifestResourceStreamName){
		Stream^ data = Assembly::GetExecutingAssembly()->GetManifestResourceStream(manifestResourceStreamName);
		AppDomain::CurrentDomain->Load((gcnew BinaryReader(data))->ReadBytes((int)data->Length));

	}


};