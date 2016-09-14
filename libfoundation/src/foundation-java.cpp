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
#elif TARGET_SUBPLATFORM_ANDROID
#include <jni.h>
extern JNIEnv *MCJavaGetThreadEnv();
extern JNIEnv *MCJavaAttachCurrentThread();
extern void MCJavaDetachCurrentThread();
#endif

#if TARGET_PLATFORM_MACOS_X || TARGET_SUBPLATFORM_ANDROID
#define TARGET_SUPPORTS_JAVA
#endif

#ifdef TARGET_SUPPORTS_JAVA
static JNIEnv *s_env;
static JavaVM *s_jvm;
#endif

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
    
    jint ret = JNI_CreateJavaVM(&s_jvm, (void **)&s_env, &vm_args);
#elif TARGET_SUBPLATFORM_ANDROID
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

#ifdef TARGET_PLATFORM_MACOS_X
void MCJavaAttachCurrentThread()
{
    s_jvm -> AttachCurrentThread((void **)&s_env, nil);
}
#endif

#ifdef TARGET_SUPPORTS_JAVA
void *MCJavaGetEnv()
{
    return s_env;
}

static bool __MCJavaStringFromJString(jstring p_string, MCStringRef& r_string)
{
    const char *nativeString = s_env -> GetStringUTFChars(p_string, 0);
    
    bool t_success;
    t_success = MCStringCreateWithCString(nativeString, r_string);
    
    s_env->ReleaseStringUTFChars(p_string, nativeString);
    return t_success;
}

static bool __MCJavaNumberFromJByte(jbyte p_byte, MCNumberRef& r_number)
{
    return MCNumberCreateWithInteger((integer_t)p_byte, r_number);
}

static bool __MCJavaNumberFromJShort(jshort p_short, MCNumberRef& r_number)
{
    return MCNumberCreateWithInteger((integer_t)p_short, r_number);
}

static bool __MCJavaNumberFromJLong(jlong p_long, MCNumberRef& r_number)
{
    return MCNumberCreateWithReal((real64_t)p_long, r_number);
}

static bool __MCJavaNumberFromJInt(jint p_int, MCNumberRef& r_number)
{
    return MCNumberCreateWithInteger((integer_t)p_int, r_number);
}

static bool __MCJavaBooleanFromJBoolean(jboolean p_bool, MCBooleanRef& r_bool)
{
    return MCBooleanCreateWithBool((bool)p_bool, r_bool);
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
    void *object;
};

__MCJavaObjectImpl *MCJavaObjectGet(MCJavaObjectRef p_obj)
{
    return (__MCJavaObjectImpl*)MCValueGetExtraBytesPtr(p_obj);
}

