/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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
#include "foundation-java-private.h"

static MCJavaType MCJavaMapTypeCodeSubstring(MCStringRef p_type_code, MCRange p_range)
{
    if (MCStringSubstringIsEqualToCString(p_type_code, p_range, "[",
                                          kMCStringOptionCompareExact))
        return kMCJavaTypeObjectArray;
    
    for (uindex_t i = 0; i < sizeof(type_map) / sizeof(type_map[0]); i++)
    {
        if (MCStringSubstringIsEqualToCString(p_type_code, p_range, type_map[i] . name, kMCStringOptionCompareExact))
        {
            return type_map[i] . type;
        }
    }
    
    return kMCJavaTypeObject;
}

int MCJavaMapTypeCode(MCStringRef p_type_code)
{
    return static_cast<int>(MCJavaMapTypeCodeSubstring(p_type_code,MCRangeMake(0,MCStringGetLength(p_type_code))));
}

static bool __GetExpectedTypeCode(MCTypeInfoRef p_type, MCJavaType& r_code)
{
    if (p_type == kMCSInt8TypeInfo)
        r_code = kMCJavaTypeByte;
    else if (p_type == kMCSInt16TypeInfo)
        r_code = kMCJavaTypeShort;
    else if (p_type == kMCSInt32TypeInfo)
        r_code = kMCJavaTypeInt;
    else if (p_type == kMCSInt64TypeInfo)
        r_code = kMCJavaTypeLong;
    else if (p_type == kMCBoolTypeInfo)
        r_code = kMCJavaTypeBoolean;
    else if (p_type == kMCFloatTypeInfo)
        r_code = kMCJavaTypeFloat;
    else if (p_type == kMCDoubleTypeInfo)
        r_code = kMCJavaTypeDouble;
    else if (p_type == kMCNullTypeInfo)
        r_code = kMCJavaTypeVoid;
    else if (p_type == MCJavaGetObjectTypeInfo())
        r_code = kMCJavaTypeObject;
    else
    {
        MCResolvedTypeInfo t_src, t_target;
        if (!MCTypeInfoResolve(p_type, t_src))
            return false;
        
        if (!MCTypeInfoResolve(MCJavaGetObjectTypeInfo(), t_target))
            return false;
        
        if (!MCResolvedTypeInfoConforms(t_src, t_target))
            return false;
        
        r_code = kMCJavaTypeObject;
    }
    
    return true;
}

static bool __MCTypeInfoConformsToJavaType(MCTypeInfoRef p_type, MCJavaType p_code)
{
    MCJavaType t_code;
    if (!__GetExpectedTypeCode(p_type, t_code))
        return false;
    
    return t_code == p_code;
}

static bool __NextArgument(MCStringRef p_arguments, MCRange& x_range)
{
    if (x_range . offset + x_range . length >= MCStringGetLength(p_arguments))
        return false;
    
    x_range . offset = x_range . offset + x_range . length;
    
    MCRange t_new_range;
    t_new_range . offset = x_range . offset;
    t_new_range . length = 1;
    
    uindex_t t_length = 1;
    
    MCJavaType t_next_type;
    while ((t_next_type = MCJavaMapTypeCodeSubstring(p_arguments, t_new_range)) == kMCJavaTypeObjectArray)
    {
        t_new_range . offset++;
        t_length++;
    }
    
    if (t_next_type == kMCJavaTypeObject)
    {
        if (!MCStringFirstIndexOfChar(p_arguments, ';', x_range . offset, kMCStringOptionCompareExact, t_length))
            return false;
        
        // Consume the ;
        t_length++;
    }
    
    x_range . length = t_length;
    return true;
}

static bool __MCJavaCallNeedsClassInstance(MCJavaCallType p_type)
{
    switch (p_type)
    {
        case MCJavaCallTypeStatic:
        case MCJavaCallTypeConstructor:
        case MCJavaCallTypeInterfaceProxy:
        case MCJavaCallTypeStaticGetter:
        case MCJavaCallTypeStaticSetter:
            return false;
        case MCJavaCallTypeInstance:
        case MCJavaCallTypeNonVirtual:
        case MCJavaCallTypeGetter:
        case MCJavaCallTypeSetter:
            return true;
    }
    
    MCUnreachableReturn(false);
}

static bool __RemoveSurroundingParentheses(MCStringRef p_in, MCStringRef& r_out)
{
    return MCStringCopySubstring(p_in,
                                 MCRangeMakeMinMax(1, MCStringGetLength(p_in) - 1),
                                 r_out);
}

bool MCJavaPrivateCheckSignature(MCTypeInfoRef p_signature, MCStringRef p_args, MCStringRef p_return, int p_call_type)
{
    MCJavaCallType t_call_type = static_cast<MCJavaCallType>(p_call_type);
    if (t_call_type == MCJavaCallTypeInterfaceProxy)
        return true;
    
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    uindex_t t_first_param = 0;
    if (__MCJavaCallNeedsClassInstance(t_call_type))
    {
        t_first_param = 1;
    }
    
    // Remove brackets from arg string
    MCAutoStringRef t_args;
    if (!__RemoveSurroundingParentheses(p_args, &t_args))
        return false;
    
    MCRange t_range = MCRangeMake(0, 0);
    
    // Check the types of the arguments.
    for(uindex_t i = t_first_param; i < t_param_count; i++)
    {
        MCTypeInfoRef t_type;
        t_type = MCHandlerTypeInfoGetParameterType(p_signature, i);
        if (!__NextArgument(*t_args, t_range))
            return false;
        MCJavaType t_jtype;
        t_jtype = MCJavaMapTypeCodeSubstring(*t_args, t_range);
        
        if (!__MCTypeInfoConformsToJavaType(t_type, t_jtype))
            return false;
    }
    
    MCTypeInfoRef t_return_type = MCHandlerTypeInfoGetReturnType(p_signature);
    switch (p_call_type)
    {
        case MCJavaCallTypeConstructor:
        case MCJavaCallTypeInterfaceProxy:
            return __MCTypeInfoConformsToJavaType(t_return_type, kMCJavaTypeObject);
        case MCJavaCallTypeSetter:
        case MCJavaCallTypeStaticSetter:
            return __MCTypeInfoConformsToJavaType(t_return_type, kMCJavaTypeVoid);
        default:
        {
            auto t_return_code = static_cast<MCJavaType>(MCJavaMapTypeCode(p_return));
            return __MCTypeInfoConformsToJavaType(t_return_type, t_return_code);
        }
    }
}

MCTypeInfoRef kMCJavaNativeMethodIdErrorTypeInfo;
MCTypeInfoRef kMCJavaNativeMethodCallErrorTypeInfo;
MCTypeInfoRef kMCJavaBindingStringSignatureErrorTypeInfo;
MCTypeInfoRef kMCJavaCouldNotInitialiseJREErrorTypeInfo;
MCTypeInfoRef kMCJavaJRENotSupportedErrorTypeInfo;

bool MCJavaPrivateErrorsInitialize()
{
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.NativeMethodIdError"), MCNAME("java"), MCSTR("JNI exception thrown when getting native method id"), kMCJavaNativeMethodIdErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.NativeMethodCallError"), MCNAME("java"), MCSTR("JNI exception thrown when calling native method"), kMCJavaNativeMethodCallErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.BindingStringSignatureError"), MCNAME("java"), MCSTR("Java binding string does not match foreign handler signature"), kMCJavaBindingStringSignatureErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.CouldNotInitialiseJREError"), MCNAME("java"), MCSTR("Could not initialise Java Runtime Environment"), kMCJavaCouldNotInitialiseJREErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.JRENotSupported"), MCNAME("java"), MCSTR("Java Runtime Environment no supported with current configuration"), kMCJavaJRENotSupportedErrorTypeInfo))
        return false;

    return true;
}

void MCJavaPrivateErrorsFinalize()
{
    MCValueRelease(kMCJavaNativeMethodIdErrorTypeInfo);
    MCValueRelease(kMCJavaNativeMethodCallErrorTypeInfo);
    MCValueRelease(kMCJavaBindingStringSignatureErrorTypeInfo);
    MCValueRelease(kMCJavaCouldNotInitialiseJREErrorTypeInfo);
    MCValueRelease(kMCJavaJRENotSupportedErrorTypeInfo);
}

bool MCJavaPrivateErrorThrow(MCTypeInfoRef p_error_type)
{
    MCAutoErrorRef t_error;
    if (!MCErrorCreate(p_error_type, nil, &t_error))
        return false;
    
    return MCErrorThrow(*t_error);
}

#ifdef TARGET_SUPPORTS_JAVA

#ifdef TARGET_SUBPLATFORM_ANDROID
extern JNIEnv *MCJavaGetThreadEnv();
extern JNIEnv *MCJavaAttachCurrentThread();
extern void MCJavaDetachCurrentThread();
#endif

static JNIEnv *s_env;
static JavaVM *s_jvm;

#if defined(TARGET_PLATFORM_LINUX) || defined(TARGET_PLATFORM_MACOS_X)

static bool s_weak_link_jvm = false;

extern "C" int initialise_weak_link_jvm_with_path(const char*);
static bool initialise_weak_link_jvm()
{
    if (s_weak_link_jvm)
        return true;
    
    MCAutoStringRef t_path;
#if defined(TARGET_PLATFORM_LINUX)
    // On linux we require the path to libjvm.so to be in LD_LIBRARY_PATH
    // so we can just weak link directly.
    if (!MCStringFormat(&t_path, "libjvm.so"))
        return false;
#else
    const char *t_javahome = getenv("JAVA_HOME");
    
    if (t_javahome == nullptr)
        return false;
    
    if (!MCStringFormat(&t_path, "%s/jre/lib/jli/libjli.dylib", t_javahome))
        return false;
#endif
    
    MCAutoStringRefAsSysString t_jvm_lib;
    if (!t_jvm_lib . Lock(*t_path))
        return false;
    
    if (!initialise_weak_link_jvm_with_path(*t_jvm_lib))
        return false;
    
    s_weak_link_jvm = true;
    return true;
}

#endif

static void init_jvm_args(JavaVMInitArgs *x_args)
{
#if defined(TARGET_PLATFORM_LINUX) || defined(TARGET_PLATFORM_MACOS_X)
    JNI_GetDefaultJavaVMInitArgs(x_args);
#endif
}

static bool create_jvm(JavaVMInitArgs *p_args)
{
#if defined(TARGET_PLATFORM_LINUX) || defined(TARGET_PLATFORM_MACOS_X)
    if (JNI_CreateJavaVM(&s_jvm, (void **)&s_env, p_args) != 0)
        return false;
#endif
    
    return true;
}

bool initialise_jvm()
{
#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
    if (!initialise_weak_link_jvm())
        return false;
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_6;
    init_jvm_args(&vm_args);
    
    JavaVMOption* options = new (nothrow) JavaVMOption[1];
    options[0].optionString = const_cast<char*>("-Djava.class.path=/Users/gheizhwinder/Programming/livecode-private/livecode/build-android-armv6/livecode/out/Debug/classes_livecode_community");
    
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;
    
    return create_jvm(&vm_args);
#endif
    return true;
}

