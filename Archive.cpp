#include "StdAfx.h"
#include "Archive.h"
#include "C7ZipLibrary.h"

#include "7zip/CPP/7zip/Archive/IArchive.h"
#include "7zip/CPP/Windows/PropVariant.h"
#include "7zip/CPP/7zip/ICoder.h"
#include "7zip/CPP/7zip/IPassword.h"

#include "7zip/CPP/Common/MyCom.h"

#include <boost/algorithm/string.hpp>  

using namespace std;

Archive::Archive(C7ZipLibrary* owner, PathString archivepath, IInArchive* reader){

	Reader = reader;

	uint32_t itemcount;
	Reader->GetNumberOfItems(&itemcount);
	
	vector<string> dirlist;

	for(uint32_t i = 0; i < itemcount ; i++){
		NWindows::NCOM::CPropVariant prop;

		if (Reader->GetProperty(i, kpidPath, &prop) != S_OK || prop.vt != VT_BSTR)continue;

		//just convert to uft8 early since it will make things easier when handling lua strings
		string& itempath = NormalizedPath(prop.bstrVal);

		if (Reader->GetProperty(i, kpidIsDir, &prop) != S_OK)continue;

		bool isDir = prop.vt == VT_BOOL ? prop.boolVal : false;

		if(!isDir){
			PathToFile[itempath] = FileEntry(i);
		}else{
			dirlist.push_back(itempath);
		}
	}

	vector<int> SlashIndexs;

	/*
	BOOST_FOREACH(string& dirpath, dirlist){
		
		int index = 0;

		while((index = dirpath.find('/', index+1)) != PathString::npos){
			SlashIndexs.push_back(index);
		}

		string ParentPath;

		int ParentEnd;

		if(SlashIndexs.size() != 0){
			int CreatedCount;
			ParentEnd = SlashIndexs.back();
			ParentPath = dirpath.substr(0, ParentEnd);

			auto ParentDir = PathToDirectory[ParentPath];

			if(ParentDir == NULL){
				SlashIndexs.push_back(dirpath.size());
				CreateDirectorysForPath(dirpath, SlashIndexs);
			}else{
				PathToDirectory[dirpath] = ParentDir->GetCreateDirectory(dirpath.substr(SlashIndexs.back()+1));
			}


		}else{
			PathToDirectory[dirpath] = GetCreateDirectory(dirpath);
		}

		SlashIndexs.clear();
	}
	*/

	AddFilesToDirectorys();
}


Archive::SelfType* Archive::CreateDirectorysForPath(const std::string& path, vector<int>& SlashIndexs){

	SelfType* Directory;

	if(SlashIndexs.size() > 1){
		int CreatedCount;

		Directory = BuildDirectoryObjectsForPath(path, SlashIndexs, CreatedCount);

		//if more than 1 directory was created or 
		if(SlashIndexs.back() > 0 || CreatedCount != 1){
			auto CurrentDir = Directory->Parent;

			int PrevSlash = abs(SlashIndexs[SlashIndexs.size()-1]);

			for(int i = SlashIndexs.size()-2; i != 0 ; --i){
				
				if(SlashIndexs[i] < 0){
					//remember its negative 
					int length = PrevSlash+SlashIndexs[i];

					string p = path.substr(0, -SlashIndexs[i]);
					PathToDirectory[p] = CurrentDir;
				}
				PrevSlash = abs(SlashIndexs[i]);
				CurrentDir = CurrentDir->Parent;
			}
		}
	}else{
		Directory = CreateDirectory(path);
	}

	//store the directory we created
	PathToDirectory[path] = Directory;

	return Directory;
}

typedef hash_map<PathString, FileEntry>::_Paircc FileNode;


