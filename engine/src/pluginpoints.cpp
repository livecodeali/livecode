
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

#include "pluginpoints.h"
#include "globals.h"
#include "foundation-auto.h"

////////////////////////////////////////////////////////////////////////////////

bool MCEnginePlugin::IsCompatible(const MCPluginHandlerList *p_list)
{
	for (uindex_t i = 0; i < p_list -> size; i++)
	{
		MCValueRef t_value;
		if (!MCArrayFetchValue(m_bindings, false, MCNAME(p_list -> table[i]), t_value))
			return false;
		
		if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeHandler)
			return false;
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCEnginePluginLCB::MCEnginePluginLCB(MCScriptModuleRef p_module, MCArrayRef p_bindings)
{
	m_bindings = MCValueRetain(p_bindings);
	MCScriptCreateInstanceOfModule(p_module, &m_instance);
}

MCEnginePluginLCB::~MCEnginePluginLCB()
{
	
}

bool MCEnginePluginLCB::Invoke(MCExecContext& ctxt, MCNameRef p_handler, MCValueRef *p_args, uindex_t p_count)
{
	// We have done the checking in the bind step, so we know the
	// array has a value with the given key and that it is a handler
	
	MCValueRef t_handler;
	MCAssert(MCArrayFetchValue(m_bindings, false, p_handler, t_handler));
	
	MCAssert(MCValueGetTypeCode(t_handler) == kMCValueTypeCodeHandler);
	
	MCAutoValueRef t_result;
	if (!MCHandlerInvoke((MCHandlerRef)t_handler, p_args, p_count, &t_result))
		return false;
	
	if (*t_result != nil)
	{
		ctxt . SetTheResultToValue(*t_result);
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCEnginePluginPoint::MCEnginePluginPoint()
{
	m_plugin = nil;
}

MCEnginePluginPoint::~MCEnginePluginPoint()
{
/*
	if (m_plugin != nil)
		m_plugin -> Release();
 */
}

bool MCEnginePluginPoint::HandlerList(MCProperListRef& r_list)
{
	MCAutoProperListRef t_list;
	if (!MCProperListCreateMutable(&t_list))
		return false;
	
	const MCPluginHandlerList *t_handler_list;
	t_handler_list = GetHandlerList();
	
	for (uindex_t i = 0; i < t_handler_list -> size; i++)
	{
		if (!MCProperListPushElementOntoBack(*t_list, MCNAME(t_handler_list -> table[i])))
			return false;
	}
	
	r_list = MCValueRetain(*t_list);
	return true;
}

bool MCEnginePluginPoint::Bind(MCEnginePlugin* p_plugin)
{
	if (!p_plugin -> IsCompatible(GetHandlerList()))
		return false;
	
	m_plugin = p_plugin;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

const char* MCPutOutputPluginPoint::kHandlers[] =
{
	"PutOutput",
};

MCPluginHandlerList MCPutOutputPluginPoint::kHandlerList =
{
	sizeof(kHandlers) / sizeof(kHandlers[0]),
	&kHandlers[0],
};

void MCPutOutputPluginPoint::PutOutput(MCExecContext& ctxt, MCStringRef p_to_put)
{
	MCAssert(m_plugin != nil);

	MCAutoValueRefArray t_args;
	if (!t_args . New(1))
	{
		ctxt . Throw();
		return;
	}
	
	t_args[0] = MCValueRetain(p_to_put);
	m_plugin -> Invoke(ctxt, MCNAME("PutOutput"), t_args . Ptr(), t_args . Count());
}

////////////////////////////////////////////////////////////////////////////////

bool MCPluginPointsInitialize()
{
	if (!MCMemoryNew(MCputoutputpluginpoint))
		return false;
	
	return true;
}

static bool MCPluginPointLookup(MCNameRef p_point_name, MCEnginePluginPoint** r_plugin_point)
{
	*r_plugin_point = MCputoutputpluginpoint;
	return true;
}

bool MCPluginPointBindLCB(MCNameRef p_module, MCNameRef p_plugin_point, MCArrayRef p_bindings)
{
	MCEnginePluginPoint *t_point;
	if (!MCPluginPointLookup(p_plugin_point, &t_point))
		return false;
	
	MCAutoScriptModuleRef t_module;
	if (!MCScriptLookupModule(p_module, &t_module))
		return false;
	
	MCEnginePluginLCB *t_plugin = new (nothrow) MCEnginePluginLCB(*t_module, p_bindings);
	if (t_plugin == nil)
		return false;
	
	if (!t_point -> Bind(t_plugin))
	{
		delete t_plugin;
		return false;
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////