void finalise_jvm()
{
#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
    if (s_jvm != nullptr)
    {
        s_jvm -> DestroyJavaVM();
    }
#endif
}

bool initialise_jni()
{
#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
    if (!MCJavaInitialize(s_env))
        return false;
#endif
    
    return true;
}

void finalise_jni()
{
#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
    MCJavaFinalize(s_env);
#endif
}

void MCJavaDoAttachCurrentThread()
{
#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
    s_jvm -> AttachCurrentThread((void **)&s_env, nullptr);
#else
    s_env = MCJavaAttachCurrentThread();
#endif
}

////////////////////////////////////////////////////////////////////////////////

static jclass s_boolean_class = nil;
static jmethodID s_boolean_constructor = nil;
static jmethodID s_boolean_boolean_value= nil;

static bool init_boolean_class(JNIEnv *env)
{
    jclass t_boolean_class = env->FindClass("java/lang/Boolean");
    s_boolean_class = (jclass)env->NewGlobalRef(t_boolean_class);
    
    if (s_boolean_class == nil)
        return false;
    
    if (s_boolean_constructor == nil)
        s_boolean_constructor = env->GetMethodID(s_boolean_class, "<init>", "(Z)V");
    if (s_boolean_boolean_value == nil)
        s_boolean_boolean_value = env->GetMethodID(s_boolean_class, "booleanValue", "()Z");
    if (s_boolean_constructor == nil || s_boolean_boolean_value == nil)
        return false;
    
    return true;
}

static jclass s_integer_class = nil;
static jmethodID s_integer_constructor = nil;
static jmethodID s_integer_integer_value = nil;

static bool init_integer_class(JNIEnv *env)
{
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_integer_class = env->FindClass("java/lang/Integer");
    s_integer_class = (jclass)env->NewGlobalRef(t_integer_class);
    
    if (s_integer_class == nil)
        return false;
    
    if (s_integer_constructor == nil)
        s_integer_constructor = env->GetMethodID(s_integer_class, "<init>", "(I)V");
    if (s_integer_integer_value == nil)
        s_integer_integer_value = env->GetMethodID(s_integer_class, "intValue", "()I");
    if (s_integer_constructor == nil || s_integer_integer_value == nil)
        return false;
    
    return true;
}

static jclass s_double_class = nil;
static jmethodID s_double_constructor = nil;
static jmethodID s_double_double_value = nil;

static bool init_double_class(JNIEnv *env)
{
    jclass t_double_class = env->FindClass("java/lang/Double");
    s_double_class = (jclass)env->NewGlobalRef(t_double_class);
    
    if (s_double_class == nil)
        return false;
    
    if (s_double_constructor == nil)
        s_double_constructor = env->GetMethodID(s_double_class, "<init>", "(D)V");
    if (s_double_double_value == nil)
        s_double_double_value = env->GetMethodID(s_double_class, "doubleValue", "()D");
    if (s_double_constructor == nil || s_double_double_value == nil)
        return false;
    
    return true;
}

static jclass s_string_class = nil;

static bool init_string_class(JNIEnv *env)
{
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_string_class = env->FindClass("java/lang/String");
    s_string_class = (jclass)env->NewGlobalRef(t_string_class);
    
    if (s_string_class == nil)
        return false;
    
    return true;
}

static jclass s_array_list_class = nil;
static jmethodID s_array_list_constructor = nil;
static jmethodID s_array_list_append = nil;

bool init_arraylist_class(JNIEnv *env)
{
    // PM-2014-02-16: Bug [[ 14489 ]] Use global ref for s_array_list_class to ensure it will be valid next time we use it
    jclass t_array_list_class = env->FindClass("java/util/ArrayList");
    s_array_list_class = (jclass)env->NewGlobalRef(t_array_list_class);
    
    if (s_array_list_class == nil)
        return false;
    
    if (s_array_list_constructor == nil)
        s_array_list_constructor = env->GetMethodID(s_array_list_class, "<init>", "()V");
    if (s_array_list_constructor == nil)
        return false;
    
    if (s_array_list_append == nil)
        s_array_list_append = env->GetMethodID(s_array_list_class, "add", "(Ljava/lang/Object;)Z");
    if (s_array_list_append == nil)
        return false;
    
    return true;
}

static jclass s_hash_map_class = nil;
static jmethodID s_hash_map_constructor = nil;
static jmethodID s_hash_map_put = nil;
static jmethodID s_hash_map_entry_set = nil;

static jclass s_map_entry_class = nil;
static jmethodID s_map_entry_get_key = nil;
static jmethodID s_map_entry_get_value = nil;

