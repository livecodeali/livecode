/* Copyright (C) 2003-2016 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"

#ifdef TARGET_PLATFORM_MACOS_X
#include <JavaVM/jni.h>
#endif

#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_SUBPLATFORM_ANDROID)
#define TARGET_SUPPORTS_JAVA
#endif

#ifdef TARGET_SUPPORTS_JAVA
static JNIEnv *s_env;
static JavaVM *s_jvm;

bool __MCJavaInitialize()
{
#ifdef TARGET_PLATFORM_MACOS_X
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_6;
    JNI_GetDefaultJavaVMInitArgs(&vm_args);

    JavaVMOption* options = new JavaVMOption[1];
    options[0].optionString = "-Djava.class.path=/usr/lib/java";
    
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;
    
    JNI_CreateJavaVM(&s_jvm, (void **)&s_env, &vm_args);
#else
    extern JNIEnv *MCJavaGetThreadEnv();
    s_env = MCJavaGetThreadEnv();
#endif
    return true;
}

void __MCJavaFinalize()
{
#ifdef TARGET_PLATFORM_MACOS_X
    s_jvm->DestroyJavaVM();
#endif
}

void *MCJavaGetEnv()
{
    return s_env;
}

static void MCJavaStringFromJString(jstring p_string, MCStringRef& r_string)
{
    const char *nativeString = s_env -> GetStringUTFChars(p_string, 0);
    
    MCStringCreateWithCString(nativeString, r_string);
    
    s_env->ReleaseStringUTFChars(p_string, nativeString);
}

static jstring MCJavaGetJObjectClassName(jobject p_obj)
{
    jclass t_class = s_env->GetObjectClass(p_obj);
    
    jclass javaClassClass = s_env->FindClass("java/lang/Class");
    jmethodID javaClassNameMethod = s_env->GetMethodID(javaClassClass, "getName", "()Ljava/lang/String;");
    
    jstring className = (jstring)s_env->CallObjectMethod(t_class, javaClassNameMethod);
    
    return className;
}

#endif

typedef struct __MCJavaObject *MCJavaObjectRef;

MC_DLLEXPORT_DEF MCTypeInfoRef kMCJavaObjectTypeInfo;

MC_DLLEXPORT_DEF MCTypeInfoRef MCJavaGetObjectTypeInfo() { return kMCJavaObjectTypeInfo; }

struct __MCJavaObjectImpl
{
    MCNameRef class_name;
    void *object;
};

__MCJavaObjectImpl *MCJavaObjectGet(MCJavaObjectRef p_obj)
{
    return (__MCJavaObjectImpl*)MCValueGetExtraBytesPtr(p_obj);
}

static inline __MCJavaObjectImpl MCJavaObjectImplMake(MCNameRef p_class, void* p_obj)
{
    __MCJavaObjectImpl t_obj;
    t_obj.object = p_obj;
    t_obj.class_name = MCValueRetain(p_class);
    return t_obj;
}

static void __MCJavaObjectDestroy(MCValueRef p_value)
{
    // no-op
}

static bool __MCJavaObjectCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
    if (p_release)
        r_copy = p_value;
    else
        r_copy = MCValueRetain(p_value);
    return true;
}

static bool __MCJavaObjectEqual(MCValueRef p_left, MCValueRef p_right)
{
    if (p_left == p_right)
        return true;
    
    return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCJavaObjectImpl)) == 0;
}

static hash_t __MCJavaObjectHash(MCValueRef p_value)
{
    return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCJavaObjectImpl));
}

static bool __MCJavaObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
#ifdef TARGET_SUPPORTS_JAVA
    MCJavaObjectRef t_obj = static_cast<MCJavaObjectRef>(p_value);
    
    jobject t_target = *(jobject *)MCJavaObjectGetObject(t_obj);
    
    MCAutoStringRef t_class_name;
    MCJavaStringFromJString(MCJavaGetJObjectClassName(t_target), &t_class_name);
    return MCStringFormat (r_desc, "<java: %@>", *t_class_name);
#else
    return MCStringFormat (r_desc, "<java: %s>", "not supported");
#endif
}

static MCValueCustomCallbacks kMCJavaObjectCustomValueCallbacks =
{
    true,
    __MCJavaObjectDestroy,
    __MCJavaObjectCopy,
    __MCJavaObjectEqual,
    __MCJavaObjectHash,
    __MCJavaObjectDescribe,
    
    nil,
    nil,
};

MC_DLLEXPORT_DEF bool MCJavaObjectCreate(MCNameRef p_class_name, void *p_object, MCJavaObjectRef &r_object)
{
    bool t_success;
    t_success = true;
    
    MCJavaObjectRef t_obj;
    t_obj = nil;
    
    MCTypeInfoRef t_type_info;
    if (!MCNamedCustomTypeInfoCreate(p_class_name, kMCJavaObjectTypeInfo, &kMCJavaObjectCustomValueCallbacks, t_type_info))
        return false;
    
    t_success = MCValueCreateCustom(t_type_info, sizeof(__MCJavaObjectImpl), t_obj);
    
    if (t_success)
    {
        *MCJavaObjectGet(t_obj) = MCJavaObjectImplMake(p_class_name, p_object);
        r_object = t_obj;
    }

    return t_success;
}

bool MCJavaCreateJavaObjectTypeInfo()
{
    return MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.java.JavaObject"), kMCNullTypeInfo, &kMCJavaObjectCustomValueCallbacks, kMCJavaObjectTypeInfo);
}

MC_DLLEXPORT_DEF void *MCJavaObjectGetObject(const MCJavaObjectRef p_obj)
{
    return p_obj -> object;
}

////////////////////////////////////////////////////////////////////////////////

enum MCJavaCallType {
    MCJavaCallTypeInstance,
    MCJavaCallTypeStatic,
    MCJavaCallTypeNonVirtual
};

MC_DLLEXPORT_DEF
bool MCJavaCallJNI(MCNameRef p_class, void *p_method_id, int p_call_type, MCValueRef& r_return, const MCValueRef *p_args)
{
#ifdef TARGET_SUPPORTS_JAVA
    jmethodID t_method_id = *(jmethodID *)p_method_id;
    
    jobject t_target = *(jobject *)MCJavaObjectGetObject(*(MCJavaObjectRef *)p_args[0]);
    jclass t_target_class = s_env -> GetObjectClass(t_target);
    
    MCAssert(p_args != nil && MCValueGetTypeCode(*p_args) == kMCValueTypeCodeJava);
    
    jobject *t_param = nil;
    if (p_args[1] != nil)
    {
        t_param = (jobject *)MCJavaObjectGetObject(*(MCJavaObjectRef *)p_args[1]);
    }
    
    jobject t_result;
    switch (p_call_type)
    {
        case MCJavaCallTypeInstance:
        case MCJavaCallTypeStatic:
            if (t_param != nil)
                t_result = s_env -> CallObjectMethod(t_target_class, t_method_id, t_param);
            else
                t_result = s_env -> CallObjectMethod(t_target_class, t_method_id);
        case MCJavaCallTypeNonVirtual:
            break;
    }
    
    return MCJavaObjectCreate(p_class, &t_result, (MCJavaObjectRef&)r_return);
#else
    return MCJavaObjectCreate(p_class, nil, (MCJavaObjectRef&)r_return);
#endif
}

MC_DLLEXPORT_DEF void MCJavaStringFromJString(MCJavaObjectRef p_object, MCStringRef &r_string)
{
#ifdef TARGET_SUPPORTS_JAVA
    jstring t_string;
    t_string = *(jstring *)MCJavaObjectGetObject(p_object);
    MCJavaStringFromJString(t_string, r_string);
#else
    r_string = MCValueRetain(kMCEmptyString);
#endif
}

MC_DLLEXPORT_DEF bool MCJavaCallConstructor(MCNameRef p_class_name, MCListRef p_args, MCJavaObjectRef& r_object)
{
    MCAutoStringRef t_escaped;
    MCStringMutableCopy(MCNameGetString(p_class_name), &t_escaped);
    
    MCStringFindAndReplaceChar(*t_escaped, '.', '/', kMCStringOptionCompareExact);
    
    MCAutoStringRefAsCString t_class_cstring;
    t_class_cstring . Lock(*t_escaped);
    
#ifdef TARGET_SUPPORTS_JAVA
    jclass t_class = s_env->FindClass(*t_class_cstring);
    
    jmethodID t_constructor = s_env->GetMethodID(t_class, "<init>", "()V");
    
    jobject t_object = s_env->NewObject(t_class, t_constructor);
    
    MCAutoStringRef t_class_name;
    MCJavaStringFromJString(MCJavaGetJObjectClassName(t_object), &t_class_name);
    MCLog("created: %@", *t_class_name);
    
    return MCJavaObjectCreate(p_class_name, &t_object, r_object);
#else
    return MCJavaObjectCreate(p_class_name, nil, r_object);
#endif
}

MC_DLLEXPORT_DEF void *MCJavaGetMethodId(MCStringRef p_class, MCStringRef p_method_name, MCStringRef p_signature)
{
    MCAutoStringRefAsCString t_class_cstring, t_method_cstring, t_signature_cstring;
    t_class_cstring . Lock(p_class);
    t_method_cstring . Lock(p_method_name);
    t_signature_cstring . Lock(p_signature);
    
    jclass t_java_class;
    t_java_class = s_env->FindClass(*t_class_cstring);
    
    jmethodID t_method_id;
    t_method_id = s_env->GetMethodID(t_java_class, *t_method_cstring, *t_signature_cstring);
    
    return (void *)t_method_id;
}

MC_DLLEXPORT_DEF
bool MCJavaUnmanagedTypeInfoCreate(MCNameRef p_class, MCTypeInfoRef& r_typeinfo)
{
    return false;
}

bool MCJavaTypeInfoCreate(MCNameRef p_class, MCTypeInfoRef& r_typeinfo)
{
    return MCNamedCustomTypeInfoCreate(p_class, kMCJavaObjectTypeInfo, &kMCJavaObjectCustomValueCallbacks, r_typeinfo);
}

MC_DLLEXPORT_DEF
MCNameRef MCJavaTypeInfoGetName(MCTypeInfoRef self)
{
    MCAssert(MCTypeInfoIsCustom(self));
    
    return kMCEmptyName;
}
