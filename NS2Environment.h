#pragma once
#include "stdafx.h"

class NS2Environment{
public:
  NS2Environment() : GameIsZip(false), IsDedicatedServer(false){
    FindNSRoot();
    ProcessCommandline();
  }

  ~NS2Environment(){
  }

  bool HasValidGameArg(){
    return !GameStringPath.empty();
  }

public:
  PlatformPath NSRootPath, GameStringPath;
  std::string CommandLine, GameString;

  bool IsDedicatedServer;
  bool GameIsZip;

private:
  void FindNSRoot();
  void ProcessCommandline();
  void ParseGameCommandline(PlatformString& CommandLine);
};

extern NS2Environment NS2Env;

