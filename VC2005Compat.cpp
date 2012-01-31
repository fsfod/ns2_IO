#include "StdAfx.h"
#include "VC2005Compat.h"

//using namespace std;
VC2005RunTime* VC2005RunTime::Instance = NULL;

void VC05string::Init(const char* s, size_t len){
  char* dest = buff;

  if(len < 16){
    capacity = 15;
  }else{
    capacity = len;
    buffptr = dest = (char*)VC2005RunTime::NewOperator(len+1);
  }

  if(len > 0){
    memcpy(dest, s, len);

    dest[len] = 0;
  }else{
    buffptr = NULL;
  }

  length = len;
}

void VC05string::SetNewString(const char* newString, size_t newlength){   

  char* dest = buff;

  if(newlength > capacity){
    dest = (char*)VC2005RunTime::NewOperator(newlength+1);

    if(HasAllocatedBuffer())VC2005RunTime::DeleteOperator(buffptr);

    capacity = newlength;
    buffptr = dest;
  }else if(newlength < 16 && capacity > 15){
    //switch to the short string buffer since the string is smaller than 16 characters
    VC2005RunTime::DeleteOperator(buffptr);
    capacity = 15;
  }

  if(newlength > 0){
    memcpy(dest, newString, newlength);
    //add a null terminator in case it doesn't have one
    dest[newlength] = 0;
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
  
  ArrayDeleteOp = (deleteFuncPtr)GetProcAddress(vc2005, "??_V@YAXPAX@Z");
  

  if(NewOp == NULL || DeleteOp == NULL){
    throw std::exception("Failed to get vc2005 allocator functions");
  }
}