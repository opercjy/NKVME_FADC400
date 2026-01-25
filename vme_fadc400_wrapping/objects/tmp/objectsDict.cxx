// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME tmpdIobjectsDict
#define R__NO_DEPRECATION

/*******************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define G__DICTIONARY
#include "ROOT/RConfig.hxx"
#include "TClass.h"
#include "TDictAttributeMap.h"
#include "TInterpreter.h"
#include "TROOT.h"
#include "TBuffer.h"
#include "TMemberInspector.h"
#include "TInterpreter.h"
#include "TVirtualMutex.h"
#include "TError.h"

#ifndef G__ROOT
#define G__ROOT
#endif

#include "RtypesImp.h"
#include "TIsAProxy.h"
#include "TFileMergeInfo.h"
#include <algorithm>
#include "TCollectionProxyInfo.h"
/*******************************************************************/

#include "TDataMember.h"

// Header files passed as explicit arguments
#include "include/FadcBD.hh"
#include "include/Pmt.hh"
#include "include/RawChannel.hh"
#include "include/RawData.hh"
#include "include/Run.hh"
#include "include/RunInfo.hh"

// Header files passed via #pragma extra_include

// The generated code does not explicitly qualify STL entities
namespace std {} using namespace std;

namespace ROOT {
   static void *new_FadcBD(void *p = nullptr);
   static void *newArray_FadcBD(Long_t size, void *p);
   static void delete_FadcBD(void *p);
   static void deleteArray_FadcBD(void *p);
   static void destruct_FadcBD(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::FadcBD*)
   {
      ::FadcBD *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::FadcBD >(nullptr);
      static ::ROOT::TGenericClassInfo 
         instance("FadcBD", ::FadcBD::Class_Version(), "FadcBD.hh", 8,
                  typeid(::FadcBD), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::FadcBD::Dictionary, isa_proxy, 4,
                  sizeof(::FadcBD) );
      instance.SetNew(&new_FadcBD);
      instance.SetNewArray(&newArray_FadcBD);
      instance.SetDelete(&delete_FadcBD);
      instance.SetDeleteArray(&deleteArray_FadcBD);
      instance.SetDestructor(&destruct_FadcBD);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::FadcBD*)
   {
      return GenerateInitInstanceLocal(static_cast<::FadcBD*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::FadcBD*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

namespace ROOT {
   static void *new_Pmt(void *p = nullptr);
   static void *newArray_Pmt(Long_t size, void *p);
   static void delete_Pmt(void *p);
   static void deleteArray_Pmt(void *p);
   static void destruct_Pmt(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::Pmt*)
   {
      ::Pmt *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::Pmt >(nullptr);
      static ::ROOT::TGenericClassInfo 
         instance("Pmt", ::Pmt::Class_Version(), "Pmt.hh", 4,
                  typeid(::Pmt), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::Pmt::Dictionary, isa_proxy, 4,
                  sizeof(::Pmt) );
      instance.SetNew(&new_Pmt);
      instance.SetNewArray(&newArray_Pmt);
      instance.SetDelete(&delete_Pmt);
      instance.SetDeleteArray(&deleteArray_Pmt);
      instance.SetDestructor(&destruct_Pmt);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::Pmt*)
   {
      return GenerateInitInstanceLocal(static_cast<::Pmt*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::Pmt*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

namespace ROOT {
   static void *new_RawChannel(void *p = nullptr);
   static void *newArray_RawChannel(Long_t size, void *p);
   static void delete_RawChannel(void *p);
   static void deleteArray_RawChannel(void *p);
   static void destruct_RawChannel(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::RawChannel*)
   {
      ::RawChannel *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::RawChannel >(nullptr);
      static ::ROOT::TGenericClassInfo 
         instance("RawChannel", ::RawChannel::Class_Version(), "RawChannel.hh", 6,
                  typeid(::RawChannel), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::RawChannel::Dictionary, isa_proxy, 4,
                  sizeof(::RawChannel) );
      instance.SetNew(&new_RawChannel);
      instance.SetNewArray(&newArray_RawChannel);
      instance.SetDelete(&delete_RawChannel);
      instance.SetDeleteArray(&deleteArray_RawChannel);
      instance.SetDestructor(&destruct_RawChannel);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::RawChannel*)
   {
      return GenerateInitInstanceLocal(static_cast<::RawChannel*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::RawChannel*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

namespace ROOT {
   static void *new_RawData(void *p = nullptr);
   static void *newArray_RawData(Long_t size, void *p);
   static void delete_RawData(void *p);
   static void deleteArray_RawData(void *p);
   static void destruct_RawData(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::RawData*)
   {
      ::RawData *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::RawData >(nullptr);
      static ::ROOT::TGenericClassInfo 
         instance("RawData", ::RawData::Class_Version(), "RawData.hh", 7,
                  typeid(::RawData), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::RawData::Dictionary, isa_proxy, 4,
                  sizeof(::RawData) );
      instance.SetNew(&new_RawData);
      instance.SetNewArray(&newArray_RawData);
      instance.SetDelete(&delete_RawData);
      instance.SetDeleteArray(&deleteArray_RawData);
      instance.SetDestructor(&destruct_RawData);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::RawData*)
   {
      return GenerateInitInstanceLocal(static_cast<::RawData*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::RawData*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

namespace ROOT {
   static void *new_Run(void *p = nullptr);
   static void *newArray_Run(Long_t size, void *p);
   static void delete_Run(void *p);
   static void deleteArray_Run(void *p);
   static void destruct_Run(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::Run*)
   {
      ::Run *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::Run >(nullptr);
      static ::ROOT::TGenericClassInfo 
         instance("Run", ::Run::Class_Version(), "Run.hh", 6,
                  typeid(::Run), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::Run::Dictionary, isa_proxy, 4,
                  sizeof(::Run) );
      instance.SetNew(&new_Run);
      instance.SetNewArray(&newArray_Run);
      instance.SetDelete(&delete_Run);
      instance.SetDeleteArray(&deleteArray_Run);
      instance.SetDestructor(&destruct_Run);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::Run*)
   {
      return GenerateInitInstanceLocal(static_cast<::Run*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::Run*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

namespace ROOT {
   static void *new_RunInfo(void *p = nullptr);
   static void *newArray_RunInfo(Long_t size, void *p);
   static void delete_RunInfo(void *p);
   static void deleteArray_RunInfo(void *p);
   static void destruct_RunInfo(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::RunInfo*)
   {
      ::RunInfo *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::RunInfo >(nullptr);
      static ::ROOT::TGenericClassInfo 
         instance("RunInfo", ::RunInfo::Class_Version(), "RunInfo.hh", 8,
                  typeid(::RunInfo), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::RunInfo::Dictionary, isa_proxy, 4,
                  sizeof(::RunInfo) );
      instance.SetNew(&new_RunInfo);
      instance.SetNewArray(&newArray_RunInfo);
      instance.SetDelete(&delete_RunInfo);
      instance.SetDeleteArray(&deleteArray_RunInfo);
      instance.SetDestructor(&destruct_RunInfo);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::RunInfo*)
   {
      return GenerateInitInstanceLocal(static_cast<::RunInfo*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::RunInfo*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

//______________________________________________________________________________
atomic_TClass_ptr FadcBD::fgIsA(nullptr);  // static to hold class pointer

//______________________________________________________________________________
const char *FadcBD::Class_Name()
{
   return "FadcBD";
}

//______________________________________________________________________________
const char *FadcBD::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::FadcBD*)nullptr)->GetImplFileName();
}

//______________________________________________________________________________
int FadcBD::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::FadcBD*)nullptr)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *FadcBD::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::FadcBD*)nullptr)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *FadcBD::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::FadcBD*)nullptr)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
atomic_TClass_ptr Pmt::fgIsA(nullptr);  // static to hold class pointer

//______________________________________________________________________________
const char *Pmt::Class_Name()
{
   return "Pmt";
}

//______________________________________________________________________________
const char *Pmt::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Pmt*)nullptr)->GetImplFileName();
}

//______________________________________________________________________________
int Pmt::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Pmt*)nullptr)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *Pmt::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Pmt*)nullptr)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *Pmt::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Pmt*)nullptr)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
atomic_TClass_ptr RawChannel::fgIsA(nullptr);  // static to hold class pointer

