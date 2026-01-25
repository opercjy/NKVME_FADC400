// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME G__Notice6UVME
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
#include "Notice6UVME.h"

// Header files passed via #pragma extra_include

// The generated code does not explicitly qualify STL entities
namespace std {} using namespace std;

namespace ROOT {
   static void *new_NK6UVME(void *p = nullptr);
   static void *newArray_NK6UVME(Long_t size, void *p);
   static void delete_NK6UVME(void *p);
   static void deleteArray_NK6UVME(void *p);
   static void destruct_NK6UVME(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::NK6UVME*)
   {
      ::NK6UVME *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::NK6UVME >(nullptr);
      static ::ROOT::TGenericClassInfo 
         instance("NK6UVME", ::NK6UVME::Class_Version(), "Notice6UVME.h", 20,
                  typeid(::NK6UVME), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::NK6UVME::Dictionary, isa_proxy, 4,
                  sizeof(::NK6UVME) );
      instance.SetNew(&new_NK6UVME);
      instance.SetNewArray(&newArray_NK6UVME);
      instance.SetDelete(&delete_NK6UVME);
      instance.SetDeleteArray(&deleteArray_NK6UVME);
      instance.SetDestructor(&destruct_NK6UVME);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::NK6UVME*)
   {
      return GenerateInitInstanceLocal(static_cast<::NK6UVME*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::NK6UVME*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

//______________________________________________________________________________
atomic_TClass_ptr NK6UVME::fgIsA(nullptr);  // static to hold class pointer

//______________________________________________________________________________
const char *NK6UVME::Class_Name()
{
   return "NK6UVME";
}

//______________________________________________________________________________
const char *NK6UVME::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::NK6UVME*)nullptr)->GetImplFileName();
}

//______________________________________________________________________________
int NK6UVME::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::NK6UVME*)nullptr)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *NK6UVME::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::NK6UVME*)nullptr)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *NK6UVME::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::NK6UVME*)nullptr)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
void NK6UVME::Streamer(TBuffer &R__b)
{
   // Stream an object of class NK6UVME.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(NK6UVME::Class(),this);
   } else {
      R__b.WriteClassBuffer(NK6UVME::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_NK6UVME(void *p) {
      return  p ? new(p) ::NK6UVME : new ::NK6UVME;
   }
   static void *newArray_NK6UVME(Long_t nElements, void *p) {
      return p ? new(p) ::NK6UVME[nElements] : new ::NK6UVME[nElements];
   }
   // Wrapper around operator delete
   static void delete_NK6UVME(void *p) {
      delete (static_cast<::NK6UVME*>(p));
   }
   static void deleteArray_NK6UVME(void *p) {
      delete [] (static_cast<::NK6UVME*>(p));
   }
   static void destruct_NK6UVME(void *p) {
      typedef ::NK6UVME current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::NK6UVME

namespace ROOT {
   // Registration Schema evolution read functions
   int RecordReadRules_G__Notice6UVME() {
      return 0;
   }
   static int _R__UNIQUE_DICT_(ReadRules_G__Notice6UVME) = RecordReadRules_G__Notice6UVME();R__UseDummy(_R__UNIQUE_DICT_(ReadRules_G__Notice6UVME));
} // namespace ROOT
namespace {
  void TriggerDictionaryInitialization_G__Notice6UVME_Impl() {
    static const char* headers[] = {
"Notice6UVME.h",
nullptr
    };
    static const char* includePaths[] = {
"/home/cpnrdaq2/FADC400_CODE/notice_vme_nfadc400/include",
"/usr/local/include/",
"/home/cpnrdaq2/FADC400_CODE/notice_vme_nfadc400/src/6uvme/",
nullptr
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "G__Notice6UVME dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_AutoLoading_Map;
class __attribute__((annotate("$clingAutoload$Notice6UVME.h")))  NK6UVME;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "G__Notice6UVME dictionary payload"


#define _BACKWARD_BACKWARD_WARNING_H
// Inline headers
#include "Notice6UVME.h"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[] = {
"NK6UVME", payloadCode, "@",
nullptr
};
    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("G__Notice6UVME",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_G__Notice6UVME_Impl, {}, classesHeaders, /*hasCxxModule*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_G__Notice6UVME_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_G__Notice6UVME() {
  TriggerDictionaryInitialization_G__Notice6UVME_Impl();
}
