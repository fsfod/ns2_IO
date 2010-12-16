#pragma once

#include "StdAfx.h"

template <class FileRecord> class SourceDirectory{

typedef typename std::string StringType;

public:
	typedef typename SourceDirectory<FileRecord> SelfType;
	typedef typename std::hash_map<StringType, FileRecord>::value_type FileNode;
	typedef typename std::hash_map<StringType, SelfType>::value_type DirNode;

	SourceDirectory():Parent(NULL){}

	SourceDirectory(const StringType& name, SelfType* parent){
		Name = name;
		Parent = parent;
	}

	void FindFiles(PathString& Patten){

		BOOST_FOREACH(const FileNode& file, Files){
			//file.first.find_last_of(Patten);

			PathMatchSpecEx(file.first.c_str(), Patten.c_str(), PMSF_NORMAL);
		}
	}

	SelfType* GetDirectory(const StringType& name){

		SelfType& result = Directorys.find(name);

		if(result != Directorys.end()){
			return &((*ParentDir).second);
		}

		return NULL;
	}

	SelfType* CreateDirectory(const StringType& name){
		return 	&Directorys.insert(DirNode(name, SelfType(name, this))).first->second;
	}

	SelfType* GetCreateDirectory(const StringType& name, bool& Created){

		auto result = Directorys.find(name);

		if(result != Directorys.end()){
				Created = false;
			return &result->second;
		}else{
				Created = true;
			return 	&Directorys.insert(DirNode(name, SelfType(name, this))).first->second;
		}
	}

	SelfType* GetCreateDirectory(const StringType& name){

		auto result = Directorys.find(name);

		if(result != Directorys.end()){
			return &result->second;
		}else{
			return 	&Directorys.insert(DirNode(name, SelfType(name, this))).first->second;
		}
	}

	SelfType* BuildDirectoryObjectsForPath(const StringType& path, std::vector<int>& SlashIndexs, int& CreatedCount){

		CreatedCount = 0;
		int lastIndex = 0;
		SelfType* CurrentDir = this;


		for(int i = 0; i < SlashIndexs.size() &&  SlashIndexs[i] < path.length()+1 ; i++){
			StringType DirName;

			if(i == 0){
				DirName = path.substr(0, SlashIndexs[i]);
			}else{
				DirName = path.substr(lastIndex, SlashIndexs[i]-lastIndex);
			}
			
			bool Created = false;
			CurrentDir = CurrentDir->GetCreateDirectory(DirName, Created);

			lastIndex = SlashIndexs[i]+1;

			if(Created){
				SlashIndexs[i] = -SlashIndexs[i];
				++CreatedCount;
			}
		}

		return CurrentDir;
	}

public:
	SelfType* Parent;
	StringType Name;
	std::hash_map<StringType, FileRecord> Files;
	std::hash_map<StringType, SelfType> Directorys;
};