//______________________________________________________________________________
const char *RawChannel::Class_Name()
{
   return "RawChannel";
}

//______________________________________________________________________________
const char *RawChannel::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RawChannel*)nullptr)->GetImplFileName();
}

//______________________________________________________________________________
int RawChannel::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RawChannel*)nullptr)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *RawChannel::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RawChannel*)nullptr)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *RawChannel::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RawChannel*)nullptr)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
atomic_TClass_ptr RawData::fgIsA(nullptr);  // static to hold class pointer

//______________________________________________________________________________
const char *RawData::Class_Name()
{
   return "RawData";
}

//______________________________________________________________________________
const char *RawData::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RawData*)nullptr)->GetImplFileName();
}

//______________________________________________________________________________
int RawData::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RawData*)nullptr)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *RawData::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RawData*)nullptr)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *RawData::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RawData*)nullptr)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
atomic_TClass_ptr Run::fgIsA(nullptr);  // static to hold class pointer

//______________________________________________________________________________
const char *Run::Class_Name()
{
   return "Run";
}

//______________________________________________________________________________
const char *Run::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Run*)nullptr)->GetImplFileName();
}

//______________________________________________________________________________
int Run::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Run*)nullptr)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *Run::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Run*)nullptr)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *Run::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Run*)nullptr)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
atomic_TClass_ptr RunInfo::fgIsA(nullptr);  // static to hold class pointer