void Archive::AddFilesToDirectorys(){
	
	vector<int> SlashIndexs = vector<int>();
	
	BOOST_FOREACH(FileNode& node, PathToFile){
		const string& path = node.first;

		int index = path.find_last_of('/');

		if(index != PathString::npos){
			string ParentPath = path.substr(0, index);

			SelfType* Parent = PathToDirectory[ParentPath];

			if(Parent == NULL){
				int index = 0;

				while((index = path.find('/', index+1)) != PathString::npos){
					SlashIndexs.push_back(index);
				}

				if(SlashIndexs.size() > 1){
					Parent = CreateDirectorysForPath(ParentPath, SlashIndexs);
				}else{
					Parent = GetCreateDirectory(ParentPath);
					PathToDirectory[ParentPath] = Parent;
				}

				SlashIndexs.clear();
			}
			
			Parent->Files[path.substr(index+1)] = FileEntry(node.second);
		}
	}
}


Archive::~Archive(void){

}

bool Archive::FileExists(const PathStringArg& path){
	return PathToFile.find(path.GetNormalizedPath()) != PathToFile.end();
}

bool Archive::DirectoryExists(const PathStringArg& path){
	return PathToDirectory.find(path.GetNormalizedPath()) != PathToDirectory.end();
}

int Archive::FindFiles(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundFiles){

		auto dir = PathToDirectory.find(SearchPath.GetNormalizedPath());

		auto files = (*dir).second->Files;

		if(dir != PathToDirectory.end()){
		 int count = 0;

		 string patten = NamePatten.ToString();

			BOOST_FOREACH(const FileNode& file, (*dir).second->Files){
				if(patten.empty() || PathMatchSpecExA(file.first.c_str(), patten.c_str(), PMSF_NORMAL)){
					FoundFiles[file.first] = static_cast<FileSource*>(this);
					count++;
				}
				return count;
			}
		}

	return 0;
}

int Archive::FindDirectorys(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundDirectorys){
/*
	SelfType* dir = PathToDirectory[SearchPath];

	if(dir == NULL)return 0;
	
*/

	auto dir = PathToDirectory.find(SearchPath.GetNormalizedPath());

	if(dir != PathToDirectory.end()){
		int count = 0;

		auto files = (*dir).second->Files;

		string patten = NamePatten.ToString();

		BOOST_FOREACH(const DirNode& entry, (*dir).second->Directorys){
			if(patten.empty() || PathMatchSpecExA(entry.first.c_str(), patten.c_str(), PMSF_NORMAL)){
				FoundDirectorys[entry.first] = static_cast<FileSource*>(this);
				count++;
			}
		}
		return count;
	}

	return 0;
}

bool Archive::FileSize(const PathStringArg& path, double& Filesize){

	auto file = Files.find(path.GetNormalizedPath());

	if(file == Files.end()){
		return false;
	}

	Filesize = GetFileEntrysSize(file->second);

	return true;

/*
	NWindows::NCOM::CPropVariant prop;
	
	auto file = Files.find(path);

	if(file == Files.end())return false;

	if(Reader->GetProperty(file->second.FileIndex, kpidSize, &prop) != S_OK)throw std::exception("Unknown error while trying to get size of compressed file");

	switch (prop.vt){
		case VT_UI1: Filesize = prop.bVal;
		case VT_UI2: Filesize = prop.uiVal;
		case VT_UI4: Filesize = prop.ulVal;
		case VT_UI8: Filesize = prop.uhVal.QuadPart;

		default:
			throw std::exception("Unknown error while trying to get size of compressed file");
	}

*/
		return 0;
}

int Archive::GetFileEntrysSize(const FileEntry& fileentry){
	
	NWindows::NCOM::CPropVariant prop;

	if(Reader->GetProperty(fileentry.FileIndex, kpidSize, &prop) != S_OK){
		throw std::exception("Unknown error while trying to get size of compressed file");
	}

	int64_t Filesize;

	switch (prop.vt){
		case VT_UI1: 
			Filesize = prop.bVal;
		break;

		case VT_UI2: 
			Filesize = prop.uiVal;
		break;

		case VT_UI4: 
			Filesize = prop.ulVal;
		break;

		case VT_UI8: 
			Filesize = prop.uhVal.QuadPart;
		break;

		default:
			throw std::exception("Unexpected value type while geting size of compressed file");
	}
	

	_ASSERT(Filesize < INT32_MAX);

	return (int32_t)Filesize;
}

