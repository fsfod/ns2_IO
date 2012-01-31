#include "StdAfx.h"
#include "NSRootDir.h"
#include "LuaModule.h"
#include "StringUtil.h"
#include "SourceManager.h"
#include "DirectoryChangeWatcher.h"


//using namespace std;

namespace boostfs = boost::filesystem ;


DirectoryFileSource::DirectoryFileSource(char* dirName): ParentSource(NULL), GameFileSystemPath(dirName), ChangeWatcher(NULL){

	RealPath = NS2Env.NSRootPath/dirName;

	if(GameFileSystemPath.back() != '/'){
		GameFileSystemPath.push_back('/');
	}
}

DirectoryFileSource::DirectoryFileSource(const PlatformPath& DirectoryPath, const PathString& GamePath, DirectoryFileSource* parentSource)
 :GameFileSystemPath(GamePath), RealPath(DirectoryPath), ParentSource(parentSource), ChangeWatcher(NULL){

 if(!GameFileSystemPath.empty() && GameFileSystemPath.back() != '/'){
	 GameFileSystemPath.push_back('/');
 }
}

DirectoryFileSource::~DirectoryFileSource(){
  if(ChangeWatcher != NULL){
    delete ChangeWatcher;
  }
}

bool DirectoryFileSource::FileExists(const PathStringArg& FilePath){
	return boostfs::exists(FilePath.CreatePath(RealPath));
}

bool DirectoryFileSource::FileExists(const string& FilePath){
  return boostfs::exists(RealPath/FilePath);
}

bool DirectoryFileSource::FileSize(const PathStringArg& path, double& Filesize ){
	auto fullpath = path.CreatePath(RealPath);

	if(!boostfs::exists(fullpath)){
		return false;
	}else{
		Filesize = (double)boostfs::file_size(fullpath);
	 return true;
	}
}