static inline __MCJavaObjectImpl MCJavaObjectImplMake(void* p_obj)
{
    __MCJavaObjectImpl t_obj;
    t_obj.object = p_obj;
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
    
    jobject t_target = (jobject)MCJavaObjectGetObject(t_obj);
    
    MCAutoStringRef t_class_name;
    __MCJavaStringFromJString(MCJavaGetJObjectClassName(t_target), &t_class_name);
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

MC_DLLEXPORT_DEF bool MCJavaObjectCreate(void *p_object, MCJavaObjectRef &r_object)
{
    bool t_success;
    t_success = true;
    
    MCJavaObjectRef t_obj;
    t_obj = nil;
    
    t_success = MCValueCreateCustom(kMCJavaObjectTypeInfo, sizeof(__MCJavaObjectImpl), t_obj);
    
    if (t_success)
    {
        *MCJavaObjectGet(t_obj) = MCJavaObjectImplMake(p_object);
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
    __MCJavaObjectImpl *t_impl;
    t_impl = MCJavaObjectGet(p_obj);
    return t_impl -> object;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCJavaClassNameToPathString(MCNameRef p_class_name, MCStringRef& r_string)
{
    MCAutoStringRef t_escaped;
    if (!MCStringMutableCopy(MCNameGetString(p_class_name), &t_escaped))
        return false;
    
    if (!MCStringFindAndReplaceChar(*t_escaped, '.', '/', kMCStringOptionCompareExact))
        return false;
    
    return MCStringCopy(*t_escaped, r_string);
}

enum MCJavaCallType {
    MCJavaCallTypeInstance,
    MCJavaCallTypeStatic,
    MCJavaCallTypeNonVirtual
};

MC_DLLEXPORT_DEF
bool MCJavaCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, MCValueRef& r_return, const MCValueRef *p_args, uindex_t p_arg_count)
{
#ifdef TARGET_SUPPORTS_JAVA
    
    MCJavaAttachCurrentThread();
    
    jmethodID t_method_id = (jmethodID)p_method_id;
    
    bool t_is_instance = p_call_type != MCJavaCallTypeStatic;
    
    jobject t_instance = nil;
    if (t_is_instance)
    {
        // Java object on which to call instance method should always be first argument.
        t_instance = (jobject )MCJavaObjectGetObject(*(MCJavaObjectRef *)p_args[0]);
    }

    jvalue *t_params = nil;
    /*
    convert_params_to_jvalue_array
    */
    // At the moment just look at one jobject param
    MCJavaObjectRef t_param = (t_is_instance ? *(MCJavaObjectRef *)p_args[1] : *(MCJavaObjectRef *)p_args[0]);
    jvalue t_first_param;
    if (t_param != nil)
    {
        t_first_param . l = (jobject)MCJavaObjectGetObject(t_param);
        t_params = &t_first_param;
    }
    
    MCAutoStringRef t_class;
    MCAutoStringRefAsCString t_class_cstring;
    MCJavaClassNameToPathString(p_class_name, &t_class);
    t_class_cstring . Lock(*t_class);
    
    jobject t_result = nil;
    jclass t_target_class = nil;
    switch (p_call_type)
    {
        case MCJavaCallTypeInstance:
        {
            /*
            if (t_params != nil)
                t_result = s_env -> CallObjectMethodA(t_instance, t_method_id, t_params);
            else
             */
                t_result = s_env -> CallObjectMethodA(t_instance, t_method_id, nil);
        }
            break;
        case MCJavaCallTypeStatic:
            t_target_class = s_env -> FindClass(*t_class_cstring);
            
            if (t_params != nil)
                t_result = s_env -> CallStaticObjectMethodA(t_target_class, t_method_id, t_params);
            else
                t_result = s_env -> CallStaticObjectMethod(t_target_class, t_method_id);
            break;
        case MCJavaCallTypeNonVirtual:
            t_target_class = s_env -> FindClass(*t_class_cstring);
            
            if (t_params != nil)
                t_result = s_env -> CallNonvirtualObjectMethodA(t_instance, t_target_class, t_method_id, t_params);
            else
                t_result = s_env -> CallNonvirtualObjectMethod(t_instance, t_target_class, t_method_id);
            break;
    }
    
    return MCJavaObjectCreate(t_result, (MCJavaObjectRef&)r_return);
#else
    return MCJavaObjectCreate(nil, (MCJavaObjectRef&)r_return);
#endif
}

MC_DLLEXPORT_DEF bool MCJavaConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string)
{
#ifdef TARGET_SUPPORTS_JAVA
    jstring t_string;
    t_string = (jstring)MCJavaObjectGetObject(p_object);
   return __MCJavaStringFromJString(t_string, r_string);
#else
    r_string = MCValueRetain(kMCEmptyString);
    return true;
#endif
}

MC_DLLEXPORT_DEF bool MCJavaConvertJByteToNumberRef(MCJavaObjectRef p_object, MCNumberRef &r_number)
{
#ifdef TARGET_SUPPORTS_JAVA
    jbyte t_byte;
    t_byte = *(jbyte *)MCJavaObjectGetObject(p_object);
    return __MCJavaNumberFromJByte(t_byte, r_number);
#else
    r_number = MCValueRetain(kMCZero);
    return true;
#endif
}

MC_DLLEXPORT_DEF bool MCJavaConvertJShortToNumberRef(MCJavaObjectRef p_object, MCNumberRef &r_number)
{
#ifdef TARGET_SUPPORTS_JAVA
    jshort t_short;
    t_short = *(jshort *)MCJavaObjectGetObject(p_object);
    return __MCJavaNumberFromJShort(t_short, r_number);
#else
    r_number = MCValueRetain(kMCZero);
    return true;
#endif
}

MC_DLLEXPORT_DEF bool MCJavaConvertJIntToNumberRef(MCJavaObjectRef p_object, MCNumberRef &r_number)
{
#ifdef TARGET_SUPPORTS_JAVA
    jint t_int;
    t_int = *(jint *)MCJavaObjectGetObject(p_object);
    return __MCJavaNumberFromJInt(t_int, r_number);
#else
    r_number = MCValueRetain(kMCZero);
    return true;
#endif
}

MC_DLLEXPORT_DEF bool MCJavaConvertJLongToNumberRef(MCJavaObjectRef p_object, MCNumberRef &r_number)
{
#ifdef TARGET_SUPPORTS_JAVA
    jlong t_long;
    t_long = *(jlong *)MCJavaObjectGetObject(p_object);
    return __MCJavaNumberFromJLong(t_long, r_number);
#else
    r_number = MCValueRetain(kMCZero);
    return true;
#endif
}

MC_DLLEXPORT_DEF bool MCJavaConvertJBooleanToBooleanRef(MCJavaObjectRef p_object, MCBooleanRef &r_bool)
{
#ifdef TARGET_SUPPORTS_JAVA
    jboolean t_bool;
    t_bool = *(jboolean *)MCJavaObjectGetObject(p_object);
    return __MCJavaBooleanFromJBoolean(t_bool, r_bool);
#else
    r_number = MCValueRetain(kMCFalse);
    return true;
#endif
}

MC_DLLEXPORT_DEF bool MCJavaCallConstructor(MCNameRef p_class_name, MCListRef p_args, MCJavaObjectRef& r_object)
{
    MCAutoStringRef t_class_path;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class_path))
        return false;

    MCAutoStringRefAsCString t_class_cstring;
    t_class_cstring . Lock(*t_class_path);
    