//______________________________________________________________________________
const char *RunInfo::Class_Name()
{
   return "RunInfo";
}

//______________________________________________________________________________
const char *RunInfo::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RunInfo*)nullptr)->GetImplFileName();
}

//______________________________________________________________________________
int RunInfo::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RunInfo*)nullptr)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *RunInfo::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RunInfo*)nullptr)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *RunInfo::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RunInfo*)nullptr)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
void FadcBD::Streamer(TBuffer &R__b)
{
   // Stream an object of class FadcBD.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(FadcBD::Class(),this);
   } else {
      R__b.WriteClassBuffer(FadcBD::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_FadcBD(void *p) {
      return  p ? new(p) ::FadcBD : new ::FadcBD;
   }
   static void *newArray_FadcBD(Long_t nElements, void *p) {
      return p ? new(p) ::FadcBD[nElements] : new ::FadcBD[nElements];
   }
   // Wrapper around operator delete
   static void delete_FadcBD(void *p) {
      delete (static_cast<::FadcBD*>(p));
   }
   static void deleteArray_FadcBD(void *p) {
      delete [] (static_cast<::FadcBD*>(p));
   }
   static void destruct_FadcBD(void *p) {
      typedef ::FadcBD current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::FadcBD

//______________________________________________________________________________
void Pmt::Streamer(TBuffer &R__b)
{
   // Stream an object of class Pmt.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(Pmt::Class(),this);
   } else {
      R__b.WriteClassBuffer(Pmt::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_Pmt(void *p) {
      return  p ? new(p) ::Pmt : new ::Pmt;
   }
   static void *newArray_Pmt(Long_t nElements, void *p) {
      return p ? new(p) ::Pmt[nElements] : new ::Pmt[nElements];
   }
   // Wrapper around operator delete
   static void delete_Pmt(void *p) {
      delete (static_cast<::Pmt*>(p));
   }
   static void deleteArray_Pmt(void *p) {
      delete [] (static_cast<::Pmt*>(p));
   }
   static void destruct_Pmt(void *p) {
      typedef ::Pmt current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::Pmt

//______________________________________________________________________________
void RawChannel::Streamer(TBuffer &R__b)
{
   // Stream an object of class RawChannel.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(RawChannel::Class(),this);
   } else {
      R__b.WriteClassBuffer(RawChannel::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_RawChannel(void *p) {
      return  p ? new(p) ::RawChannel : new ::RawChannel;
   }
   static void *newArray_RawChannel(Long_t nElements, void *p) {
      return p ? new(p) ::RawChannel[nElements] : new ::RawChannel[nElements];
   }
   // Wrapper around operator delete
   static void delete_RawChannel(void *p) {
      delete (static_cast<::RawChannel*>(p));
   }
   static void deleteArray_RawChannel(void *p) {
      delete [] (static_cast<::RawChannel*>(p));
   }
   static void destruct_RawChannel(void *p) {
      typedef ::RawChannel current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::RawChannel

//______________________________________________________________________________
void RawData::Streamer(TBuffer &R__b)
{
   // Stream an object of class RawData.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(RawData::Class(),this);
   } else {
      R__b.WriteClassBuffer(RawData::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_RawData(void *p) {
      return  p ? new(p) ::RawData : new ::RawData;
   }
   static void *newArray_RawData(Long_t nElements, void *p) {
      return p ? new(p) ::RawData[nElements] : new ::RawData[nElements];
   }
   // Wrapper around operator delete
   static void delete_RawData(void *p) {
      delete (static_cast<::RawData*>(p));
   }
   static void deleteArray_RawData(void *p) {
      delete [] (static_cast<::RawData*>(p));
   }
   static void destruct_RawData(void *p) {
      typedef ::RawData current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::RawData

//______________________________________________________________________________
void Run::Streamer(TBuffer &R__b)
{
   // Stream an object of class Run.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(Run::Class(),this);
   } else {
      R__b.WriteClassBuffer(Run::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_Run(void *p) {
      return  p ? new(p) ::Run : new ::Run;
   }
   static void *newArray_Run(Long_t nElements, void *p) {
      return p ? new(p) ::Run[nElements] : new ::Run[nElements];
   }
   // Wrapper around operator delete
   static void delete_Run(void *p) {
      delete (static_cast<::Run*>(p));
   }
   static void deleteArray_Run(void *p) {
      delete [] (static_cast<::Run*>(p));
   }
   static void destruct_Run(void *p) {
      typedef ::Run current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::Run

//______________________________________________________________________________
void RunInfo::Streamer(TBuffer &R__b)
{
   // Stream an object of class RunInfo.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(RunInfo::Class(),this);
   } else {
      R__b.WriteClassBuffer(RunInfo::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_RunInfo(void *p) {
      return  p ? new(p) ::RunInfo : new ::RunInfo;
   }
   static void *newArray_RunInfo(Long_t nElements, void *p) {
      return p ? new(p) ::RunInfo[nElements] : new ::RunInfo[nElements];
   }
   // Wrapper around operator delete
   static void delete_RunInfo(void *p) {
      delete (static_cast<::RunInfo*>(p));
   }
   static void deleteArray_RunInfo(void *p) {
      delete [] (static_cast<::RunInfo*>(p));
   }
   static void destruct_RunInfo(void *p) {
      typedef ::RunInfo current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::RunInfo

namespace ROOT {
   static TClass *vectorlEunsignedsPshortgR_Dictionary();
   static void vectorlEunsignedsPshortgR_TClassManip(TClass*);
   static void *new_vectorlEunsignedsPshortgR(void *p = nullptr);
   static void *newArray_vectorlEunsignedsPshortgR(Long_t size, void *p);
   static void delete_vectorlEunsignedsPshortgR(void *p);
   static void deleteArray_vectorlEunsignedsPshortgR(void *p);
   static void destruct_vectorlEunsignedsPshortgR(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const vector<unsigned short>*)
   {
      vector<unsigned short> *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(vector<unsigned short>));
      static ::ROOT::TGenericClassInfo 
         instance("vector<unsigned short>", -2, "vector", 389,
                  typeid(vector<unsigned short>), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &vectorlEunsignedsPshortgR_Dictionary, isa_proxy, 4,
                  sizeof(vector<unsigned short>) );
      instance.SetNew(&new_vectorlEunsignedsPshortgR);
      instance.SetNewArray(&newArray_vectorlEunsignedsPshortgR);
      instance.SetDelete(&delete_vectorlEunsignedsPshortgR);
      instance.SetDeleteArray(&deleteArray_vectorlEunsignedsPshortgR);
      instance.SetDestructor(&destruct_vectorlEunsignedsPshortgR);
      instance.AdoptCollectionProxyInfo(TCollectionProxyInfo::Generate(TCollectionProxyInfo::Pushback< vector<unsigned short> >()));

      instance.AdoptAlternate(::ROOT::AddClassAlternate("vector<unsigned short>","std::vector<unsigned short, std::allocator<unsigned short> >"));
      return &instance;
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const vector<unsigned short>*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *vectorlEunsignedsPshortgR_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const vector<unsigned short>*>(nullptr))->GetClass();
      vectorlEunsignedsPshortgR_TClassManip(theClass);
   return theClass;
   }

   static void vectorlEunsignedsPshortgR_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   // Wrappers around operator new
   static void *new_vectorlEunsignedsPshortgR(void *p) {
      return  p ? ::new(static_cast<::ROOT::Internal::TOperatorNewHelper*>(p)) vector<unsigned short> : new vector<unsigned short>;
   }
   static void *newArray_vectorlEunsignedsPshortgR(Long_t nElements, void *p) {
      return p ? ::new(static_cast<::ROOT::Internal::TOperatorNewHelper*>(p)) vector<unsigned short>[nElements] : new vector<unsigned short>[nElements];
   }
   // Wrapper around operator delete
   static void delete_vectorlEunsignedsPshortgR(void *p) {
      delete (static_cast<vector<unsigned short>*>(p));
   }
   static void deleteArray_vectorlEunsignedsPshortgR(void *p) {
      delete [] (static_cast<vector<unsigned short>*>(p));
   }
   static void destruct_vectorlEunsignedsPshortgR(void *p) {
      typedef vector<unsigned short> current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class vector<unsigned short>

namespace ROOT {
   // Registration Schema evolution read functions
   int RecordReadRules_objectsDict() {
      return 0;
   }
   static int _R__UNIQUE_DICT_(ReadRules_objectsDict) = RecordReadRules_objectsDict();R__UseDummy(_R__UNIQUE_DICT_(ReadRules_objectsDict));
} // namespace ROOT
namespace {
  void TriggerDictionaryInitialization_objectsDict_Impl() {
    static const char* headers[] = {
"include/FadcBD.hh",
"include/Pmt.hh",
"include/RawChannel.hh",
"include/RawData.hh",
"include/Run.hh",
"include/RunInfo.hh",
nullptr
    };
    static const char* includePaths[] = {
"/usr/local/include/",
"/home/cpnrdaq2/FADC400_CODE/vme_fadc400_wrapping/objects/",
nullptr
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "objectsDict dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_AutoLoading_Map;
namespace std{template <typename _Tp> class __attribute__((annotate("$clingAutoload$bits/allocator.h")))  __attribute__((annotate("$clingAutoload$string")))  allocator;
}
class __attribute__((annotate("$clingAutoload$include/FadcBD.hh")))  FadcBD;
class __attribute__((annotate("$clingAutoload$include/Pmt.hh")))  Pmt;
class __attribute__((annotate("$clingAutoload$include/RawChannel.hh")))  RawChannel;
class __attribute__((annotate("$clingAutoload$include/RawData.hh")))  RawData;
class __attribute__((annotate("$clingAutoload$include/Run.hh")))  Run;
class __attribute__((annotate("$clingAutoload$include/RunInfo.hh")))  RunInfo;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "objectsDict dictionary payload"


#define _BACKWARD_BACKWARD_WARNING_H
// Inline headers
#include "include/FadcBD.hh"
#include "include/Pmt.hh"
#include "include/RawChannel.hh"
#include "include/RawData.hh"
#include "include/Run.hh"
#include "include/RunInfo.hh"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[] = {
"FadcBD", payloadCode, "@",
"Pmt", payloadCode, "@",
"RawChannel", payloadCode, "@",
"RawData", payloadCode, "@",
"Run", payloadCode, "@",
"RunInfo", payloadCode, "@",
nullptr
};
    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("objectsDict",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_objectsDict_Impl, {}, classesHeaders, /*hasCxxModule*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_objectsDict_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_objectsDict() {
  TriggerDictionaryInitialization_objectsDict_Impl();
}
