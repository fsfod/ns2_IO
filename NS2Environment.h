#pragma once
#include "stdafx.h"


class NS2Environment{
public:
  NS2Environment();
  ~NS2Environment();

  bool HasValidGameArg();

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