static bool init_hashmap_class(JNIEnv *env)
{
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_hash_map_class = env->FindClass("java/util/HashMap");
    s_hash_map_class = (jclass)env->NewGlobalRef(t_hash_map_class);
    
    if (s_hash_map_class == nil)
        return false;
    
    if (s_hash_map_constructor == nil)
        s_hash_map_constructor = env->GetMethodID(s_hash_map_class, "<init>", "()V");
    if (s_hash_map_constructor == nil)
        return false;
    
    if (s_hash_map_put == nil)
        s_hash_map_put = env->GetMethodID(s_hash_map_class, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (s_hash_map_put == nil)
        return false;
    
    if (s_hash_map_entry_set == nil)
        s_hash_map_entry_set = env->GetMethodID(s_hash_map_class, "entrySet", "()Ljava/util/Set;");
    if (s_hash_map_entry_set == nil)
        return false;
    
    
    if (s_map_entry_class == nil)
    {
        // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
        jclass t_map_entry_class = env->FindClass("java/util/Map$Entry");
        s_map_entry_class = (jclass)env->NewGlobalRef(t_map_entry_class);
    }
    if (s_map_entry_class == nil)
        return false;
    
    if (s_map_entry_get_key == nil)
        s_map_entry_get_key = env->GetMethodID(s_map_entry_class, "getKey", "()Ljava/lang/Object;");
    if (s_map_entry_get_key == nil)
        return false;
    
    if (s_map_entry_get_value == nil)
        s_map_entry_get_value = env->GetMethodID(s_map_entry_class, "getValue", "()Ljava/lang/Object;");
    if (s_map_entry_get_value == nil)
        return false;
    
    return true;
}

static jclass s_iterator_class = nil;
static jmethodID s_iterator_has_next = nil;
static jmethodID s_iterator_next = nil;

static bool init_iterator_class(JNIEnv *env)
{
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_iterator_class = env->FindClass("java/util/Iterator");
    s_iterator_class = (jclass)env->NewGlobalRef(t_iterator_class);
    
    if (s_iterator_class == nil)
        return false;
    
    if (s_iterator_has_next == nil)
        s_iterator_has_next = env->GetMethodID(s_iterator_class, "hasNext", "()Z");
    if (s_iterator_has_next == nil)
        return false;
    
    if (s_iterator_next == nil)
        s_iterator_next = env->GetMethodID(s_iterator_class, "next", "()Ljava/lang/Object;");
    if (s_iterator_next == nil)
        return false;
    
    return true;
}

static jclass s_set_class = nil;
static jmethodID s_set_iterator = nil;

static bool init_set_class(JNIEnv *env)
{
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_set_class = env->FindClass("java/util/Set");
    s_set_class = (jclass)env->NewGlobalRef(t_set_class);
    
    if (s_set_class == nil)
        return false;
    
    if (s_set_iterator == nil)
        s_set_iterator = env->GetMethodID(s_set_class, "iterator", "()Ljava/util/Iterator;");
    if (s_set_iterator == nil)
        return false;
    
    return true;
}

static jclass s_object_array_class = nil;
static jclass s_byte_array_class = nil;

static bool init_array_classes(JNIEnv *env)
{
    jclass t_object_array_class = env->FindClass("[Ljava/lang/Object;");
    s_object_array_class = (jclass)env->NewGlobalRef(t_object_array_class);
    if (s_object_array_class == nil)
        return false;
    
    jclass t_byte_array_class = env->FindClass("[B");
    s_byte_array_class = (jclass)env->NewGlobalRef(t_byte_array_class);
    if (s_byte_array_class == nil)
        return false;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

// PM-2015-02-16: [[ Bug 14489 ]] Delete global ref
void free_arraylist_class(JNIEnv *env)
{
    if (s_array_list_class != nil)
    {
        env->DeleteGlobalRef(s_array_list_class);
        s_array_list_class = nil;
    }
}

void free_boolean_class(JNIEnv *env)
{
    if (s_boolean_class != nil)
    {
        env->DeleteGlobalRef(s_boolean_class);
        s_boolean_class = nil;
    }
}

void free_integer_class(JNIEnv *env)
{
    if (s_integer_class != nil)
    {
        env->DeleteGlobalRef(s_integer_class);
        s_integer_class = nil;
    }
}

void free_double_class(JNIEnv *env)
{
    if (s_double_class != nil)
    {
        env->DeleteGlobalRef(s_double_class);
        s_double_class = nil;
    }
}

void free_string_class(JNIEnv *env)
{
    if (s_string_class != nil)
    {
        env->DeleteGlobalRef(s_string_class);
        s_string_class = nil;
    }
}

void free_hashmap_class(JNIEnv *env)
{
    if (s_hash_map_class != nil)
    {
        env->DeleteGlobalRef(s_hash_map_class);
        s_hash_map_class = nil;
    }
}

void free_map_entry_class(JNIEnv *env)
{
    if (s_map_entry_class != nil)
    {
        env->DeleteGlobalRef(s_map_entry_class);
        s_map_entry_class = nil;
    }
}

void free_iterator_class(JNIEnv *env)
{
    if (s_iterator_class != nil)
    {
        env->DeleteGlobalRef(s_iterator_class);
        s_iterator_class = nil;
    }
}

void free_set_class(JNIEnv *env)
{
    if (s_set_class != nil)
    {
        env->DeleteGlobalRef(s_set_class);
        s_set_class = nil;
    }
}

void free_array_classes(JNIEnv *env)
{
    if (s_object_array_class != nil)
    {
        env->DeleteGlobalRef(s_object_array_class);
        s_object_array_class = nil;
    }
    if (s_byte_array_class != nil)
    {
        env->DeleteGlobalRef(s_byte_array_class);
        s_byte_array_class = nil;
    }
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaInitialize(JNIEnv *p_env)
{
    if (!init_boolean_class(p_env))
        return false;
    
    if (!init_integer_class(p_env))
        return false;
    
    if (!init_double_class(p_env))
        return false;
    
    if (!init_string_class(p_env))
        return false;
    
    if (!init_arraylist_class(p_env))
        return false;
    
    if (!init_hashmap_class(p_env))
        return false;
    
    if (!init_iterator_class(p_env))
        return false;
    
    if (!init_set_class(p_env))
        return false;
    
    if (!init_array_classes(p_env))
        return false;
    
    return true;
}

void MCJavaFinalize(JNIEnv *p_env)
{
    free_arraylist_class(p_env);
    free_boolean_class(p_env);
    free_integer_class(p_env);
    free_double_class(p_env);
    free_string_class(p_env);
    free_hashmap_class(p_env);
    free_map_entry_class(p_env);
    free_iterator_class(p_env);
    free_set_class(p_env);
    free_array_classes(p_env);
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaStringFromStringRef(JNIEnv *env, MCStringRef p_string, jstring &r_java_string)
{
    if (p_string == nil)
    {
        r_java_string = nil;
        return true;
    }
    
    bool t_success = true;
    jstring t_java_string = nil;
    
    // SN-2015-04-28: [[ Bug 15151 ]] If the string is native, we don't want to
    //  unnativise it - that's how we end up with a CantBeNative empty string!
    if (MCStringIsNative(p_string))
    {
        unichar_t *t_string;
        uindex_t t_string_length;
        t_string = nil;
        t_success = MCStringConvertToUnicode(p_string, t_string, t_string_length);
        
        if (t_success)
            t_success = nil != (t_java_string = env -> NewString((const jchar*)t_string, t_string_length));
        
        if (t_string != nil)
            MCMemoryDeleteArray(t_string);
    }
    else
        t_success = nil != (t_java_string = env -> NewString((const jchar*)MCStringGetCharPtr(p_string), MCStringGetLength(p_string)));
    
    if (t_success)
        r_java_string = t_java_string;
    
    return t_success;
    
}

//////////

bool MCJavaStringToStringRef(JNIEnv *env, jstring p_java_string, MCStringRef &r_string)
{
    const char *t_string = env -> GetStringUTFChars(p_java_string, 0);
    const byte_t *t_bytes =
        reinterpret_cast<const byte_t *>(t_string);
    
    bool t_success = MCStringCreateWithBytes(t_bytes,
                                             env->GetStringUTFLength(p_java_string),
                                             kMCStringEncodingUTF8,
                                             false,
                                             r_string);
    
    env->ReleaseStringUTFChars(p_java_string, t_string);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaByteArrayFromDataRef(JNIEnv *env, MCDataRef p_data, jbyteArray &r_byte_array)
{
    if (p_data == nil || MCDataGetLength(p_data) == 0)
    {
        r_byte_array = nil;
        return true;
    }
    
    bool t_success = true;
    jbyteArray t_bytes = nil;
    
    t_success = nil != (t_bytes = env -> NewByteArray(MCDataGetLength(p_data)));
    
    if (t_success)
    {
        env -> SetByteArrayRegion(t_bytes, 0, MCDataGetLength(p_data), (const jbyte*)MCDataGetBytePtr(p_data));
    }
    
    if (t_success)
        r_byte_array = t_bytes;
    
    return t_success;
}

//////////

bool MCJavaByteArrayToDataRef(JNIEnv *env, jbyteArray p_byte_array, MCDataRef& r_data)
{
    bool t_success = true;
    
    jbyte *t_bytes = nil;
    uint32_t t_length = 0;
    
    if (p_byte_array != nil)
        t_bytes = env -> GetByteArrayElements(p_byte_array, nil);
    
    if (t_bytes != nil)
    {
        t_length = env -> GetArrayLength(p_byte_array);
        t_success = MCDataCreateWithBytes((const byte_t *)t_bytes, t_length, r_data);
        env -> ReleaseByteArrayElements(p_byte_array, t_bytes, 0);
    }
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaIntegerFromInt(JNIEnv *env, jint p_int, jobject &r_integer)
{
    bool t_success = true;
    
    jobject t_integer = nil;
    t_integer = env->NewObject(s_integer_class, s_integer_constructor, p_int);
    t_success = nil != t_integer;
    
    if (t_success)
        r_integer = t_integer;
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaInitList(JNIEnv *env, jobject &r_list)
{
    bool t_success = true;
    
    jobject t_list = nil;
    t_list = env->NewObject(s_array_list_class, s_array_list_constructor);
    t_success = nil != t_list;
    
    if (t_success)
        r_list = t_list;
    
    return t_success;
}

bool MCJavaFreeList(JNIEnv *env, jobject p_list)
{
    env->DeleteLocalRef(p_list);
    return true;
}

bool MCJavaListAppendObject(JNIEnv *env, jobject p_list, jobject p_object)
{
    // MM-2012-06-01: [[ 10211 ]] List.Add returns a boolean.
    env->CallBooleanMethod(p_list, s_array_list_append, p_object);
    // TODO: check for exceptions
    return true;
}

////////

bool MCJavaListAppendStringRef(JNIEnv *env, jobject p_list, MCStringRef p_string)
{
    bool t_success = true;
    jstring t_jstring = nil;
    
    t_success = MCJavaStringFromStringRef(env, p_string, t_jstring);
    
    if (t_success)
        t_success = MCJavaListAppendObject(env, p_list, t_jstring);
    
    if (t_jstring != nil)
        env->DeleteLocalRef(t_jstring);
    
    return t_success;
}

bool MCJavaListAppendInt(JNIEnv *env, jobject p_list, jint p_int)
{
    bool t_success = true;
    jobject t_integer = nil;
    
    t_success = MCJavaIntegerFromInt(env, p_int, t_integer);
    
    if (t_success)
        t_success = MCJavaListAppendObject(env, p_list, t_integer);
    
    if (t_integer != nil)
        env->DeleteLocalRef(t_integer);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////
// MM-2012-02-22: Added ability to create Java maps


bool MCJavaInitMap(JNIEnv *env, jobject &r_map)
{
    jobject t_map = nil;
    t_map = env->NewObject(s_hash_map_class, s_hash_map_constructor);
    if (nil == t_map)
        return false;
    
    r_map = t_map;
    
    return true;
}

bool MCJavaFreeMap(JNIEnv *env, jobject p_map)
{
    env->DeleteLocalRef(p_map);
    return true;
}

bool MCJavaMapPutObjectToObject(JNIEnv *env, jobject p_map, jobject p_key, jobject p_value)
{
    /*
    jstring t_res = (jstring) env->CallObjectMethod(p_map, s_hash_map_put, p_key, p_value);
    // TODO: check for exceptions
     */
    return true;
}

bool MCJavaMapPutStringToObject(JNIEnv *env, jobject p_map, MCStringRef p_key, jobject p_value)
{
    bool t_success;
    t_success = true;
    
    jstring t_key;
    t_key = nil;
    if (t_success)
        t_success = MCJavaStringFromStringRef(env, p_key, t_key);
    
    if (t_success)
        t_success = MCJavaMapPutObjectToObject(env, p_map, t_key, p_value);
    
    if (t_key != nil)
        env->DeleteLocalRef(t_key);
    
    return t_success;
}

bool MCJavaMapPutStringToString(JNIEnv *env, jobject p_map, MCStringRef p_key, MCStringRef p_value)
{
    bool t_success;
    t_success = true;
    
    jstring t_value;
    t_value = nil;
    if (t_success)
        t_success = MCJavaStringFromStringRef(env, p_value, t_value);
    
    if (t_success)
        t_success = MCJavaMapPutStringToObject(env, p_map, p_key, t_value);
    
    if (t_value != nil)
        env->DeleteLocalRef(t_value);
    
    return t_success;
}

//////////

bool MCJavaMapFromArrayRef(JNIEnv *p_env, MCArrayRef p_value, jobject &r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    jobject t_map;
    t_map = NULL;
    if (t_success)
        t_success = MCJavaInitMap(p_env, t_map);
    
    MCValueRef t_element;
    t_element = NULL;
    
    MCNameRef t_name;
    t_name = NULL;
    
    uintptr_t t_position;
    t_position = 0;
    
    while (t_success && MCArrayIterate(p_value, t_position, t_name, t_element))
    {
        jobject t_value;
        t_value = NULL;
        if (t_success)
            t_success = MCJavaObjectFromValueRef(p_env, t_element, t_value);
        
        if (t_success)
            t_success = MCJavaMapPutStringToObject(p_env, t_map, MCNameGetString(t_name), t_value);
        
        if (t_value != NULL)
            p_env -> DeleteLocalRef(t_value);
    }
    
    if (t_success)
        r_object = t_map;
    else if (t_map != nil)
        MCJavaFreeMap(p_env, t_map);
    
    return t_success;
}

//////////

typedef struct
{
    MCArrayRef array;
} map_to_array_context_t;

static bool s_map_to_array_callback(JNIEnv *p_env, MCNameRef p_key, jobject p_value, void *p_context)
{
    bool t_success = true;
    
    map_to_array_context_t *t_context = (map_to_array_context_t*)p_context;
    
    MCAutoValueRef t_value;
    if (t_success)
        t_success = MCJavaObjectToValueRef(p_env, p_value, &t_value);
    
    if (t_success)
        t_success = MCArrayStoreValue(t_context -> array, false, p_key, *t_value);
    
    return t_success;
}

bool MCJavaIterateMap(JNIEnv *env, jobject p_map, MCJavaMapCallback p_callback, void *p_context)
{
    bool t_success = true;
    jobject t_set = nil, t_iterator = nil;
    // get set of entries from map
    t_success = nil != (t_set = env->CallObjectMethod(p_map, s_hash_map_entry_set));
    
    // get iterator for entry set
    if (t_success)
        t_success = nil != (t_iterator = env->CallObjectMethod(t_set, s_set_iterator));
    
    // iterate over entries
    while (t_success && env->CallBooleanMethod(t_iterator, s_iterator_has_next))
    {
        jobject t_entry = nil;
        jobject t_key = nil, t_value = nil;
        
        t_success = nil != (t_entry = env->CallObjectMethod(t_iterator, s_iterator_next));
        
        // fetch entry key & value
        if (t_success)
            t_success = nil != (t_key = env->CallObjectMethod(t_entry, s_map_entry_get_key));
        if (t_success)
            t_success = nil != (t_value = env->CallObjectMethod(t_entry, s_map_entry_get_value));
        
        // convert key string to stringref
        MCAutoStringRef t_key_string;
        if (t_success)
            t_success = MCJavaStringToStringRef(env, (jstring)t_key, &t_key_string);
        
        // and then to nameref
        MCNewAutoNameRef t_key_name;
        if (t_success)
            t_success = MCNameCreate(*t_key_string, &t_key_name);
        
        // call callback
        if (t_success)
            t_success = p_callback(env, *t_key_name, t_value, p_context);
        
        
        if (t_key != nil)
            env->DeleteLocalRef(t_key);
        if (t_value != nil)
            env->DeleteLocalRef(t_value);
        if (t_entry != nil)
            env->DeleteLocalRef(t_entry);
    }
    
    if (t_set != nil)
        env->DeleteLocalRef(t_set);
    if (t_iterator != nil)
        env->DeleteLocalRef(t_iterator);
    
    return t_success;
}

bool MCJavaMapToArrayRef(JNIEnv *p_env, jobject p_map, MCArrayRef &r_array)
{
    if (!init_string_class(p_env) || !init_hashmap_class(p_env))
        return false;
    
    bool t_success = true;
    
    MCAutoArrayRef t_array;
    t_success = MCArrayCreateMutable(&t_array);
    
    map_to_array_context_t t_context;
    t_context.array = *t_array;
    
    if (t_success)
        t_success = MCJavaIterateMap(p_env, p_map, s_map_to_array_callback, &t_context);
    
    if (t_success)
        r_array = MCValueRetain(*t_array);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaBooleanFromBooleanRef(JNIEnv *p_env, MCBooleanRef p_value, jobject& r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    jobject t_boolean;
    t_boolean = NULL;
    if (t_success)
    {
        t_boolean = p_env -> NewObject(s_boolean_class, s_boolean_constructor, p_value == kMCTrue);
        t_success = t_boolean != NULL;
    }
    
    if (t_success)
        r_object = t_boolean;
    
    return t_success;
}

//////////

bool MCJavaBooleanToBooleanRef(JNIEnv *p_env, jobject p_object, MCBooleanRef& r_value)
{
    if (p_object == NULL)
    {
        r_value = NULL;
        return true;
    }
    
    if (p_env -> CallBooleanMethod(p_object, s_boolean_boolean_value))
        r_value = MCValueRetain(kMCTrue);
    else
        r_value = MCValueRetain(kMCFalse);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaNumberFromNumberRef(JNIEnv *p_env, MCNumberRef p_value, jobject& r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    jobject t_number;
    t_number = NULL;
    if (t_success)
    {
        if (MCNumberIsInteger(p_value))
            t_number = p_env -> NewObject(s_integer_class, s_integer_constructor, MCNumberFetchAsInteger(p_value));
        else if (MCNumberIsInteger(p_value))
            t_number = p_env -> NewObject(s_double_class, s_double_constructor, MCNumberFetchAsReal(p_value));
        t_success = t_number != NULL;
    }
    
    if (t_success)
        r_object = t_number;
    
    return t_success;
}

//////////

bool MCJavaNumberToNumberRef(JNIEnv *p_env, jobject p_object, MCNumberRef& r_value)
{
    if (p_object == NULL)
    {
        r_value = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    MCAutoNumberRef t_value;
    if (t_success)
    {
        if (p_env -> IsInstanceOf(p_object, s_integer_class))
        {
            integer_t t_int;
            t_int = p_env -> CallIntMethod(p_object, s_integer_integer_value);
            t_success = MCNumberCreateWithInteger(t_int, &t_value);
        }
        else if (p_env -> IsInstanceOf(p_object, s_double_class))
        {
            real64_t t_double;
            t_double = p_env -> CallDoubleMethod(p_object, s_double_double_value);
            t_success = MCNumberCreateWithReal(t_double, &t_value);
        }
        else
            t_success = false;
    }
    
    if (t_success)
        r_value = MCValueRetain(*t_value);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaArrayFromArrayRef(JNIEnv *p_env, MCArrayRef p_value, jobjectArray& r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    jclass t_object_class;
    t_object_class = NULL;
    if (t_success)
    {
        t_object_class = p_env -> FindClass("java/lang/Object");
        t_success = t_object_class != NULL;
    }
    
    jobjectArray t_array;
    t_array = NULL;
    if (t_success)
    {
        t_array = p_env -> NewObjectArray(MCArrayGetCount(p_value), t_object_class, NULL);
        t_success = t_array != NULL;
    }
    
    if (t_success)
    {
        for (uint32_t i = 0; i < MCArrayGetCount(p_value) && t_success; i++)
        {
            MCValueRef t_value;
            t_value = NULL;
            if (t_success)
                t_success = MCArrayFetchValueAtIndex(p_value, i + 1, t_value);
            
            jobject t_j_value;
            t_j_value = NULL;
            if (t_success)
                t_success = MCJavaObjectFromValueRef(p_env, t_value, t_j_value);
            
            if (t_success)
                p_env -> SetObjectArrayElement(t_array, i, t_j_value);
            
            if (t_j_value != NULL)
                p_env -> DeleteLocalRef(t_j_value);
        }
    }
    
    if (t_success)
        r_object = t_array;
    
    return t_success;
}

//////////

bool MCJavaArrayToArrayRef(JNIEnv *p_env, jobjectArray p_object, MCArrayRef& r_value)
{
    if (p_object == NULL)
    {
        r_value = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    MCAutoArrayRef t_array;
    if (t_success)
        t_success = MCArrayCreateMutable(&t_array);
    
    if (t_success)
    {
        uint32_t t_size;
        t_size = p_env -> GetArrayLength(p_object);
        
        for (uint32_t i = 0; i < t_size && t_success; i++)
        {
            MCAutoValueRef t_value;
            if (t_success)
            {
                jobject t_object;
                t_object = NULL;
                
                t_object = p_env -> GetObjectArrayElement(p_object, i);
                t_success = MCJavaObjectToValueRef(p_env, t_object, &t_value);
                
                if (t_object != NULL)
                    p_env -> DeleteLocalRef(t_object);
            }
            
            if (t_success)
                t_success = MCArrayStoreValueAtIndex(*t_array, i + 1, *t_value);
        }
    }
    
    if (t_success)
        r_value = MCValueRetain(*t_array);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////
// MM-2015-06-10: We can now communicate with Java using MCValueRefs.
//
// Using @ as the sig type, ValueRefs will be automatically converted to and from jobjects.
//
// The type of the params passed on the Java side will be inferred from the type of the ValueRef.
// For example, if you pass a sequential ArrayRef then we will assume that the corresponding
// param type on the Java side will be Object[].
//
// The return type on the Java side should always be Object.
//
// The following conversions will occur:
//    * MCBooleanRef   -> Boolean
//    * MCNumberRef    -> Integer (if int)
//                     -> Double (if real)
//    * MCNameRef      -> String
//    * MCStringRef    -> String
//    * MCDataRef      -> byte[]
//    * MCArrayRef     -> Object[] (if sequential)
//                     -> Map (otherwise)
//

static bool MCArrayIsSequence(const MCArrayRef p_array)
{
    return false;
}

bool MCJavaObjectFromValueRef(JNIEnv *p_env, MCValueRef p_value, jobject& r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    switch (MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeNull:
            r_object = NULL;
            break;
            
        case kMCValueTypeCodeBoolean:
            t_success = MCJavaBooleanFromBooleanRef(p_env, (MCBooleanRef) p_value, r_object);
            break;
            
        case kMCValueTypeCodeNumber:
            t_success = MCJavaNumberFromNumberRef(p_env, (MCNumberRef) p_value, r_object);
            break;
            
        case kMCValueTypeCodeName:
        {
            jstring t_string;
            t_success = MCJavaStringFromStringRef(p_env, MCNameGetString((MCNameRef) p_value), t_string);
            if (t_success)
                r_object = t_string;
            break;
        }
            
        case kMCValueTypeCodeString:
        {
            jstring t_string;
            t_success = MCJavaStringFromStringRef(p_env, (MCStringRef) p_value, t_string);
            if (t_success)
                r_object = t_string;
            break;
        }
            
        case kMCValueTypeCodeData:
        {
            jbyteArray t_array;
            t_success = MCJavaByteArrayFromDataRef(p_env, (MCDataRef) p_value, t_array);
            if (t_success)
                r_object = t_array;
            break;
        }
            
        case kMCValueTypeCodeArray:
            if (MCArrayIsSequence((MCArrayRef) p_value))
            {
                jobjectArray t_array;
                t_success = MCJavaArrayFromArrayRef(p_env, (MCArrayRef) p_value, t_array);
                if (t_success)
                    r_object = t_array;
            }
            else
                t_success = MCJavaMapFromArrayRef(p_env, (MCArrayRef) p_value, r_object);
            break;
            
        case kMCValueTypeCodeList:
            // TODO
            t_success = false;
            break;
            
        case kMCValueTypeCodeSet:
            // TODO
            t_success = false;
            break;
            
        case kMCValueTypeCodeCustom:
            // TODO
            t_success = false;
            break;
            
        default:
            MCUnreachable();
            break;
    }
    
    return t_success;
}

//////////

bool MCJavaObjectToValueRef(JNIEnv *p_env, jobject p_object, MCValueRef &r_value)
{
    if (p_object == NULL)
    {
        r_value = NULL;
        return true;
    }
    
    if (p_env -> IsInstanceOf(p_object, s_boolean_class))
    {
        MCAutoBooleanRef t_value;
        if (MCJavaBooleanToBooleanRef(p_env, p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_integer_class) || p_env -> IsInstanceOf(p_object, s_double_class))
    {
        MCAutoNumberRef t_value;
        if (MCJavaNumberToNumberRef(p_env, p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_string_class))
    {
        MCAutoStringRef t_value;
        if (MCJavaStringToStringRef(p_env, (jstring) p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_byte_array_class))
    {
        MCAutoDataRef t_value;
        if (MCJavaByteArrayToDataRef(p_env, (jbyteArray) p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_object_array_class))
    {
        MCAutoArrayRef t_value;
        if (MCJavaArrayToArrayRef(p_env, (jobjectArray) p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_hash_map_class))
    {
        MCAutoArrayRef t_value;
        if (MCJavaMapToArrayRef(p_env, p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

static MCJavaType valueref_to_returntype(MCValueRef p_value)
{
    switch (MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeBoolean:
            return kMCJavaTypeBooleanRef;
        case kMCValueTypeCodeNumber:
            return kMCJavaTypeNumberRef;
        case kMCValueTypeCodeString:
            return kMCJavaTypeMCStringRef;
        case kMCValueTypeCodeData:
            return kMCJavaTypeByteArray;
        case kMCValueTypeCodeArray:
            if (MCArrayIsSequence((MCArrayRef) p_value))
                return kMCJavaTypeObjectArray;
            else
                return kMCJavaTypeMap;
            
        case kMCValueTypeCodeNull:
        case kMCValueTypeCodeName:
        case kMCValueTypeCodeList:
        case kMCValueTypeCodeSet:
        case kMCValueTypeCodeCustom:
        default:
            return kMCJavaTypeMCValueRef;
    }
}

static MCJavaType native_sigchar_to_returntype(char p_sigchar)
{
    switch (p_sigchar)
    {
        case 'v':
            return kMCJavaTypeVoid;
        case 's':
            return kMCJavaTypeCString;
        case 'x':
            return kMCJavaTypeMCStringRef;
        case 'd':
            return kMCJavaTypeByteArray;
        case 'i':
            return kMCJavaTypeInt;
        case 'j':
            return kMCJavaTypeLong;
        case 'b':
            return kMCJavaTypeBoolean;
        case 'l':
            return kMCJavaTypeList;
            // MM-2012-02-22: Added ability to create Java maps
        case 'm':
            return kMCJavaTypeMap;
        case 'o':
            return kMCJavaTypeObject;
        case 'f':
            return kMCJavaTypeFloat;
        case 'r':
            return kMCJavaTypeDouble;
        case '@':
            return kMCJavaTypeMCValueRef;
    }
    
    return kMCJavaTypeUnknown;
}

static const char *return_type_to_java_sig(MCJavaType p_type)
{
    switch (p_type)
    {
        case kMCJavaTypeVoid:  // void
            return "V";
        case kMCJavaTypeInt:  // integer (32bit signed)
            return "I";
        case kMCJavaTypeLong:  // long int (64bit signed)
            return "J";
        case kMCJavaTypeBoolean:  // boolean
            return "Z";
        case kMCJavaTypeCString:  // string from char *
        case kMCJavaTypeMCStringRef: // string from MCStringRef
            return "Ljava/lang/String;";
        case kMCJavaTypeByteArray:  // binary data from MCString * as byte[]
            return "[B";
        case kMCJavaTypeList:  // List object
            return "Ljava/util/List;";
            // MM-2012-02-22: Added ability to create Java maps
        case kMCJavaTypeMap:  // Map object
            return "Ljava/util/Map;";
        case kMCJavaTypeObject:  // java Object
            return "Ljava/lang/Object;";
        case kMCJavaTypeFloat:
            return "F";
        case kMCJavaTypeDouble:
            return "D";
        case kMCJavaTypeMCValueRef:
            return "Ljava/lang/Object;";
        case kMCJavaTypeObjectArray:
            return "[Ljava/lang/Object;";
        case kMCJavaTypeBooleanRef:
            return "java/lang/Boolean";
        case kMCJavaTypeNumberRef:
            return "java/lang/Number";
        default:
            break;
    }
    
    return nil;
}
static const char *native_sigchar_to_javasig(char p_sigchar)
{
    return return_type_to_java_sig(native_sigchar_to_returntype(p_sigchar));
}

static bool build_java_signature(const char *p_signature, MCStringRef& r_signature, uint32_t& r_param_count)
{
    uint32_t t_param_count;
    t_param_count = 0;
    
    const char *t_return_type;
    t_return_type = native_sigchar_to_javasig(*p_signature++);
    
    MCAutoStringRef t_param_string;
    if (!MCStringCreateMutable(0, &t_param_string))
        return false;
    
    while(*p_signature != '\0')
    {
        const char *t_javasig = native_sigchar_to_javasig(*p_signature);
        if (!MCStringAppendNativeChars(*t_param_string,
                                       reinterpret_cast<const char_t *>(t_javasig),
                                       strlen(t_javasig)))
            return false;
        t_param_count += 1;
        p_signature += 1;
    }
    
    MCAutoStringRef t_signature;
    if (!MCStringFormat(&t_signature, "(%@)%s", *t_param_string, t_return_type))
        return false;
    
    r_signature = MCValueRetain(*t_signature);
    r_param_count = t_param_count;
    
    return true;
}

void MCJavaMethodParamsFree(JNIEnv *env, MCJavaMethodParams *p_params, bool p_global_refs)
{
    if (p_params != nil)
    {
        MCMemoryDeallocate(p_params->signature);
        
        for (uint32_t i = 0; i < p_params->param_count; i++)
        {
            if (p_params->delete_param[i] && p_params->params[i].l != nil)
            {
                if (p_global_refs)
                    env->DeleteGlobalRef(p_params->params[i].l);
                else
                    env->DeleteLocalRef(p_params->params[i].l);
            }
        }
        
        MCMemoryDeleteArray(p_params->delete_param);
        MCMemoryDeleteArray(p_params->params);
        
        MCMemoryDelete(p_params);
    }
}

////////////////////////////////////////////////////////////////////////////////


bool MCJavaConvertParameters(JNIEnv *env, const char *p_signature, va_list p_args, MCJavaMethodParams *&r_params, bool p_global_refs)
{
    bool t_success = true;
    
    MCJavaMethodParams *t_params = nil;
    
    if (t_success)
        t_success = MCMemoryNew(t_params);
    
    const char *t_return_type;
    
    if (t_success)
        t_success = nil != (t_return_type = native_sigchar_to_javasig(*p_signature));
    
    if (t_success)
    {
        t_params->return_type = native_sigchar_to_returntype(*p_signature++);
    }
    
    MCAutoStringRef t_param_string;
    if (!MCStringCreateMutable(0, &t_param_string))
        return false;
    
    jstring t_java_string;
    const char *t_cstring;
    
    jbyteArray t_byte_array;
    jobject t_java_object;
    
    const char *t_param_sig;
    uint32_t t_index = 0;
    
    while (t_success && *p_signature != '\0')
    {
        MCJavaType t_arg_type;
        t_arg_type = native_sigchar_to_returntype(*p_signature);
        
        // If the sig suggests a value ref has been passed, then determine the type based on the type of the value ref passed.
        // Otherwise just use the type passed in the sig.
        MCJavaType t_param_type;
        MCValueRef t_value_ref;
        t_value_ref = NULL;
        if (t_arg_type == kMCJavaTypeMCValueRef)
        {
            t_value_ref = va_arg(p_args, MCValueRef);
            t_param_type = valueref_to_returntype(t_value_ref);
        }
        else
            t_param_type = t_arg_type;
        
        t_success = MCMemoryResizeArray(t_index + 1, t_params->params, t_params->param_count);
        if (t_success)
            t_success = MCMemoryResizeArray(t_index + 1, t_params->delete_param, t_params->param_count);
        
        if (t_success)
            t_success = nil != (t_param_sig = return_type_to_java_sig(t_param_type));

        if (!MCStringAppendNativeChars(*t_param_string,
                                       reinterpret_cast<const char_t *>(t_param_sig),
                                       strlen(t_param_sig)))
            return false;
        
        if (t_success)
        {
            jvalue t_value;
            bool t_delete = false;
            bool t_object = false;
            switch (t_arg_type)
            {
                case kMCJavaTypeCString:
                {
                    t_cstring = va_arg(p_args, const char *);
                    
                    t_success = MCJavaStringFromStringRef(env, MCSTR(t_cstring), t_java_string);
                    if (t_success)
                        t_value . l = t_java_string;
                    
                    t_delete = true;
                    t_object = true;
                }
                    break;
                case kMCJavaTypeMCStringRef:
                {
                    if (t_success)
                        t_success = MCJavaStringFromStringRef(env, va_arg(p_args, MCStringRef), t_java_string);
                    if (t_success)
                        t_value . l = t_java_string;
                    
                    t_delete = true;
                    t_object = true;
                }
                    break;
                case kMCJavaTypeByteArray:
                {
                    if (t_success)
                        t_success = MCJavaByteArrayFromDataRef(env, va_arg(p_args, MCDataRef), t_byte_array);
                    
                    if (t_success)
                        t_value.l = t_byte_array;
                    
                    t_delete = true;
                    t_object = true;
                }
                    break;
                case kMCJavaTypeInt:
                    t_value . i = va_arg(p_args, int);
                    t_delete = false;
                    break;
                case kMCJavaTypeLong:
                    t_value . j = va_arg(p_args, int64_t);
                    t_delete = false;
                    break;
                case kMCJavaTypeBoolean:
                    t_value . z = va_arg(p_args, int) ? JNI_TRUE : JNI_FALSE;
                    t_delete = false;
                    break;
                case kMCJavaTypeList:
                    t_value . l = va_arg(p_args, jobject);
                    t_delete = false;
                    t_object = true;
                    break;
                    // MM-2012-02-22: Added ability to create Java maps
                case kMCJavaTypeMap:
                    t_value . l = va_arg(p_args, jobject);
                    t_delete = false;
                    t_object = true;
                    break;
                case kMCJavaTypeObject:
                    t_value . l = va_arg(p_args, jobject);
                    t_delete = false;
                    t_object = true;
                    break;
                case kMCJavaTypeFloat:
                    t_value . f = va_arg(p_args, float);
                    t_delete = false;
                    break;
                case kMCJavaTypeDouble:
                    t_value . d = va_arg(p_args, double);
                    t_delete = false;
                    break;
                case kMCJavaTypeMCValueRef:
                {
                    if (t_success)
                        t_success = MCJavaObjectFromValueRef(env, t_value_ref, t_java_object);
                    if (t_success)
                        t_value . l = t_java_object;
                    
                    t_delete = true;
                    t_object = true;
                    
                    break;
                }
                default:
                    MCUnreachableReturn(false);
            }
            if (p_global_refs && t_object)
            {
                t_params->params[t_index].l = env->NewGlobalRef(t_value.l);
                if (t_delete)
                    env->DeleteLocalRef(t_value.l);
                t_params->delete_param[t_index] = true;
            }
            else
            {
                t_params->params[t_index] = t_value;
                t_params->delete_param[t_index] = t_delete;
            }
        }
        
        t_index += 1;
        p_signature += 1;
    }
    
    MCAutoStringRef t_signature;
    if (t_success)
        t_success = MCStringFormat(&t_signature, "(%@)%s", *t_param_string, t_return_type);

    if (t_success)
    {
        r_params = t_params;
        MCValueAssign(t_params->signature, *t_signature);
    }
    else
    {
        MCJavaMethodParamsFree(env, t_params, p_global_refs);
    }
    
    return t_success;
}

void MCJavaColorToComponents(uint32_t p_color, uint16_t &r_red, uint16_t &r_green, uint16_t &r_blue, uint16_t &r_alpha)
{
    uint8_t t_red, t_green, t_blue, t_alpha;
    t_alpha = p_color >> 24;
    t_red = (p_color >> 16) & 0xFF;
    t_green = (p_color >> 8) & 0xFF;
    t_blue = p_color & 0xFF;
    
    r_red = (t_red << 8) | t_red;
    r_green = (t_green << 8) | t_green;
    r_blue = (t_blue << 8) | t_blue;
    r_alpha = (t_alpha << 8) | t_alpha;
}

static bool __MCJavaStringFromJString(jstring p_string, MCStringRef& r_string)
{
    MCJavaDoAttachCurrentThread();
    const char *nativeString = s_env -> GetStringUTFChars(p_string, 0);
    bool t_success = MCStringCreateWithCString(nativeString, r_string);
    s_env->ReleaseStringUTFChars(p_string, nativeString);
    return t_success;
}

static bool __MCJavaStringToJString(MCStringRef p_string, jstring& r_string)
{
    MCJavaDoAttachCurrentThread();

    return MCJavaStringFromStringRef(s_env, p_string, r_string);
}

static bool __MCJavaDataFromJByteArray(jbyteArray p_byte_array, MCDataRef& r_data)
{
    MCJavaDoAttachCurrentThread();
    
    jbyte *t_jbytes = nullptr;
    if (p_byte_array != nullptr)
        t_jbytes = s_env -> GetByteArrayElements(p_byte_array, nullptr);
    
    if (t_jbytes == nullptr)
        return false;

    jsize t_length = s_env -> GetArrayLength(p_byte_array);
    
    MCAssert(t_length >= 0);
    
    if (size_t(t_length) > size_t(UINDEX_MAX))
        return false;
    
    auto t_bytes = reinterpret_cast<const byte_t *>(t_jbytes);
    bool t_success = MCDataCreateWithBytes(t_bytes, t_length, r_data);
    s_env -> ReleaseByteArrayElements(p_byte_array, t_jbytes, 0);

    return t_success;
}

static bool __MCJavaDataToJByteArray(MCDataRef p_data, jbyteArray& r_byte_array)
{
    MCJavaDoAttachCurrentThread();
    
    if (p_data == nullptr || MCDataIsEmpty(p_data))
    {
        r_byte_array = nullptr;
        return true;
    }
    
    uindex_t t_length = MCDataGetLength(p_data);
    jbyteArray t_bytes = s_env -> NewByteArray(t_length);
    
    if (t_bytes == nullptr)
        return false;
    
    auto t_data = reinterpret_cast<const jbyte*>(MCDataGetBytePtr(p_data));
    s_env->SetByteArrayRegion(t_bytes, 0, t_length, t_data);
    
    r_byte_array = t_bytes;
    return true;
}

static bool __MCJavaProperListFromJObjectArray(jobjectArray p_obj_array, MCProperListRef& r_list)
{
    MCJavaDoAttachCurrentThread();
    
    if (p_obj_array == nullptr)
    {
        r_list = MCValueRetain(kMCEmptyProperList);
        return true;
    }
    
    MCAutoProperListRef t_list;
    if (!MCProperListCreateMutable(&t_list))
        return false;
    
    uint32_t t_size = s_env -> GetArrayLength(p_obj_array);
    
    for (uint32_t i = 0; i < t_size; i++)
    {
        MCAutoValueRef t_value;
        
        jobject t_object = s_env -> GetObjectArrayElement(p_obj_array, i);
        if (!MCJavaObjectToValueRef(s_env, t_object, &t_value))
            return false;

        if (!MCProperListPushElementOntoBack(*t_list, *t_value))
            return false;
    }
    
    return MCProperListCopy(*t_list, r_list);
}

static bool __MCJavaProperListToJObjectArray(MCProperListRef p_list, jobjectArray& r_obj_array)
{
    MCJavaDoAttachCurrentThread();
    
    if (MCProperListIsEmpty(p_list))
    {
        r_obj_array = nullptr;
        return true;
    }
    
    jclass t_object_class = s_env -> FindClass("java/lang/Object");
    if (t_object_class == nullptr)
        return false;
    
    jobjectArray t_array =
        s_env -> NewObjectArray(MCProperListGetLength(p_list),
                                t_object_class, nullptr);
    if (t_array == nullptr)
        return false;
    
    for (uint32_t i = 0; i < MCProperListGetLength(p_list); i++)
    {
        MCValueRef t_value = MCProperListFetchElementAtIndex(p_list, i);
        
        jobject t_j_value = nullptr;
        if (!MCJavaObjectFromValueRef(s_env, t_value, t_j_value))
            return false;
        
    
        s_env -> SetObjectArrayElement(t_array, i, t_j_value);
        
        if (t_j_value != nullptr)
            s_env -> DeleteLocalRef(t_j_value);
    }
    

    r_obj_array = t_array;
    
    return true;
}

bool MCJavaObjectCreateGlobalRef(jobject p_object, MCJavaObjectRef &r_object)
{
    MCAssert(p_object != nullptr);
    void *t_obj_ptr = s_env -> NewGlobalRef(p_object);
    return MCJavaObjectCreate(t_obj_ptr, r_object);
}

bool MCJavaObjectCreateNullableGlobalRef(jobject p_object, MCJavaObjectRef &r_object)
{
    if (p_object == nullptr)
    {
        r_object = nullptr;
        return true;
    }
    return MCJavaObjectCreateGlobalRef(p_object, r_object);
}

static jstring MCJavaGetJObjectClassName(jobject p_obj)
{
    jclass t_class = s_env->GetObjectClass(p_obj);
    
    jclass javaClassClass = s_env->FindClass("java/lang/Class");
    
    jmethodID javaClassNameMethod = s_env->GetMethodID(javaClassClass, "getName", "()Ljava/lang/String;");
    
    jstring className = (jstring)s_env->CallObjectMethod(t_class, javaClassNameMethod);
    
    return className;
}

static bool __JavaJNIInstanceMethodResult(jobject p_instance, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_return_type = static_cast<MCJavaType>(p_return_type);
    
    switch (t_return_type)
    {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> CallBooleanMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> CallByteMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> CallCharMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> CallShortMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> CallIntMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> CallLongMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                s_env -> CallFloatMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> CallDoubleMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeObjectArray:
        {
            jobject t_result =
                s_env -> CallObjectMethodA(p_instance, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            s_env -> CallVoidMethodA(p_instance, p_method_id, p_params);
            return true;
            
        default:
            break;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNIStaticMethodResult(jclass p_class, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_return_type = static_cast<MCJavaType>(p_return_type);
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> CallStaticBooleanMethodA(p_class, p_method_id, p_params);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> CallStaticByteMethodA(p_class, p_method_id, p_params);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> CallStaticCharMethodA(p_class, p_method_id, p_params);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> CallStaticShortMethodA(p_class, p_method_id, p_params);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> CallStaticIntMethodA(p_class, p_method_id, p_params);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> CallStaticLongMethodA(p_class, p_method_id, p_params);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                s_env -> CallStaticFloatMethodA(p_class, p_method_id, p_params);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> CallStaticDoubleMethodA(p_class, p_method_id, p_params);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeObjectArray:
        {
            jobject t_result =
                s_env -> CallStaticObjectMethodA(p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            s_env -> CallStaticVoidMethodA(p_class, p_method_id, p_params);
            return true;
            
        default:
            break;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNINonVirtualMethodResult(jobject p_instance, jclass p_class, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_return_type = static_cast<MCJavaType>(p_return_type);
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> CallNonvirtualBooleanMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> CallNonvirtualByteMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> CallNonvirtualCharMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> CallNonvirtualShortMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> CallNonvirtualIntMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> CallNonvirtualLongMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                s_env -> CallNonvirtualFloatMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> CallNonvirtualDoubleMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeObjectArray:
        {
            jobject t_result =
                s_env -> CallNonvirtualObjectMethodA(p_instance, p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            s_env -> CallNonvirtualVoidMethodA(p_instance, p_class, p_method_id, p_params);
            return true;
            
        default:
            break;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNIGetFieldResult(jobject p_instance, jfieldID p_field_id, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_return_type = static_cast<MCJavaType>(p_return_type);
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> GetBooleanField(p_instance, p_field_id);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> GetByteField(p_instance, p_field_id);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> GetCharField(p_instance, p_field_id);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> GetShortField(p_instance, p_field_id);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> GetIntField(p_instance, p_field_id);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> GetLongField(p_instance, p_field_id);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                s_env -> GetFloatField(p_instance, p_field_id);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> GetDoubleField(p_instance, p_field_id);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeObjectArray:
        {
            jobject t_result =
                s_env -> GetObjectField(p_instance, p_field_id);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
        default:
            break;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNIGetStaticFieldResult(jclass p_class, jfieldID p_field_id, int p_field_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_field_type = static_cast<MCJavaType>(p_field_type);
    
    switch (t_field_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> GetStaticBooleanField(p_class, p_field_id);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> GetStaticByteField(p_class, p_field_id);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> GetStaticCharField(p_class, p_field_id);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> GetStaticShortField(p_class, p_field_id);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> GetStaticIntField(p_class, p_field_id);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> GetStaticLongField(p_class, p_field_id);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                   s_env -> GetStaticFloatField(p_class, p_field_id);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> GetStaticDoubleField(p_class, p_field_id);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeObjectArray:
        {
            jobject t_result =
                s_env -> GetStaticObjectField(p_class, p_field_id);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
        default:
            break;
    }
    
    MCUnreachableReturn(false);
}

static void __JavaJNISetFieldResult(jobject p_instance, jfieldID p_field_id, const void *p_param, int p_field_type)
{
    MCJavaDoAttachCurrentThread();
    auto t_field_type = static_cast<MCJavaType>(p_field_type);
    
    switch (t_field_type)
    {
        case kMCJavaTypeBoolean:
        {
            s_env -> SetBooleanField(p_instance,
                                     p_field_id,
                                     *(static_cast<const jboolean *>(p_param)));
            return;
        }
        case kMCJavaTypeByte:
        {
            s_env -> SetByteField(p_instance,
                                  p_field_id,
                                  *(static_cast<const jbyte *>(p_param)));
            return;
        }
        case kMCJavaTypeChar:
        {
            s_env -> SetCharField(p_instance,
                                  p_field_id,
                                  *(static_cast<const jchar *>(p_param)));
            return;
        }
        case kMCJavaTypeShort:
        {
            s_env -> SetShortField(p_instance,
                                   p_field_id,
                                   *(static_cast<const jshort *>(p_param)));
            return;
        }
        case kMCJavaTypeInt:
        {
            s_env -> SetIntField(p_instance,
                                 p_field_id,
                                 *(static_cast<const jint *>(p_param)));
            return;
        }
        case kMCJavaTypeLong:
        {
            s_env -> SetLongField(p_instance,
                                  p_field_id,
                                  *(static_cast<const jlong *>(p_param)));
            return;
        }
        case kMCJavaTypeFloat:
        {
            s_env -> SetFloatField(p_instance,
                                   p_field_id,
                                   *(static_cast<const jfloat *>(p_param)));
            return;
        }
        case kMCJavaTypeDouble:
        {
            s_env -> SetDoubleField(p_instance,
                                    p_field_id,
                                    *(static_cast<const jdouble *>(p_param)));
            return;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeObjectArray:
        {
            jobject t_obj = nullptr;
            if (p_param != nullptr)
            {
                MCJavaObjectRef t_param =
                *(static_cast<const MCJavaObjectRef *>(p_param));
                
                t_obj = static_cast<jobject>(MCJavaObjectGetObject(t_param));
            }
            s_env -> SetObjectField(p_instance,
                                    p_field_id,
                                    t_obj);
        }
        case kMCJavaTypeVoid:
        default:
            break;
    }
    
    MCUnreachable();
}

static void __JavaJNISetStaticFieldResult(jclass p_class, jfieldID p_field_id, const void *p_param, int p_field_type)
{
    MCJavaDoAttachCurrentThread();
    auto t_field_type = static_cast<MCJavaType>(p_field_type);
    
    switch (t_field_type)
    {
        case kMCJavaTypeBoolean:
        {
            s_env -> SetStaticBooleanField(p_class,
                                     p_field_id,
                                     *(static_cast<const jboolean *>(p_param)));
            return;
        }
        case kMCJavaTypeByte:
        {
            s_env -> SetStaticByteField(p_class,
                                  p_field_id,
                                  *(static_cast<const jbyte *>(p_param)));
            return;
        }
        case kMCJavaTypeChar:
        {
            s_env -> SetStaticCharField(p_class,
                                  p_field_id,
                                  *(static_cast<const jchar *>(p_param)));
            return;
        }
        case kMCJavaTypeShort:
        {
            s_env -> SetStaticShortField(p_class,
                                   p_field_id,
                                   *(static_cast<const jshort *>(p_param)));
            return;
        }
        case kMCJavaTypeInt:
        {
            s_env -> SetStaticIntField(p_class,
                                 p_field_id,
                                 *(static_cast<const jint *>(p_param)));
            return;
        }
        case kMCJavaTypeLong:
        {
            s_env -> SetStaticLongField(p_class,
                                  p_field_id,
                                  *(static_cast<const jlong *>(p_param)));
            return;
        }
        case kMCJavaTypeFloat:
        {
            s_env -> SetStaticFloatField(p_class,
                                   p_field_id,
                                   *(static_cast<const jfloat *>(p_param)));
            return;
        }
        case kMCJavaTypeDouble:
        {
            s_env -> SetStaticDoubleField(p_class,
                                    p_field_id,
                                    *(static_cast<const jdouble *>(p_param)));
            return;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeObjectArray:
        {
            jobject t_obj = nullptr;
            if (p_param != nullptr)
            {
                MCJavaObjectRef t_param =
                    *(static_cast<const MCJavaObjectRef *>(p_param));
            
                t_obj = static_cast<jobject>(MCJavaObjectGetObject(t_param));
            }
            s_env -> SetStaticObjectField(p_class,
                                          p_field_id,
                                          t_obj);
            return;
        }
        case kMCJavaTypeVoid:
        default:
            break;
    }
    
    MCUnreachable();
}

static bool __JavaJNIConstructorResult(jclass p_class, jmethodID p_method_id, jvalue *p_params, void *r_result)
{
    MCJavaDoAttachCurrentThread();

    jobject t_result = s_env -> NewObjectA(p_class, p_method_id, p_params);
    
    MCJavaObjectRef t_result_value;
    if (!MCJavaObjectCreateGlobalRef(t_result, t_result_value))
        return false;
    *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;

    return true;
}

static bool __JavaJNIGetParams(void **args, MCTypeInfoRef p_signature, jvalue *&r_params)
{
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    MCAutoArray<jvalue> t_args;
    if (!t_args . New(t_param_count))
        return false;
    
    for (uindex_t i = 0; i < t_param_count; i++)
    {
        MCTypeInfoRef t_type = MCHandlerTypeInfoGetParameterType(p_signature, i);
        
        MCJavaType t_expected;
        if (!__GetExpectedTypeCode(t_type, t_expected))
            return false;
        
        switch (t_expected)
        {
            case kMCJavaTypeObjectArray:
            case kMCJavaTypeObject:
            {
                MCJavaObjectRef t_obj = *(static_cast<MCJavaObjectRef *>(args[i]));
                if (t_obj == nullptr)
                {
                    t_args[i] . l = nullptr;
                }
                else
                {
                    t_args[i] . l =
                        static_cast<jobject>(MCJavaObjectGetObject(t_obj));
                }
                break;
            }
            case kMCJavaTypeBoolean:
                t_args[i].z = *(static_cast<jboolean *>(args[i]));
                break;
            case kMCJavaTypeByte:
                t_args[i].b = *(static_cast<jbyte *>(args[i]));
                break;
            case kMCJavaTypeChar:
                t_args[i].c = *(static_cast<jchar *>(args[i]));
                break;
            case kMCJavaTypeShort:
                t_args[i].s = *(static_cast<jshort *>(args[i]));
                break;
            case kMCJavaTypeInt:
                t_args[i].i = *(static_cast<jint *>(args[i]));
                break;
            case kMCJavaTypeLong:
                t_args[i].j = *(static_cast<jlong *>(args[i]));
                break;
            case kMCJavaTypeFloat:
                t_args[i].f = *(static_cast<jfloat *>(args[i]));
                break;
            case kMCJavaTypeDouble:
                t_args[i].d = *(static_cast<jdouble *>(args[i]));
                break;
            default:
                MCUnreachableReturn(false);
        }
    }
    
    t_args . Take(r_params, t_param_count);
    return true;
}

static bool MCJavaClassNameToPathString(MCNameRef p_class_name, MCStringRef& r_string)
{
    MCAutoStringRef t_escaped;
    if (!MCStringMutableCopy(MCNameGetString(p_class_name), &t_escaped))
        return false;
    
    if (!MCStringFindAndReplaceChar(*t_escaped, '.', '/', kMCStringOptionCompareExact))
        return false;
    
    return MCStringCopy(*t_escaped, r_string);
}

static jclass MCJavaPrivateFindClass(MCNameRef p_class_name)
{
    if (MCStringBeginsWith(MCNameGetString(p_class_name),
                           MCSTR("com.runrev.android"),
                           kMCStringOptionCompareExact))
    {
#if defined(TARGET_SUBPLATFORM_ANDROID)
        jstring t_class_string;
        if (!__MCJavaStringToJString(MCNameGetString(p_class_name), t_class_string))
            return nullptr;
        
        extern void* MCAndroidGetClassLoader(void);
        jobject t_class_loader = static_cast<jobject>(MCAndroidGetClassLoader());
        
        jclass t_class_loader_class = s_env->FindClass("java/lang/ClassLoader");
        jmethodID t_find_class = s_env->GetMethodID(t_class_loader_class,
                                                    "findClass",
                                                    "(Ljava/lang/String;)Ljava/lang/Class;");
        
        jobject t_class = s_env->CallObjectMethod(t_class_loader,
                                                  t_find_class,
                                                  t_class_string);
        
        return static_cast<jclass>(t_class);
#else
        return nullptr;
#endif
    }
    
    MCAutoStringRef t_class_path;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class_path))
        return nullptr;
    
    MCAutoStringRefAsCString t_class_cstring;
    if (!t_class_cstring.Lock(*t_class_path))
        return nullptr;
    
    return s_env->FindClass(*t_class_cstring);
}

bool MCJavaCreateInterfaceProxy(MCNameRef p_class_name, MCTypeInfoRef p_signature, void *p_method_id, void *r_result, void **p_args, uindex_t p_arg_count)
{
    if (MCHandlerTypeInfoGetParameterCount(p_signature) != 1)
        return false;
    
    MCValueRef t_handlers = *(static_cast<MCValueRef *>(p_args[0]));
    if (MCValueGetTypeCode(t_handlers) == kMCValueTypeCodeArray)
    {
        // Array of handlers for interface proxy
    }
    else if (MCValueGetTypeCode(t_handlers) == kMCValueTypeCodeHandler)
    {
        // Single handler for listener interface
    }
    else
    {
        return false;
    }
    
    jclass t_inv_handler_class =
        MCJavaPrivateFindClass(MCNAME("com.runrev.android.LCBInvocationHandler"));

    jmethodID t_method = static_cast<jmethodID>(p_method_id);
    
    jclass t_interface = MCJavaPrivateFindClass(p_class_name);
    
    jlong t_handler = reinterpret_cast<jlong>(MCValueRetain(t_handlers));
    
    jobject t_proxy = s_env->CallStaticObjectMethod(t_inv_handler_class,
                                                    t_method,
                                                    t_interface,
                                                    t_handler);
    
    MCJavaObjectRef t_result_value;
    if (!MCJavaObjectCreateNullableGlobalRef(t_proxy, t_result_value))
        return false;
    *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
    return true;
}

bool MCJavaPrivateCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count)
{
    if (p_method_id == nullptr)
        return false;

    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    MCJavaType t_return_type;
    if (!__GetExpectedTypeCode(MCHandlerTypeInfoGetReturnType(p_signature), t_return_type))
        return false;
    
    jvalue *t_params = nullptr;
    if (p_call_type != MCJavaCallTypeInterfaceProxy &&
        !__JavaJNIGetParams(p_args, p_signature, t_params))
        return false;
    
    switch (p_call_type)
    {
        case MCJavaCallTypeInterfaceProxy:
            MCAssert(t_return_type == kMCJavaTypeObject);
            if (!MCJavaCreateInterfaceProxy(p_class_name, p_signature,
                                            p_method_id, r_return,
                                            p_args, p_arg_count))
                return false;
            break;
            
        // JavaJNI...Result functions only return false due to memory
        // allocation failures. If they succeed, fall through the switch
        // statement and check the JNIEnv for exceptions.
        case MCJavaCallTypeConstructor:
        {
            MCAssert(t_return_type == kMCJavaTypeObject);
            jclass t_target_class = MCJavaPrivateFindClass(p_class_name);
            if (!__JavaJNIConstructorResult(t_target_class,
                                            static_cast<jmethodID>(p_method_id),
                                            &t_params[0],
                                            r_return))
                return false;
            
            break;
        }
        case MCJavaCallTypeInstance:
        {
            // Java object on which to call instance method should always be first argument.
            MCAssert(t_param_count > 0);
            jobject t_instance = t_params[0].l;
            if (t_param_count > 1)
            {
                if (!__JavaJNIInstanceMethodResult(t_instance,
                                                   static_cast<jmethodID>(p_method_id),
                                                   &t_params[1],
                                                   t_return_type,
                                                   r_return))
                    return false;
            }
            else
            {
                if (!__JavaJNIInstanceMethodResult(t_instance,
                                                   static_cast<jmethodID>(p_method_id),
                                                   nullptr,
                                                   t_return_type,
                                                   r_return))
                    return false;
            }
            break;
        }
        case MCJavaCallTypeStatic:
        {
            jclass t_target_class = MCJavaPrivateFindClass(p_class_name);
            if (! __JavaJNIStaticMethodResult(t_target_class,
                                              static_cast<jmethodID>(p_method_id),
                                              t_params, t_return_type,
                                              r_return))
                return false;
            
            break;
        }
        case MCJavaCallTypeNonVirtual:
        {
            MCAssert(t_param_count > 0);
            jobject t_instance = t_params[0].l;
            jclass t_target_class = MCJavaPrivateFindClass(p_class_name);
            if (t_param_count > 1)
            {
                if (!__JavaJNINonVirtualMethodResult(t_instance,
                                                    t_target_class,
                                                    static_cast<jmethodID>(p_method_id),
                                                    &t_params[1],
                                                    t_return_type,
                                                    r_return))
                    return false;
            }
            else
            {
                if (!__JavaJNINonVirtualMethodResult(t_instance,
                                                     t_target_class,
                                                     static_cast<jmethodID>(p_method_id),
                                                     nullptr,
                                                     t_return_type,
                                                     r_return))
                    return false;
            }
            break;
        }
        case MCJavaCallTypeGetter:
        case MCJavaCallTypeSetter:
        {
            MCAssert(t_param_count > 0);
            jobject t_instance = t_params[0].l;
            
            if (p_call_type == MCJavaCallTypeGetter)
            {
                if (!__JavaJNIGetFieldResult(t_instance,
                                             static_cast<jfieldID>(p_method_id),
                                             t_return_type,
                                             r_return))
                    return false;

                break;
            }
            
            MCJavaType t_field_type;
            if (!__GetExpectedTypeCode(MCHandlerTypeInfoGetParameterType(p_signature, 1),
                                       t_field_type))
                return false;
            
            __JavaJNISetFieldResult(t_instance,
                                    static_cast<jfieldID>(p_method_id),
                                    &t_params[1],
                                    t_field_type);
            break;
        }
        case MCJavaCallTypeStaticGetter:
        case MCJavaCallTypeStaticSetter:
        {
            jclass t_target_class = MCJavaPrivateFindClass(p_class_name);
            if (p_call_type == MCJavaCallTypeStaticGetter)
            {
                if (!__JavaJNIGetStaticFieldResult(t_target_class,
                                                   static_cast<jfieldID>(p_method_id),
                                                   t_return_type,
                                                   r_return))
                    return false;
                
                break;
            }
            
            MCJavaType t_field_type;
            if (!__GetExpectedTypeCode(MCHandlerTypeInfoGetParameterType(p_signature, 1),
                                       t_field_type))
                return false;
            
            __JavaJNISetStaticFieldResult(t_target_class,
                                          static_cast<jfieldID>(p_method_id),
                                          &t_params[0],
                                          t_field_type);
            break;
        }
        default:
            MCUnreachableReturn(false);
    }
    
    // If we got here there were no memory errors. Check the JNI Env for
    // exceptions
    if (s_env -> ExceptionCheck() == JNI_TRUE)
    {
#ifdef _DEBUG
        s_env -> ExceptionDescribe();
#endif
        
        // Failure to clear the exception causes a crash when the JNI is
        // next used.
        s_env -> ExceptionClear();
        return MCJavaPrivateErrorThrow(kMCJavaNativeMethodCallErrorTypeInfo);
    }
    
    return true;
}

bool MCJavaPrivateObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
    MCJavaObjectRef t_obj = static_cast<MCJavaObjectRef>(p_value);
    
    MCAutoStringRef t_class_name;
    if (!MCJavaPrivateGetJObjectClassName(t_obj, &t_class_name))
        return false;
    
    void *t_object = MCJavaObjectGetObject(t_obj);
    
    return MCStringFormat(r_desc, "<java: %@ - address: %p>", *t_class_name, t_object);
}

bool MCJavaPrivateConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
    jstring t_string;
    if (!__MCJavaStringToJString(p_string, t_string))
        return false;
    
    return MCJavaObjectCreateGlobalRef(t_string, r_object);
}

bool MCJavaPrivateConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string)
{
    auto t_string = static_cast<jstring>(MCJavaObjectGetObject(p_object));
    return __MCJavaStringFromJString(t_string, r_string);
}

bool MCJavaPrivateConvertDataRefToJByteArray(MCDataRef p_data, MCJavaObjectRef &r_object)
{
    jbyteArray t_array;
    if (!__MCJavaDataToJByteArray(p_data, t_array))
        return false;
    
    return MCJavaObjectCreateGlobalRef(t_array, r_object);
}

bool MCJavaPrivateConvertJByteArrayToDataRef(MCJavaObjectRef p_object, MCDataRef &r_data)
{
    auto t_data = static_cast<jbyteArray>(MCJavaObjectGetObject(p_object));
    return __MCJavaDataFromJByteArray(t_data, r_data);
}

bool MCJavaPrivateConvertProperListToJObjectArray(MCProperListRef p_list, MCJavaObjectRef& r_jobject_array)
{
    jobjectArray t_array;
    if (!__MCJavaProperListToJObjectArray(p_list, t_array))
        return false;
    
    return MCJavaObjectCreateGlobalRef(t_array, r_jobject_array);
}

bool MCJavaPrivateConvertJObjectArrayToProperList(MCJavaObjectRef p_jobject_array, MCProperListRef& r_list)
{
    auto t_array = static_cast<jobjectArray>(MCJavaObjectGetObject(p_jobject_array));
    return __MCJavaProperListFromJObjectArray(t_array, r_list);
}

bool MCJavaPrivateGetJObjectClassName(MCJavaObjectRef p_object, MCStringRef &r_name)
{
    jobject t_object = static_cast<jobject>(MCJavaObjectGetObject(p_object));
    jstring t_class_name = MCJavaGetJObjectClassName(t_object);
    return __MCJavaStringFromJString(t_class_name, r_name);
}

void* MCJavaPrivateGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_arguments, MCStringRef p_return, int p_call_type)
{
    MCAutoStringRefAsCString t_method_cstring, t_return_cstring;
    t_method_cstring . Lock(p_method_name);
    t_return_cstring . Lock(p_return);
    
    MCJavaDoAttachCurrentThread();
    
    jclass t_java_class = MCJavaPrivateFindClass(p_class_name);
    
    void *t_id = nullptr;
    if (t_java_class != nullptr)
    {
        switch (p_call_type)
        {
            case MCJavaCallTypeInstance:
            case MCJavaCallTypeNonVirtual:
            case MCJavaCallTypeStatic:
            {
                MCAutoStringRef t_sig;
                if (!MCStringCreateWithStrings(&t_sig, p_arguments, p_return))
                    return nullptr;
                
                MCAutoStringRefAsCString t_signature_cstring;
                if (!t_signature_cstring . Lock(*t_sig))
                    return nullptr;
                
                if (p_call_type == MCJavaCallTypeStatic)
                {
                    t_id = s_env->GetStaticMethodID(t_java_class, *t_method_cstring, *t_signature_cstring);
                    break;
                }
                
                t_id = s_env->GetMethodID(t_java_class, *t_method_cstring, *t_signature_cstring);
                break;
            }
            case MCJavaCallTypeConstructor:
            {
                // Constructors are called with a void return type, and using
                // the method name <init>
                MCAutoStringRef t_sig;
                if (!MCStringFormat(&t_sig, "%@%s", p_arguments, "V"))
                    return nullptr;
            
                MCAutoStringRefAsCString t_signature_cstring;
                if (!t_signature_cstring . Lock(*t_sig))
                    return nullptr;
    
                t_id = s_env->GetMethodID(t_java_class, "<init>", *t_signature_cstring);
                break;
            }
            case MCJavaCallTypeInterfaceProxy:
            {
                jclass t_inv_handler_class =
                    MCJavaPrivateFindClass(MCNAME("com.runrev.android.LCBInvocationHandler"));
                
                t_id = s_env->GetStaticMethodID(t_inv_handler_class,
                                                "getProxy",
                                                "(Ljava/lang/Class;J)Ljava/lang/Object;");
                break;
            }
            case MCJavaCallTypeGetter:
                t_id = s_env -> GetFieldID(t_java_class, *t_method_cstring, *t_return_cstring);
                break;
            case MCJavaCallTypeStaticGetter:
                t_id = s_env -> GetStaticFieldID(t_java_class, *t_method_cstring, *t_return_cstring);
                break;
                
            case MCJavaCallTypeStaticSetter:
            case MCJavaCallTypeSetter:
            {
                // Remove brackets from arg string to find field type
                MCAutoStringRef t_args;
                if (!__RemoveSurroundingParentheses(p_arguments, &t_args))
                    return nullptr;
                
                MCAutoStringRefAsCString t_signature_cstring;
                if (!t_signature_cstring . Lock(*t_args))
                    return nullptr;
                
                if (p_call_type == MCJavaCallTypeSetter)
                {
                    t_id = s_env -> GetFieldID(t_java_class, *t_method_cstring, *t_signature_cstring);
                    break;
                }
                t_id = s_env -> GetStaticFieldID(t_java_class, *t_method_cstring, *t_signature_cstring);
                break;
            }
        }
    }
 
    // If we got here there were no memory errors. Check the JNI Env for
    // exceptions
    if (s_env -> ExceptionCheck() == JNI_TRUE)
    {
#ifdef _DEBUG
        s_env -> ExceptionDescribe();
#endif
        
        // Failure to clear the exception causes a crash when the JNI is
        // next used.
        s_env -> ExceptionClear();
        MCJavaPrivateErrorThrow(kMCJavaNativeMethodCallErrorTypeInfo);
        return nullptr;
    }
    return t_id;
}

void MCJavaPrivateDestroyObject(MCJavaObjectRef p_object)
{
    MCJavaDoAttachCurrentThread();
    
    jobject t_obj = static_cast<jobject>(MCJavaObjectGetObject(p_object));
    
    s_env -> DeleteGlobalRef(t_obj);
}

void MCJavaPrivateDoNativeListenerCallback(jlong p_handler, jstring p_method_name, jobjectArray p_args)
{
    MCValueRef t_handler = nullptr;
    
    MCValueRef t_handlers = reinterpret_cast<MCValueRef>(p_handler);
    if (MCValueGetTypeCode(t_handlers) == kMCValueTypeCodeArray)
    {
        // Array of handlers for interface proxy
        MCAutoStringRef t_name;
        MCNewAutoNameRef t_key;
        if (!__MCJavaStringFromJString(p_method_name, &t_name) ||
            !MCNameCreate(*t_name, &t_key) ||
            !MCArrayFetchValue(static_cast<MCArrayRef>(t_handlers),
                              false, *t_key, t_handler) ||
            MCValueGetTypeCode(t_handler) != kMCValueTypeCodeHandler)
        {
            t_handler = nullptr;
        }
    }
    else
    {
        MCAssert(MCValueGetTypeCode(t_handlers) == kMCValueTypeCodeHandler);
        // Single handler for listener interface
        t_handler = t_handlers;
    }
    
    if (t_handler == nullptr)
    {
        // Throw
        return;
    }

    // We have an LCB handler, so just invoke with the args.
    MCValueRef t_result;
    MCAutoProperListRef t_list;
    if (!__MCJavaProperListFromJObjectArray(p_args, &t_list))
    {
        // Throw
        return;
    }
    
    MCProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(*t_list, t_mutable_list))
        return;
    
    MCHandlerTryToInvokeWithList(static_cast<MCHandlerRef>(t_handler),
                                 t_mutable_list, t_result);
    
    MCValueRelease(t_result);
    MCValueRelease(t_mutable_list);
}
#else

bool initialise_jvm()
{

    return true;
}

void finalise_jvm()
{
}

bool MCJavaPrivateCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count)
{
    return false;
}

bool MCJavaPrivateObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
    return MCStringFormat (r_desc, "<java: %s>", "not supported");
}

bool MCJavaPrivateConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
    return false;
}

bool MCJavaPrivateConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string)
{
    return false;
}

bool MCJavaPrivateConvertDataRefToJByteArray(MCDataRef p_string, MCJavaObjectRef &r_object)
{
    return false;
}

bool MCJavaPrivateConvertJByteArrayToDataRef(MCJavaObjectRef p_object, MCDataRef &r_string)
{
    return false;
}

bool MCJavaPrivateConvertProperListToJObjectArray(MCProperListRef p_list, MCJavaObjectRef& r_jobject_array)
{
    return false;
}

bool MCJavaPrivateConvertJObjectArrayToProperList(MCJavaObjectRef p_jobject_array, MCProperListRef& r_list)
{
    return false;
}


void* MCJavaPrivateGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_arguments, MCStringRef p_return, int p_call_type)
{
    return nullptr;
}

void MCJavaPrivateDestroyObject(MCJavaObjectRef p_object)
{
    // no op
}

bool MCJavaPrivateGetJObjectClassName(MCJavaObjectRef p_object, MCStringRef &r_name)
{
    return false;
}
#endif
