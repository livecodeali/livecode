/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#ifdef TARGET_SUBPLATFORM_ANDROID
#include <jni.h>
#include <mblandroidjava.h>


void MCJavaGetJObject(MCNameRef p_classname)
{
    jclass cls1 = (*env)->GetObjectClass(env, obj);
    if (cls1 == 0)
        ... /* error */
        cls = (*env)->NewGlobalRef(env, cls1);
}
#endif

MC_DLLEXPORT_DEF MCTypeInfoRef kMCJavaObjectTypeInfo;

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCJavaObjectTypeInfo() { return kMCJavaObjectTypeInfo; }

struct __MCJavaObjectImpl
{
	void *object;
};

typedef struct __MCJavaObject *MCJavaObjectRef;

__MCJavaObjectImpl *MCJavaObjectGet(MCJavaObjectRef p_obj)
{
	return (__MCJavaObjectImpl*)MCValueGetExtraBytesPtr(p_obj);
}

bool MCJavaObjectCreate(const __MCJavaObjectImpl &p_obj, MCJavaObjectRef &r_object)
{
	bool t_success;
	t_success = true;
	
	MCJavaObjectRef t_obj;
	t_obj = nil;
	
	t_success = MCValueCreateCustom(kMCJavaObjectTypeInfo, sizeof(__MCJavaObjectImpl), t_obj);
	
	if (t_success)
	{
		*MCJavaObjectGet(t_obj) = p_obj;
		t_success = MCValueInterAndRelease(t_obj, r_object);
        if (!t_success)
            MCValueRelease(t_obj);
	}
	return t_success;
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

#ifdef TARGET_SUBPLATFORM_ANDROID
static void MCJavaStringFromJString(jobject p_string, MCStringRef& r_string)
{
    JNIEnv *t_env = MCJavaGetThreadEnv();
    const char *nativeString = t_env->GetStringUTFChars(p_string, 0);
    
    MCStringCreateWithCString(nativeString, &r_string);
    
    t_env->ReleaseStringUTFChars(p_string, nativeString);
}
#endif

static bool __MCJavaObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	MCJavaObjectRef t_obj = static_cast<MCJavaObjectRef>(p_value);
    
    __MCJavaObjectImpl *t_impl;
    t_impl = MCJavaObjectGet(t_obj);
    
#ifdef TARGET_SUBPLATFORM_ANDROID
    jobject t_object;
    t_object = *(jobject *)t_impl -> object;
    
    MCAutoStringRef t_class_name;
    JNIEnv *t_env = MCJavaGetThreadEnv();
    jclass t_class = t_env->GetObjectClass(t_object);
    
    // First get the class object
    jmethodID mid = t_env->GetMethodID(t_class, "getClass", "()Ljava/lang/Class;");
    jobject clsObj = t_env->CallObjectMethod(t_object, mid);
    
    // Now get the class object's class descriptor
    jclass cls = t_env->GetObjectClass(clsObj);
    
    // Find the getSimpleName() method in the class object
    jmethodID methodId = t_env->GetMethodID(t_class, "getSimpleName", "()Ljava/lang/String;");
    jstring className = (jstring) t_env->CallObjectMethod(cls, methodId);
    
    t_env->DeleteLocalRef(t_class);
    t_env->DeleteLocalRef(cls);
    
    MCJavaStringFromJavaString(className, &t_class_name);
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

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCJavaStringFromJavaString(MCJavaObjectRef p_object, MCStringRef &r_string)
{
    __MCJavaObjectImpl *t_impl;
    t_impl = MCJavaObjectGet(p_object);
#ifdef TARGET_SUBPLATFORM_ANDROID
    jobject t_object;
    t_object = *(jobject *)t_impl -> object;
    MCJavaStringFromJString(t_object, r_string);
#else
    r_string = MCValueRetain(kMCEmptyString);
#endif
}

extern "C" MC_DLLEXPORT_DEF bool MCJavaNewObject(MCStringRef p_class_name, MCListRef p_args, MCJavaObjectRef& r_object)
{
    MCAutoStringRef t_escaped;
    MCStringMutableCopy(p_class_name, &t_escaped);
    
    MCStringFindAndReplaceChar(*t_escaped, '.', '/', kMCStringOptionCompareExact);
    
    MCAutoStringRefAsCString t_class_cstring;
    t_class_cstring . Lock(*t_escaped);

#ifdef TARGET_SUBPLATFORM_ANDROID
    JNIEnv *t_env = MCJavaGetThreadEnv();
    jclass t_class = t_env->FindClass(*t_class_cstring);
    
    jmethodID t_constructor = t_env->GetMethodID(t_class, "<init>", "(V)");
    
    jobject t_object = t_env->NewObject(t_class, t_constructor);

    self -> object = &t_object;
    r_object = self;
    
    return MCJavaObjectCreate(MCJavaObjectImplMake(&t_object), r_object);
#else
    return MCJavaObjectCreate(MCJavaObjectImplMake(nil), r_object);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_java_Initialize(void)
{
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.java.JavaObject"), kMCNullTypeInfo, &kMCJavaObjectCustomValueCallbacks, kMCJavaObjectTypeInfo))
		return false;
    
    return true;
}

extern "C" void com_livecode_java_Finalize(void)
{
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////

