#include "StdAfx.h"
#include "VC2005Compat.h"

//using namespace std;
VC2005RunTime* VC2005RunTime::Instance = NULL;

void VC05string::Init(const char* s, size_t len){
  length = len;
  capacity = length > 16 ? length : 16;

  char* dest = buff;

  if(length > 16){
    buffptr = dest = (char*)VC2005RunTime::NewOperator(length);
  }

  if(length > 0){
    memcpy(dest, s, length);
  }else{
    buffptr = NULL;
  }
}

void VC05string::SetNewString(const char* newString, size_t newlength){   

  char* dest = buff;

  if(newlength > capacity){
    dest = (char*)VC2005RunTime::NewOperator(newlength);

    if(capacity != 16)VC2005RunTime::DeleteOperator(buffptr);

    capacity = newlength;
    buffptr = dest;
  }else if(newlength <= 16 && capacity > 16){
    //switch to the short string buffer since the string is smaller than 16 characters
    VC2005RunTime::DeleteOperator(buffptr);
    capacity = 16;
  }

  if(newlength > 0){
    memcpy(dest, newString, newlength);
  }else{
    buffptr = NULL;
  }

  length = newlength;
}

VC2005RunTime::VC2005RunTime(){
  vc2005 = GetModuleHandleA("msvcr80.dll");

  vc2005cpp = GetModuleHandleA("MSVCP80.dll");

  NewOp = (newFuncPtr)GetProcAddress(vc2005, "??2@YAPAXI@Z");
  DeleteOp = (deleteFuncPtr)GetProcAddress(vc2005, "??3@YAXPAX@Z");

  if(NewOp == NULL || DeleteOp == NULL){
    throw std::exception("Failed to get vc2005 allocator functions");
  }
}