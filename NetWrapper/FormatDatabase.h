#pragma once

using namespace System;
using namespace System::IO;
using namespace System::Collections::Generic;

#include "ITexture.h"
#include "Pathing.h"
#include "BitmapFormat.h"
#include "BufferMemory.h"


namespace LodNative{
	ref class FormatDatabase{
	public:
		static List<Func<FileInfo^, BufferMemory::ISegment^, int, ITextureBase^>^>^ FormatDetectors = gcnew List<Func<FileInfo^, BufferMemory::ISegment^, int, ITextureBase^>^>();
		static Dictionary<Type^, Dictionary<Type^, Func<ITextureBase^, ITextureBase^>^>^>^ FromToConverters = gcnew Dictionary<Type^, Dictionary<Type^, Func<ITextureBase^, ITextureBase^>^>^>();


		// buffer segment will be consumed, unless there is an exception!
		static ITextureBase^ Recognize(String^ file, BufferMemory::ISegment^ data, int textureId){
			try{
				FileInfo^ fi = gcnew FileInfo(file);
				for each(Func<FileInfo^, BufferMemory::ISegment^, int, ITextureBase^>^ func in FormatDetectors){
					ITextureBase^ texture = func(fi, data, textureId);
					if (texture != nullptr){
						return texture;
					}
				}
				return BitmapFormat::LoadUnknownFile(fi, data, textureId);
			}
			catch (Exception^ err){
				throw gcnew FormatException("Failed to Recognize Texture [" + file + "]", err);
			}
		}
		static IAssignableTexture^ ConvertToAssignable(ITextureBase^ self){
			IAssignableTexture^ assignableTexture = dynamic_cast<IAssignableTexture^>(self);
			if (assignableTexture != nullptr){
				return assignableTexture;
			}
			else{
				// Todo: Use Path(from,current=>current is IAssignableTexture, grid) instead of just converting to bmp...
				return ConvertTo<BitmapFormat^>(self);
			}
		}
		generic<class TTexture> where TTexture: ITextureBase
			static TTexture ConvertTo(ITextureBase^ self){
				Type^ srcType = self->GetType();
				Type^ trgType = TTexture::typeid;
				ITextureBase^ currentTexture = self;

				// Todo: Cach hot paths...?! Might not worth the effort, since it just called few times per tex and mostly to generate thumbs...
				array<Func<ITextureBase^, ITextureBase^>^>^ path = Pathing<Type^, Func<ITextureBase^, ITextureBase^>^>::FindLinks(srcType, trgType, FromToConverters);
				if (path == nullptr)
				{
					throw gcnew Exception("Couldn't find a way to convert " + srcType->ToString() + " to " + trgType->ToString() + ". Have you added the necessary converters?");
				}
				else
				{
					for each(Func<ITextureBase^, ITextureBase^>^ currentStep in path)
					{
						currentTexture = currentStep(currentTexture);
					}
					return (TTexture)currentTexture;
				}
		}

		/* generic copy version?!
		generic<class TTexture> where TTexture: ITextureBase{
			static TTexture 
		}*/
	private:
#if unused
		ref class GetConverterJobScope{
		public:
			ITextureBase^ texture;
			GetConverterJobScope(ITextureBase^ tex){
				texture = tex;
			}
			AssignableData^ RunConvertion(AssignableFormat^ expectedFormat){
				auto modifiedTexture = texture->ConvertTo<BitmapFormat^>()->Resize(expectedFormat->Width, expectedFormat->Height)->ConvertTo(expectedFormat->Format);
				Func<AssignableFormat^, AssignableData^>^ func = nullptr;
				auto assignable = modifiedTexture->GetAssignableData(nullptr /* should already by in the expected format! */, func);
				if (assignable == nullptr){
					throw gcnew NotSupportedException("Must return assignable!");
				}
				return assignable;
			}
		};
	public:
		static Func<AssignableFormat ^, AssignableData ^> ^ GetConverterJob(ITextureBase^ texture){
			return gcnew Func<AssignableFormat^, AssignableData^>(gcnew GetConverterJobScope(texture), &GetConverterJobScope::RunConvertion);
		}
#endif
	private:
		generic<class TFromTexture, class TToTexture> where TFromTexture : ITextureBase where TToTexture : ITextureBase
		ref class AddConversionScope{
		private:
			Func<TFromTexture, TToTexture>^ Converter;
		public:
			AddConversionScope(Func<TFromTexture, TToTexture>^ converter){
				Converter = converter;
			}
			ITextureBase^ WrappedConverter(ITextureBase^ from){
				return Converter((TFromTexture)from);
			}
		};
	public:
		generic<class TFromTexture, class TToTexture> where TFromTexture : ITextureBase where TToTexture : ITextureBase
			static void AddConversion(Func<TFromTexture, TToTexture>^ converter)
		{
				Dictionary<Type^, Func<ITextureBase^, ITextureBase^>^>^ subs;
				if (!FromToConverters->TryGetValue(TFromTexture::typeid, subs))
				{
					subs = gcnew Dictionary<Type^, Func<ITextureBase^, ITextureBase^>^>();
					FromToConverters[TFromTexture::typeid] = subs;
				}
				subs[TToTexture::typeid] = gcnew Func<ITextureBase^, ITextureBase^>(gcnew AddConversionScope<TFromTexture, TToTexture>(converter), &AddConversionScope<TFromTexture, TToTexture>::WrappedConverter);
		}
	private:
		generic<class TTexture> where TTexture : ITextureBase
		ref class AddRecognitionScope{
			Func<System::IO::FileInfo^, BufferMemory::ISegment^, int, TTexture>^ Recognizer;
		public:
			AddRecognitionScope(Func<System::IO::FileInfo^, BufferMemory::ISegment^, int, TTexture>^ recognizer){
				Recognizer = recognizer;
			}
			ITextureBase^ Recognize(System::IO::FileInfo^ file, BufferMemory::ISegment^ bytes, int textureId){
				return Recognizer(file, bytes, textureId);
			}
		};

	public:
		generic<class TTexture> where TTexture : ITextureBase
			static void AddRecognition(Func<FileInfo^, BufferMemory::ISegment^, int, TTexture>^ recognizer)
		{
				if ((TTexture::typeid)->IsAbstract || (TTexture::typeid)->IsInterface)
				{
					throw gcnew ArgumentException("Type has to be the actually used class. Nothing abstract or even interfaces!");
				}
				FormatDetectors->Add(gcnew Func<System::IO::FileInfo^, BufferMemory::ISegment^, int, ITextureBase^>(gcnew AddRecognitionScope<TTexture>(recognizer), &AddRecognitionScope<TTexture>::Recognize));
		}

		static bool CouldDXTCompressionMakesAnySense(ITextureBase^ texture){
			return !texture->IsNormal && (texture->Width >= 16) && (texture->Height >= 16);
		}

	private:
		static BitmapFormat^ TryRecognizeTGA(FileInfo^ file, BufferMemory::ISegment^ data, int textureId);
		static FormatDatabase();
	};
}