#ifdef TARGET_SUPPORTS_JAVA
    jclass t_class = s_env->FindClass(*t_class_cstring);
    
    jmethodID t_constructor = s_env->GetMethodID(t_class, "<init>", "()V");
    
    jobject t_object = s_env->NewObject(t_class, t_constructor);

    MCAutoStringRef t_class_name;
    __MCJavaStringFromJString(MCJavaGetJObjectClassName(t_object), &t_class_name);
    MCLog("created: %@", *t_class_name);
    \
    return MCJavaObjectCreate(s_env -> NewGlobalRef(t_object), r_object);
#else
    return MCJavaObjectCreate(nil, r_object);
#endif
}

MC_DLLEXPORT_DEF void *MCJavaGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_signature)
{
    MCAutoStringRef t_class_path;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class_path))
        return nil;
    
    MCAutoStringRefAsCString t_class_cstring, t_method_cstring, t_signature_cstring;
    t_class_cstring . Lock(*t_class_path);
    t_method_cstring . Lock(p_method_name);
    t_signature_cstring . Lock(p_signature);

#ifdef TARGET_SUPPORTS_JAVA
    jclass t_java_class;
    t_java_class = s_env->FindClass(*t_class_cstring);
    
    jmethodID t_method_id;
    t_method_id = s_env->GetMethodID(t_java_class, *t_method_cstring, *t_signature_cstring);
    
    return (void *)t_method_id;
#else
    return nil;
#endif
}

MC_DLLEXPORT_DEF
MCNameRef MCJavaTypeInfoGetName(MCTypeInfoRef self)
{
    MCAssert(MCTypeInfoIsCustom(self));
    
    return kMCEmptyName;
}
