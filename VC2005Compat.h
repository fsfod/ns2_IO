#pragma once

#include "stdafx.h"

class VC2005RunTime{

  typedef void* (__cdecl *newFuncPtr)(size_t size);
  typedef void (__cdecl *deleteFuncPtr)(void* mem);

public:
  VC2005RunTime();

  static VC2005RunTime* VC2005RunTime::GetInstance(){

    if(Instance == NULL){
      Instance = ::new VC2005RunTime();
    }
   return Instance;
  }

  static void* NewOperator(size_t size){
    return Instance->NewOp(size);
  }

  static void DeleteOperator(void* p){
    return Instance->DeleteOp(p);
  }

private:
  static VC2005RunTime* Instance;

  HMODULE vc2005, vc2005cpp;
  deleteFuncPtr DeleteOp;
  newFuncPtr NewOp;
};


class VC05string{

public:
  VC05string(const VC05string& other){
    Init(other.c_str(), other.size());
  }

  VC05string(const char* s){
    Init(s, strlen(s));
  }

  VC05string(VC05string&& other):length(0), capacity(16), buffptr(NULL){
    *this = std::move(other);
  }

  VC05string() :length(0), capacity(16), buffptr(NULL){
  }

  VC05string(const std::string& other){
    Init(other.c_str(), other.size());
  }

  ~VC05string(){
    if(capacity > 16)VC2005RunTime::DeleteOperator(buffptr);
  }

  std::string ToVC10String() const{
    return std::string(c_str(), length);
  }

  const std::string& GetTempVC10Ptr() const{
    return reinterpret_cast<const std::string&>(*(std::string*)&buffptr);
  }

  VC05string& VC05string::operator =(const std::string& other){
    SetNewString(other.c_str(), other.size());

    return *this; 
  }

  VC05string& VC05string::operator =(const VC05string& other){ 
    
    if(this != &other){
      SetNewString(other.c_str(), other.size());
    }

    return *this; 
  }

  VC05string& VC05string::operator =(VC05string&& other){ 

    if(this != &other){
      if(other.size() > 16){
        if(capacity > 16)VC2005RunTime::DeleteOperator(buffptr);

        buffptr = other.buffptr;
      }else{
        SetNewString(other.c_str(), other.size());
      }

      other.buffptr = NULL;
      other.capacity = 16;
      other.length = 0;
    }

    return *this; 
  }

  VC05string& VC05string::operator =(const char* newString){
    SetNewString(newString, strlen(newString));

    return *this; 
  }

  const char* c_str() const{
    return capacity > 16 ? buffptr : buff;
  }
  
  int size() const{
    return length;
  }

private:
  void Init(const char* s, size_t len);
  void SetNewString(const char* newString, size_t newlength);

  int Allocator;
  union{
    char* buffptr;
    char buff[16];
  };
  int length;
  int capacity;
};

template  <typename T> class VC05Vector{

public:

  typedef T&  reference;
  typedef T&&  rvalue_reference;
  typedef T*  pointer;

  size_t size() const{
    return (Last-First);
  }

  size_t capacity() const{
    return (End-First);
  }

  void CheckSpace(){
    if(size()+1 > capacity()){
      reallocate(size()*2);
    }
  }

  void reallocate(size_t newsize){
    _ASSERT(newsize != 0);

    int CurrentCount = size();

    pointer NewArray = reinterpret_cast<pointer>(VC2005RunTime::NewOperator(newsize));
    memcpy(NewArray, First, CurrentCount*sizeof(T));
    VC2005RunTime::DeleteOperator(First);

    First = NewArray;
    End = NewArray+newsize;
    Last = NewArray+CurrentCount;
  }

  void push_back(rvalue_reference value){
    CheckSpace();
   // init the element with placement new
    new (&++Last)T(value);
  }

  void push_back(reference value){
    CheckSpace();
    //init the element with placement new
    new (&++Last)T(value);
  }

  T& operator [](int i){
    return First[i];
  }

  T pop_back(){
    if(size() == 0){
      throw std::runtime_error("VC05Vector is empty");
    }

    return *Last--;
  }

  void push_front(reference value){
    if(size() == capacity()){
      throw std::runtime_error("cannot resize VC05Vector's");
    }

    if(size() > 1) {
      memmove(First+1, First, size()*sizeof(T));
    }
    
    memcpy(First, &value, sizeof(T));

    Last++;
  }

private:
  int Allocator;//really its just garbage because 2005 lacked empty base class optimizations
  T* First; 
  T* Last;
  T* End;
};

template <class T> class VC2005Allocator;

// specialize for void:
template <> class VC2005Allocator<void>{
public:
  typedef void*       pointer;
  typedef const void* const_pointer;
  // reference to void members are impossible.
  typedef void        value_type;

  template <class U>
  struct rebind
  {
    typedef VC2005Allocator<U> other;
  };
};

template <class T> class VC2005Allocator{
public:
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;

  template <class U>struct rebind{
    typedef VC2005Allocator<U> other;
  };

  VC2005Allocator() throw(){}

  template <class U>VC2005Allocator(const VC2005Allocator<U>& u) throw(){}
  ~VC2005Allocator() throw(){}

  pointer address(reference r) const{
    return &r;
  }

  const_pointer address(const_reference r) const{
    return &r;
  }

  size_type max_size() const throw(){
    return UINT32_MAX/sizeof(T);
  }

  pointer allocate(size_type n, VC2005Allocator<void>::const_pointer hint = 0){
    return reinterpret_cast<pointer>(VC2005RunTime::NewOperator(n * sizeof(T)));
  }

  void deallocate(pointer p, size_type n){
    VC2005RunTime::operator delete(p);
  }

  void construct(pointer p, const_reference val){
    ::new(p) T(val);
  }

  void destroy(pointer p){
    p->~T();
  }
};

template <class T1, class T2>inline bool operator ==(const VC2005Allocator<T1>& a1, const VC2005Allocator<T2>& a2) throw(){
  return true;
}

template <class T1, class T2> inline bool operator !=(const VC2005Allocator<T1>& a1, const VC2005Allocator<T2>& a2) throw(){
  return false;
}