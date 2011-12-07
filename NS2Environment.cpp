#include "StdAfx.h"
#include "NS2Environment.h"

NS2Environment NS2Env;

void NS2Environment::FindNSRoot(){
  HMODULE hmodule = GetModuleHandleW(NULL);

  if(hmodule == INVALID_HANDLE_VALUE){
    throw exception("NSIO failed to get the module handle to ns2.exe to find ns2 root directory");
  }

  wstring ExePath;

  ExePath.resize(MAX_PATH, 0);

  int pathLen = GetModuleFileName(hmodule, (wchar_t*)ExePath.c_str(), MAX_PATH);

  if(pathLen > MAX_PATH) {
    ExePath.resize(pathLen);

    pathLen = GetModuleFileName(hmodule, (wchar_t*)ExePath.c_str(), pathLen);
  }

  replace(ExePath, '\\', '/');

  PlatformPath exepath = PlatformPath(ExePath);

  if(exepath.filename() == "server.exe"){
    IsDedicatedServer = true;
  }

  NSRootPath = exepath.parent_path();
}

void NS2Environment::ProcessCommandline(){

  wstring NativeCommandLine = GetCommandLine();

  WStringToUTF8STLString(NativeCommandLine, CommandLine);

  int arg_postion = NativeCommandLine.find(_T("-game"));

  if(arg_postion != string::npos){
    PlatformString GameCmd = NativeCommandLine.substr(arg_postion+5);

    ParseGameCommandline(GameCmd);
  }
}

void NS2Environment::ParseGameCommandline(PlatformString& CommandLine ){

  if(CommandLine.length() < 2){
    throw exception("failed to parse \"game\" command line argument(find arg start)");
  }

  bool HasQuotedString = false;

  size_t startpos = CommandLine.find_first_not_of(_T(" =\t"));

  if(startpos == PathString::npos){
    throw exception("failed to parse \"game\" command line argument(find arg start)");
  }

  int end = PathString::npos;

  if(CommandLine[startpos] == '\"') {
    ++startpos;
    HasQuotedString = true;

    //find the closing quote
    end = CommandLine.find_first_of('\"', startpos);

    if(end == PathString::npos){
      throw exception("failed to parse \"game\" command line argument(can't find closing quote)");
    }
  }else{
    end = CommandLine.find_first_of(_T(" \t"), startpos);
  }

  //end could equal string::npos if the -game string was right at the end of the commandline
  PlatformString GameCmdPath = CommandLine.substr(startpos, end != PathString::npos ? end-startpos : PathString::npos);

  replace(GameCmdPath, '\\', '/');

  int FirstSlash = GameCmdPath.find('/');

  GameIsZip = false;

  if(FirstSlash == PathString::npos || FirstSlash == GameCmdPath.size()-1){
    GameStringPath = NSRootPath/GameCmdPath;
  }else{
    GameStringPath = GameCmdPath;

    //if the path has no root name i.e. "somedir\mymod" treat it as relative to NSRoot
    if(!GameStringPath.has_root_name()){
      GameStringPath = NSRootPath/GameCmdPath;
    }
  }

  auto ext = GameStringPath.extension().wstring();
  boost::to_lower(ext);

  if(!boostfs::exists(GameStringPath)){
    if(!ext.empty()){
      throw exception("Zip file passed to -game command line does not exist");
    }else{
      throw exception("Directory passed to -game command line does not exist");
    }
  }else{
    if(boostfs::is_directory(GameStringPath)){
      GameIsZip = false;
    }else{
      if(ext != _T(".zip")){
        throw exception("Only zip files can be passed to -game command line");
      }
      GameIsZip = true;
    }
  }
}
