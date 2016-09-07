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


void MCJavaGetJObject(MCNameRef p_classname)
{
    jclass cls1 = (*env)->GetObjectClass(env, obj);
    if (cls1 == 0)
        ... /* error */
        cls = (*env)->NewGlobalRef(env, cls1);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCJavaObjectTypeInfo;

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCJavaObjectTypeInfo() { return kMCJavaObjectTypeInfo; }

extern "C" MC_DLLEXPORT_DEF void MCJavaStringFromJavaString(MCJavaObjectRef p_object, MCStringRef &r_string)
{
    r_string = MCValueRetain(kMCEmptyString);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_java_Initialize(void)
{
    return MCJavaTypeInfoCreate(MCNAME("java.lang.object"), kMCJavaObjectTypeInfo);
}

extern "C" void com_livecode_java_Finalize(void)
{
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////

