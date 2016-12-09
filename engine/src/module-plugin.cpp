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

////////////////////////////////////////////////////////////////////////////////

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "pluginpoints.h"

extern "C" MC_DLLEXPORT_DEF void MCPluginBind(MCStringRef p_module, MCStringRef p_plugin_point, MCArrayRef p_bindings)
{
	MCNewAutoNameRef t_module, t_plugin;
	MCNameCreate(p_module, &t_module);
	MCNameCreate(p_plugin_point, &t_plugin);
	
	MCPluginPointBindLCB(*t_module, *t_plugin, p_bindings);
}

/*
MC_DLLEXPORT_DEF MCTypeInfoRef kMCEngineScriptObjectDoesNotExistErrorTypeInfo = nil;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCEngineScriptObjectNoContextErrorTypeInfo = nil;
*/
////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_plugin_Initialize(void)
{   
    return true;
}

extern "C" void com_livecode_plugin_Finalize(void)
{

}

////////////////////////////////////////////////////////////////////////////////
