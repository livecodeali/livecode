#ifndef pluginpoints_h
#define pluginpoints_h

#include "foundation.h"
#include "prefix.h"

#include "libscript/script.h"
#include "libscript/script-auto.h"
#include "exec.h"

struct MCPluginHandlerList
{
	uindex_t size;
	const char **table;
};

class MCEnginePlugin
{
public:
	//virtual void Retain();
	//virtual void Release();
	
	virtual ~MCEnginePlugin() { };
	virtual bool Invoke(MCExecContext& ctxt, MCNameRef p_handler, MCValueRef *args, uindex_t p_count) = 0;
	
	bool IsCompatible(const MCPluginHandlerList *list);
	
protected:
	
	MCArrayRef m_bindings;
};


class MCEnginePluginLCB : public MCEnginePlugin
{
public:
	
	MCEnginePluginLCB(MCScriptModuleRef, MCArrayRef);
	~MCEnginePluginLCB();
	
	virtual bool Invoke(MCExecContext& ctxt, MCNameRef p_handler, MCValueRef *args, uindex_t p_count);
private:
	
	MCAutoScriptInstanceRef m_instance;
};

////////////////////////////////////////////////////////////////////////////////

class MCEnginePluginPoint
{
public:
	
	MCEnginePluginPoint();
	virtual ~MCEnginePluginPoint();
	
	bool IsBound()
	{
		return m_plugin != nil;
	}
	
	bool Bind(MCEnginePlugin *p_plugin);
	virtual const MCPluginHandlerList *GetHandlerList(void) const = 0;

	bool HandlerList(MCProperListRef& r_list);
	
protected:
	
	MCEnginePlugin *m_plugin;
};

////////////////////////////////////////////////////////////////////////////////

class MCPutOutputPluginPoint :
public MCEnginePluginPoint
{
public:
	void PutOutput(MCExecContext& ctxt, MCStringRef p_to_put);
	virtual const MCPluginHandlerList *GetHandlerList(void) const { return &kHandlerList; }
	
private:
	
	static const char* kHandlers[];
	static MCPluginHandlerList kHandlerList;
};

////////////////////////////////////////////////////////////////////////////////

bool MCPluginPointsInitialize();
bool MCPluginPointBindLCB(MCNameRef p_module, MCNameRef p_plugin_point, MCArrayRef p_bindings);

#endif /* pluginpoints_h */