bool Archive::GetModifiedTime(const PathStringArg& Path, int32_t& Time){
	return true;
}

class SimpleOutSteam : public ISequentialOutStream, public CMyUnknownImp{

public:
	MY_UNKNOWN_IMP

	SimpleOutSteam(): Size(0), Capacity(0), Position(0), buffer(NULL){
	}

	SimpleOutSteam(int size): Size(0), Capacity(0), Position(0), buffer(NULL){
		SetSize(size);
	}

	virtual ~SimpleOutSteam(){
		if(buffer != NULL){
			delete buffer;
			buffer = NULL;
		}
	}

	STDMETHOD(Write)(const void *data, UInt32 dataSize, UInt32 *processedSize){
		if(Position+dataSize > Capacity){
			SetSize(Capacity*2);
		}

		memcpy_s(buffer+Position, Capacity-Position, data, dataSize);

		*processedSize = dataSize;

		Position =+ dataSize;

		return S_OK;
	}
	
	STDMETHOD(SetSize)(Int64 newSize){
		
		if(newSize > Capacity){
			char* old = buffer;

			buffer = new char[newSize];

			if(Size != 0){
				memcpy_s(buffer, newSize, old, Size);
			}

			Capacity = newSize;

			if(old != NULL)delete old;
		}

		Size = newSize;

		if(Position > Size){
			Position = Size;
		}

		return S_OK;
	}

	int LoadChunk(lua_State* L){
		return luaL_loadbuffer(L, buffer, Position, "");
	}

private:
	char* buffer;

	int Size, Position, Capacity;

	friend class ExtractCallback;
};

class ExtractCallback: public IArchiveExtractCallback,
	public CMyUnknownImp{

public:
	MY_UNKNOWN_IMP

	ExtractCallback(int index, int size) : FileIndex(index), stream(size), Result(-1){
		stream.AddRef();
		stream.AddRef();
	}

		// IProgress
	STDMETHOD(SetTotal)(UInt64 size){
		return S_OK;
	}
	STDMETHOD(SetCompleted)(const UInt64 *completeValue){
		return S_OK;
	}

	// IArchiveExtractCallback
	STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode){
		_ASSERT(index == FileIndex);

		if(askExtractMode != NArchive::NExtract::NAskMode::kExtract)return S_OK;

		*outStream = &stream;

		return S_OK;
	}

	STDMETHOD(PrepareOperation)(Int32 askExtractMode){
		return S_OK;
	}

	int LoadLuaChunk(lua_State* L){
		switch(Result){
			case NArchive::NExtract::NOperationResult::kOK:
			break;

			case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
				throw exception("error extracting file unsupported compressing method used on file");
			break;

			case NArchive::NExtract::NOperationResult::kCRCError:
				throw exception("error extracting file crc mismatch");
			break;

			case NArchive::NExtract::NOperationResult::kDataError:
				throw exception("error extracting file unknown data error");
			break;
		}

		return stream.LoadChunk(L);
	}

	STDMETHOD(SetOperationResult)(Int32 OperationResult){
		_ASSERT(Result == -1);
		Result = OperationResult;
	 return S_OK;
	}
	
	SimpleOutSteam stream;

private:
	int Result;
	int FileIndex;
	
};

void Archive::LoadLuaFile(lua_State* L, const PathStringArg& FilePath){

	auto file = PathToFile.find(FilePath.GetNormalizedPath());
	
	if(file == PathToFile.end()){
			throw exception("cannot load a lua file that does not exist");
		return;
	}

	uint32_t index = file->second.FileIndex;

	int size = GetFileEntrysSize(file->second);
	 
	ExtractCallback extract(index, size);
	extract.AddRef();

	Reader->Extract(&index, 1, false, &extract);

	extract.LoadLuaChunk(L);

	//luaL_loadbuffer(L, , , "");

	return;
}