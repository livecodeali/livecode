/* Copyright (C) 2016 LiveCode Ltd.
 
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

////////////////////////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCReflectTypeTypeInfo;

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCReflectTypeTypeInfo() { return kMCReflectTypeTypeInfo; }

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_reflect_Initialize(void)
{
    return MCNamedTypeInfoCreate(MCNAME("com.livecode.type"), kMCReflectTypeTypeInfo);
}

extern "C" void com_livecode_reflect_Finalize(void)
{
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////

