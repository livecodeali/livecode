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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "exec.h"

#include "mblandroidjava.h"

uint32_t NativeToUnicode(const char *p_string, uint32_t p_string_length, unichar_t *p_buffer, uint32_t p_buffer_length);
uint32_t UnicodeToNative(const unichar_t *p_string, uint32_t p_string_length, char *p_buffer, uint32_t p_buffer_length);

static bool native_to_unicode(const char *p_native, uint32_t p_length, unichar_t *&r_unicode)
{
    if (p_native == nil)
    {
        r_unicode = nil;
        return true;
    }
   
    bool t_success = true;
    
    unichar_t *t_unicode = nil;
    
    t_success = MCMemoryAllocate(2 * p_length, t_unicode);
    
    if (t_success)
        t_success = p_length == NativeToUnicode(p_native, p_length, t_unicode, p_length * 2);
    
    if (t_success)
        r_unicode = t_unicode;
    else
        MCMemoryDeallocate(t_unicode);
    
    return t_success;
}

static bool unicode_to_native(const unichar_t *p_unicode, uint32_t p_length, char *&r_native)
{
    if (p_unicode == nil)
    {
        r_native = nil;
        return true;
    }
    
    bool t_success = true;
    
    char *t_native = nil;
    
    t_success = MCMemoryAllocate(p_length + 1, t_native);
    
    if (t_success)
        t_success = p_length == UnicodeToNative(p_unicode, p_length, t_native, p_length);
    
    if (t_success)
    {
        t_native[p_length] = '\0';
        r_native = t_native;
    }
    else
        MCCStringFree(t_native);
    
    return t_success;
}

bool MCJavaStringFromNative(JNIEnv *env, const MCString *p_string, jstring &r_java_string)
{
    if (p_string == nil)
    {
        r_java_string = nil;
        return true;
    }
    
    bool t_success = true;
    
    jstring t_java_string = nil;
    unichar_t *t_unicode = nil;
    
    t_success = native_to_unicode(p_string->getstring(), p_string->getlength(), t_unicode);
    if (t_success)
        t_success = nil != (t_java_string = env -> NewString((jchar*)t_unicode, p_string->getlength()));
    
    MCMemoryDeallocate(t_unicode);

    if (t_success)
        r_java_string = t_java_string;
    
    return t_success;
}

bool MCJavaStringFromNative(JNIEnv *env, const char *p_string, jstring &r_java_string)
{
    if (p_string == nil)
    {
        r_java_string = nil;
        return true;
    }
    else
    {
        MCString t_mcstring(p_string);
        return MCJavaStringFromNative(env, &t_mcstring, r_java_string);
    }
}

bool MCJavaStringFromUnicode(JNIEnv *env, const MCString *p_string, jstring &r_java_string)
{
    if (p_string == nil)
    {
        r_java_string = nil;
        return true;
    }
    
    bool t_success = true;
    jstring t_java_string = nil;
    
    t_success = nil != (t_java_string = env -> NewString((const jchar*)p_string->getstring(), p_string->getlength() / 2));
    
    if (t_success)
        r_java_string = t_java_string;
    
    return t_success;
}

bool MCJavaStringToUnicode(JNIEnv *env, jstring p_java_string, unichar_t *&r_unicode, uint32_t &r_length)
{
    bool t_success = true;
    
    const jchar *t_unicode_string = nil;
    int t_unicode_length = 0;
    unichar_t *t_copy = nil;
    
    if (p_java_string != nil)
        t_unicode_string = env -> GetStringChars(p_java_string, NULL);
    
    if (t_unicode_string != nil)
    {
        t_unicode_length = env -> GetStringLength(p_java_string);
        t_success = MCMemoryAllocateCopy(t_unicode_string, t_unicode_length * 2, (void*&)t_copy);
        env -> ReleaseStringChars(p_java_string, t_unicode_string);
    }
    
    if (t_success)
    {
        r_unicode = t_copy;
        r_length = t_unicode_length;
    }
    
    return t_success;
}

bool MCJavaStringToNative(JNIEnv *env, jstring p_java_string, char *&r_native)
{
    bool t_success = true;
    
    const jchar *t_unicode_string = nil;
    uint32_t t_unicode_length = 0;
    
    char *t_native = nil;
    uint32_t t_length = 0;
    
    if (p_java_string != nil)
        t_unicode_string = env -> GetStringChars(p_java_string, NULL);
    
    if (t_unicode_string != nil)
    {
        t_unicode_length = env -> GetStringLength(p_java_string);
        
        if (t_success)
            t_success = unicode_to_native(t_unicode_string, t_unicode_length, t_native);
        
        env -> ReleaseStringChars(p_java_string, t_unicode_string);
    }
    
    if (t_success)
        r_native = t_native;
    
    return t_success;
}
