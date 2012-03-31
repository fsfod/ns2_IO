#pragma once
#include "Stdafx.h"
#include "SourceCollection.h"
#include "MountedFile.h"

class Archive;
class DirectoryFileSource;
class ResourceOverrider;


class FileSource;



class SourceManager{

public: 
  static void SetupFilesystemMounting();

  static DirectoryFileSource* GetDirectorySourceForPath(const PlatformPath& path);

  static Archive* OpenArchive(const PlatformPath& path);
  static void ArchiveClosed(Archive* arch);

  static void CloseAllArchives();

  static void MountSource(::FileSource* source);
  static void MountSourcePathOnce(::FileSource* source);

  static int UnmountFilesFromSource(::FileSource* source);
  //static void MountFile(const string& path, Archive* source, int index, bool TriggerModifed = false);
  
  static void MountFile(const string& path, MountedFile& File, bool TriggerModifed = false);

  //static void MountFile(const string& path, DirectoryFileSource* source, PlatformPath& realPath, bool TriggerModifed = false);
  static void MountFilesList(Archive* source, vector<pair<string, int>>& fileList, int GroupId, bool triggerModifed = false );
  static bool UnmountFile( const string& path );

private:
  static std::map<PlatformPath::string_type, Archive*> OpenArchives;

  static std::map<PlatformPath, DirectoryFileSource*> CreatedFileSources;

  static std::map<string, int> GroupNameToId;

  static ResourceOverrider* Overrider;
  static SourceCollection* ExtraSources;
};

