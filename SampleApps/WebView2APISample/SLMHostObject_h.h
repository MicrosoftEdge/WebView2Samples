

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Tue Jan 19 08:44:07 2038
 */
/* Compiler settings for SLMHostObject.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __SLMHostObject_h_h__
#define __SLMHostObject_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __ISLMHostObject_FWD_DEFINED__
#define __ISLMHostObject_FWD_DEFINED__
typedef interface ISLMHostObject ISLMHostObject;

#endif 	/* __ISLMHostObject_FWD_DEFINED__ */


#ifndef __SLMHostObject_FWD_DEFINED__
#define __SLMHostObject_FWD_DEFINED__

#ifdef __cplusplus
typedef class SLMHostObject SLMHostObject;
#else
typedef struct SLMHostObject SLMHostObject;
#endif /* __cplusplus */

#endif 	/* __SLMHostObject_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __SLMHostObjectLibrary_LIBRARY_DEFINED__
#define __SLMHostObjectLibrary_LIBRARY_DEFINED__

/* library SLMHostObjectLibrary */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_SLMHostObjectLibrary;

#ifndef __ISLMHostObject_INTERFACE_DEFINED__
#define __ISLMHostObject_INTERFACE_DEFINED__

/* interface ISLMHostObject */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_ISLMHostObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c6f3e4b2-9d5a-4f7e-8b2c-3d4e5f6a7b8c")
    ISLMHostObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryStatus( 
            /* [retval][out] */ BSTR *status) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetupAsync( 
            /* [in] */ IDispatch *progressCallback,
            /* [in] */ IDispatch *completionCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InferAsync( 
            /* [in] */ BSTR prompt,
            /* [in] */ IDispatch *tokenCallback,
            /* [in] */ IDispatch *completionCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelInference( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSystemPrompt( 
            /* [in] */ BSTR prompt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSystemPrompt( 
            /* [retval][out] */ BSTR *prompt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsOnline( 
            /* [retval][out] */ VARIANT_BOOL *online) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsReady( 
            /* [retval][out] */ VARIANT_BOOL *ready) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModels( 
            /* [retval][out] */ BSTR *modelsJson) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetModel( 
            /* [in] */ BSTR alias,
            /* [retval][out] */ VARIANT_BOOL *success) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentModel( 
            /* [retval][out] */ BSTR *alias) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ISLMHostObjectVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISLMHostObject * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISLMHostObject * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISLMHostObject * This);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, QueryStatus)
        HRESULT ( STDMETHODCALLTYPE *QueryStatus )( 
            ISLMHostObject * This,
            /* [retval][out] */ BSTR *status);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, SetupAsync)
        HRESULT ( STDMETHODCALLTYPE *SetupAsync )( 
            ISLMHostObject * This,
            /* [in] */ IDispatch *progressCallback,
            /* [in] */ IDispatch *completionCallback);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, InferAsync)
        HRESULT ( STDMETHODCALLTYPE *InferAsync )( 
            ISLMHostObject * This,
            /* [in] */ BSTR prompt,
            /* [in] */ IDispatch *tokenCallback,
            /* [in] */ IDispatch *completionCallback);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, CancelInference)
        HRESULT ( STDMETHODCALLTYPE *CancelInference )( 
            ISLMHostObject * This);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, SetSystemPrompt)
        HRESULT ( STDMETHODCALLTYPE *SetSystemPrompt )( 
            ISLMHostObject * This,
            /* [in] */ BSTR prompt);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, GetSystemPrompt)
        HRESULT ( STDMETHODCALLTYPE *GetSystemPrompt )( 
            ISLMHostObject * This,
            /* [retval][out] */ BSTR *prompt);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, IsOnline)
        HRESULT ( STDMETHODCALLTYPE *IsOnline )( 
            ISLMHostObject * This,
            /* [retval][out] */ VARIANT_BOOL *online);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, IsReady)
        HRESULT ( STDMETHODCALLTYPE *IsReady )( 
            ISLMHostObject * This,
            /* [retval][out] */ VARIANT_BOOL *ready);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, GetModels)
        HRESULT ( STDMETHODCALLTYPE *GetModels )( 
            ISLMHostObject * This,
            /* [retval][out] */ BSTR *modelsJson);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, SetModel)
        HRESULT ( STDMETHODCALLTYPE *SetModel )( 
            ISLMHostObject * This,
            /* [in] */ BSTR alias,
            /* [retval][out] */ VARIANT_BOOL *success);
        
        DECLSPEC_XFGVIRT(ISLMHostObject, GetCurrentModel)
        HRESULT ( STDMETHODCALLTYPE *GetCurrentModel )( 
            ISLMHostObject * This,
            /* [retval][out] */ BSTR *alias);
        
        END_INTERFACE
    } ISLMHostObjectVtbl;

    interface ISLMHostObject
    {
        CONST_VTBL struct ISLMHostObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISLMHostObject_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ISLMHostObject_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ISLMHostObject_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ISLMHostObject_QueryStatus(This,status)	\
    ( (This)->lpVtbl -> QueryStatus(This,status) ) 

#define ISLMHostObject_SetupAsync(This,progressCallback,completionCallback)	\
    ( (This)->lpVtbl -> SetupAsync(This,progressCallback,completionCallback) ) 

#define ISLMHostObject_InferAsync(This,prompt,tokenCallback,completionCallback)	\
    ( (This)->lpVtbl -> InferAsync(This,prompt,tokenCallback,completionCallback) ) 

#define ISLMHostObject_CancelInference(This)	\
    ( (This)->lpVtbl -> CancelInference(This) ) 

#define ISLMHostObject_SetSystemPrompt(This,prompt)	\
    ( (This)->lpVtbl -> SetSystemPrompt(This,prompt) ) 

#define ISLMHostObject_GetSystemPrompt(This,prompt)	\
    ( (This)->lpVtbl -> GetSystemPrompt(This,prompt) ) 

#define ISLMHostObject_IsOnline(This,online)	\
    ( (This)->lpVtbl -> IsOnline(This,online) ) 

#define ISLMHostObject_IsReady(This,ready)	\
    ( (This)->lpVtbl -> IsReady(This,ready) ) 

#define ISLMHostObject_GetModels(This,modelsJson)	\
    ( (This)->lpVtbl -> GetModels(This,modelsJson) ) 

#define ISLMHostObject_SetModel(This,alias,success)	\
    ( (This)->lpVtbl -> SetModel(This,alias,success) ) 

#define ISLMHostObject_GetCurrentModel(This,alias)	\
    ( (This)->lpVtbl -> GetCurrentModel(This,alias) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISLMHostObject_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_SLMHostObject;

#ifdef __cplusplus

class DECLSPEC_UUID("d7a4f5c3-ae6b-5a8f-9c3d-4e5f6a7b8c9d")
SLMHostObject;
#endif
#endif /* __SLMHostObjectLibrary_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


