// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME G__NoticeNFADC400
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
#include "NoticeNFADC400.h"

// Header files passed via #pragma extra_include

// The generated code does not explicitly qualify STL entities
namespace std {} using namespace std;

namespace ROOT {
   static void *new_NKNFADC400(void *p = nullptr);
   static void *newArray_NKNFADC400(Long_t size, void *p);
   static void delete_NKNFADC400(void *p);
   static void deleteArray_NKNFADC400(void *p);
   static void destruct_NKNFADC400(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::NKNFADC400*)
   {
      ::NKNFADC400 *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::NKNFADC400 >(nullptr);
      static ::ROOT::TGenericClassInfo 
         instance("NKNFADC400", ::NKNFADC400::Class_Version(), "NoticeNFADC400.h", 11,
                  typeid(::NKNFADC400), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::NKNFADC400::Dictionary, isa_proxy, 4,
                  sizeof(::NKNFADC400) );
      instance.SetNew(&new_NKNFADC400);
      instance.SetNewArray(&newArray_NKNFADC400);
      instance.SetDelete(&delete_NKNFADC400);
      instance.SetDeleteArray(&deleteArray_NKNFADC400);
      instance.SetDestructor(&destruct_NKNFADC400);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::NKNFADC400*)
   {
      return GenerateInitInstanceLocal(static_cast<::NKNFADC400*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::NKNFADC400*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

//______________________________________________________________________________
atomic_TClass_ptr NKNFADC400::fgIsA(nullptr);  // static to hold class pointer

//______________________________________________________________________________
const char *NKNFADC400::Class_Name()
{
   return "NKNFADC400";
}

//______________________________________________________________________________
const char *NKNFADC400::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::NKNFADC400*)nullptr)->GetImplFileName();
}

//______________________________________________________________________________
int NKNFADC400::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::NKNFADC400*)nullptr)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *NKNFADC400::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::NKNFADC400*)nullptr)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *NKNFADC400::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::NKNFADC400*)nullptr)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
void NKNFADC400::Streamer(TBuffer &R__b)
{
   // Stream an object of class NKNFADC400.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(NKNFADC400::Class(),this);
   } else {
      R__b.WriteClassBuffer(NKNFADC400::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_NKNFADC400(void *p) {
      return  p ? new(p) ::NKNFADC400 : new ::NKNFADC400;
   }
   static void *newArray_NKNFADC400(Long_t nElements, void *p) {
      return p ? new(p) ::NKNFADC400[nElements] : new ::NKNFADC400[nElements];
   }
   // Wrapper around operator delete
   static void delete_NKNFADC400(void *p) {
      delete (static_cast<::NKNFADC400*>(p));
   }
   static void deleteArray_NKNFADC400(void *p) {
      delete [] (static_cast<::NKNFADC400*>(p));
   }
   static void destruct_NKNFADC400(void *p) {
      typedef ::NKNFADC400 current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::NKNFADC400

namespace ROOT {
   // Registration Schema evolution read functions
   int RecordReadRules_G__NoticeNFADC400() {
      return 0;
   }
   static int _R__UNIQUE_DICT_(ReadRules_G__NoticeNFADC400) = RecordReadRules_G__NoticeNFADC400();R__UseDummy(_R__UNIQUE_DICT_(ReadRules_G__NoticeNFADC400));
} // namespace ROOT
namespace {
  void TriggerDictionaryInitialization_G__NoticeNFADC400_Impl() {
    static const char* headers[] = {
"NoticeNFADC400.h",
nullptr
    };
    static const char* includePaths[] = {
"/home/cpnrdaq2/FADC400_CODE/notice_vme_nfadc400/include",
"/usr/local/include/",
"/home/cpnrdaq2/FADC400_CODE/notice_vme_nfadc400/src/nfadc400/",
nullptr
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "G__NoticeNFADC400 dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_AutoLoading_Map;
class __attribute__((annotate("$clingAutoload$NoticeNFADC400.h")))  NKNFADC400;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "G__NoticeNFADC400 dictionary payload"


#define _BACKWARD_BACKWARD_WARNING_H
// Inline headers
#include "NoticeNFADC400.h"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[] = {
"NKNFADC400", payloadCode, "@",
nullptr
};
    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("G__NoticeNFADC400",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_G__NoticeNFADC400_Impl, {}, classesHeaders, /*hasCxxModule*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_G__NoticeNFADC400_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_G__NoticeNFADC400() {
  TriggerDictionaryInitialization_G__NoticeNFADC400_Impl();
}