bool DirectoryFileSource::DirectoryExists( const PathStringArg& DirectoryPath ){
	auto fullpath = DirectoryPath.CreatePath(RealPath);

	int result = GetFileAttributes(fullpath.c_str());

	return result != INVALID_FILE_ATTRIBUTES && (result&FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int DirectoryFileSource::FindFiles(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundFiles){
  int count = 0;

  ForEachFile(SearchPath.CreateSearchPath("", NamePatten), [=,&FoundFiles ,&count](const FileInfo& file)mutable ->void{
    if(!file.IsDirectory()){
      FoundFiles[UTF16ToUTF8STLString(file.cFileName)] = this;
      count++;
    }
  });

	return count;
}

int DirectoryFileSource::FindDirectorys(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundDirectorys){

  int count = 0;

  ForEachFile(SearchPath.CreateSearchPath("", NamePatten), [=, &FoundDirectorys,&count](const FileInfo& file)mutable ->void{
    if(file.IsDirectory()){
      FoundDirectorys[UTF16ToUTF8STLString(file.cFileName)] = this;
      count++;
    }
  });

  return count;
}

bool DirectoryFileSource::GetModifiedTime( const PathStringArg& Path, int32_t& Time ){

	auto fullpath = Path.CreatePath(RealPath);

#ifdef UNICODE 
	struct _stat32 FileInfo;

	int result = _wstat32(fullpath.c_str(), &FileInfo);
#else
	struct stat FileInfo;

	int result = stat(fullpath.c_str(), &FileInfo);
#endif

	if(result == 0){
		return false;
	}

	Time = FileInfo.st_mtime;

	return true;
}

const string luaExt(".lua");

int DirectoryFileSource::LoadLuaFile( lua_State* L, const PathStringArg& FilePath )
{

	auto path = RealPath/ConvertAndValidatePath(FilePath);

  if(FilePath.GetExtension() != luaExt){
    throw std::runtime_error(FilePath.ToString() + " is not a lua file");
  }

  string chunkpath = path.string();
  //

  boost::replace(chunkpath, '/', '\\');

	int result = LuaModule::LoadLuaFile(L, path, ('@'+chunkpath).c_str() );

	//loadfile should of pushed either a function or an error message on to the stack leave it there as the return value
	return result;
}

int DirectoryFileSource::RelativeRequire(lua_State* L, const PathStringArg& ModuleName ){
  
  auto SearchPath = RealPath/ConvertAndValidatePath(ModuleName)/"?.dll";

  auto package = luabind::globals(L)["package"];

  //relative 
  try{
    string OriginalCPath = luabind::object_cast<string>(package["cpath"]);
    string OriginalPath = luabind::object_cast<string>(package["path"]);

    package["cpath"] = SearchPath.string();


    package["cpath"] = OriginalCPath;
    package["path"] = OriginalPath;
  }catch(luabind::cast_failed e){
    
  }

  return 0;
}

////////////////////////////////////////////////////////////////////
////////////////////        LUA Wrappers        ////////////////////
////////////////////////////////////////////////////////////////////


const string& DirectoryFileSource::get_Path(){
	return GameFileSystemPath;
}

extern ResourceOverrider* OverrideSource;

void DirectoryFileSource::MountFile( const PathStringArg& FilePath, const PathStringArg& DestinationPath ){

  ConvertAndValidatePath(FilePath);

  PlatformPath path = FilePath.CreatePath(RealPath);

  if(!boostfs::exists(path)){
    throw std::exception("Cannot mount a file that does not exist");
  }

  MountedFiles.insert(std::make_pair(FilePath.GetNormalizedPath(), DestinationPath.GetNormalizedPath()));

  SourceManager::MountFile(DestinationPath.GetNormalizedPath(), MountedFile(this, path, true));
}

class MountFileIT{
public:
  MountFileIT(PlatformPath& currentPath,  string& DestPath) : CurrentPath(currentPath), CurrentDestPath(DestPath),
    CurrentPathS(const_cast<std::wstring&>(CurrentPath.native()) ){
  }

  __forceinline void operator()(const FileInfo& file){
    if(file.IsDirectory()){
      int namesize = CurrentDestPath.size();

      CurrentDestPath.append(file.GetName());
      CurrentDestPath.push_back('/');

      CurrentPath /= file.cFileName;

      namesize = CurrentDestPath.size()-namesize;

      //lambda recursion fun times
      //ForEachFile(CurrentPath, foreachFunc);

      //CurrentPath.remove_filename()

      //shrink the path back down one directory
      CurrentPathS.resize(CurrentPathS.size()-namesize);
      CurrentDestPath.resize(CurrentDestPath.size()-namesize);

    }else{
      CurrentPath/file.cFileName;
    }
  }

private:
  string CurrentDestPath;
  PlatformPath CurrentPath;
  std::wstring& CurrentPathS;

};

inline void AppendDirtoPath(string& path, const string& dir){
  path.reserve(path.size()+dir.size()+2);
  path.append(dir);
  path.push_back('/');
}

void DirectoryFileSource::MountFiles(const PathStringArg& BasePath, const PathStringArg& DestinationPath){
  ConvertAndValidatePath(BasePath);

  string NormBasePath = BasePath.GetNormalizedPath();
   NormBasePath.push_back('/');

  PlatformPath CurrentPath = BasePath.CreatePath(RealPath);
  string CurrentDestPath = DestinationPath.GetNormalizedPath();

  if(!CurrentDestPath.empty() && CurrentDestPath.back() != '/'){
    CurrentDestPath.push_back('/');
  }

  std::wstring& CurrentPathS =  const_cast<std::wstring&>(CurrentPath.native());
  CurrentPathS.reserve(255);

  int pathOffset  = RealPath.native().size();

  std::function<void(const FileInfo&)> foreachFunc = [&](const FileInfo& file) mutable ->void {
    if(file.IsDirectory()){
      int namesize = CurrentPathS.size();
      
      string dirname = file.GetNormlizedNameWithSlash();

      NormBasePath.append(dirname);
      CurrentDestPath.append(dirname);

      CurrentPath /= file.cFileName;
      
      //lambda recursion fun times
      ForEachFile(CurrentPath.c_str()+pathOffset, foreachFunc);

      //shrink the path back down one directory
      CurrentPathS.resize(namesize);

      CurrentDestPath.resize(CurrentDestPath.size()-dirname.size());
      NormBasePath.resize(NormBasePath.size()-dirname.size());
    }else{
      //CurrentDestPath
      string FileName = file.GetNormlizedName();
      string MountedPath = CurrentDestPath+FileName;

      SourceManager::MountFile(MountedPath, MountedFile(this, CurrentPath/file.cFileName, false));

      MountedFiles.insert(std::make_pair(NormBasePath+FileName, std::move(MountedPath))); 
    }
  };

  ForEachFile(BasePath.ToString(),  foreachFunc);
}

M4::File* DirectoryFileSource::GetEngineFile( M4::Allocator* alc, const string& path )
{
  PlatformPath fullpath = RealPath/path;

  if(!boostfs::exists(fullpath))return NULL;



  return new (alc->AllocateAligned(sizeof(DirEngineFile), 4)) DirEngineFile(fullpath);
}

DirectoryFileSource* DirectoryFileSource::CreateChildSource( const string& SubDirectory ){
  if(ParentSource != NULL)throw exception("Cannot create child source from non root DirectoryFileSource");

  auto node = ChildSources.find(SubDirectory);

  if(node != ChildSources.end()){
    return node->second;
  }

  PlatformPath path = RealPath/SubDirectory;

  if(!boostfs::exists(path)){
    throw exception("DirectoryFileSource::CreateChildSource: cannot create child source because that subdirectory does not exist");
  }

  return new DirectoryFileSource(path, GameFileSystemPath+SubDirectory, this);
}

std::int64_t DirectoryFileSource::GetFileModificationTime(const string& path){
 return boostfs::last_write_time(RealPath/path);
}

void DirectoryFileSource::GetChangedFiles( VC05Vector<VC05string>& changes ){
  //just let the parent collect changes
  //if(ParentSource != NULL)return;

  if(ChangeWatcher == NULL){
    ChangeWatcher = new DirectoryChangeWatcher(RealPath);
  }else{
    ChangeWatcher->CheckForChanges(changes);


    typedef pair<const std::string, MountedFile>* ListEntry;

    vector<ListEntry> files;

    BOOST_FOREACH(ListEntry file,  files){
      if(false){
      }
    }
    //GetFilesFromSource();
  }
}

bool DirectoryFileSource::FileExist(const string& path){
  return boostfs::exists(RealPath/path);
}
