/* Copyright (C) 2017 LiveCode Ltd.
 
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

#include "libscript/script.h"
#include "script-private.h"

#include "ffi.h"

#include "foundation-auto.h"

#include <Foundation/Foundation.h>

#include "script-execute.hpp"

bool MCScriptCallObjCCatchingErrors(ffi_cif *p_cif, void (*p_function)(), void *p_result_ptr, void **p_arg_ptrs)
{
    @try
    {
        ffi_call(p_cif, p_function, p_result_ptr, p_arg_ptrs);
        return true;
    }
    @catch (NSException *exception)
    {
        MCAutoStringRef t_reason;
        if (!MCStringCreateWithCFStringRef((CFStringRef)[exception reason], &t_reason))
        {
            return false;
        }
        
        return MCScriptThrowForeignExceptionError(*t_reason);
    }
    
    return false;
}

#import <objc/runtime.h>

bool MCScriptProtocolProxyClassName(MCStringRef p_protocol_name, MCStringRef& r_proxy_name)
{
    return MCStringFormat(r_proxy_name, "_%@_proxy", p_protocol_name);
}

Class MCScriptCreateProtocolProxyClass(MCStringRef p_protocol_name)
{
    MCAutoStringRefAsCString t_protocol;
    if (!t_protocol.Lock(p_protocol_name))
        return nullptr;
    
    // Check protocol exists
    if (objc_getProtocol(*t_protocol) == nullptr)
        return nullptr;
    
    MCAutoStringRef t_proxy_name;
    if (!MCScriptProtocolProxyClassName(p_protocol_name, &t_proxy_name))
        return nullptr;
    
    MCAutoStringRefAsCString t_classname;
    if (!t_classname.Lock(*t_proxy_name))
        return nullptr;
    
    Class t_new_class = objc_getClass(*t_classname);
    if (t_new_class == nullptr)
    {
        t_new_class = objc_allocateClassPair([NSObject class], *t_classname, 0);
        if (t_new_class != nullptr)
            objc_registerClassPair(t_new_class);
    }

    return t_new_class;
}

static void callbackIMP(id self, SEL _cmd, id mapview, id overlay)
{
    NSString *t_selector = NSStringFromSelector(_cmd);
    NSString *t_selector_impl = [t_selector stringByAppendingString:@"_callback"];
    SEL t_callback_selector = NSSelectorFromString(t_selector_impl);
    NSInvocation *inv = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:t_callback_selector]];
    [inv setSelector:t_callback_selector];
    [inv setTarget:self];
    
    [inv setArgument:&(mapview) atIndex:2]; //arguments 0 and 1 are self and _cmd respectively, automatically set by NSInvocation
    [inv setArgument:&(overlay) atIndex:3]; //arguments 0 and 1 are self and _cmd respectively,
    
    [inv invoke];
}

bool MCScriptCreateProtocolProxy(MCNameRef p_proxy_class, MCTypeInfoRef p_signature, void *r_result, void **p_args, uindex_t p_arg_count)
{
    if (MCHandlerTypeInfoGetParameterCount(p_signature) != 1)
        return false;
    
    MCArrayRef t_handlers = *(static_cast<MCArrayRef *>(p_args[0]));
    
    /*
    if (!__MCScriptObjCIsHandlerSuitableForProxy(p_proxy_class, t_handlers))
        return false;
    */
    MCAutoStringRef t_proxy_name;
    if (!MCScriptProtocolProxyClassName(MCNameGetString(p_proxy_class), &t_proxy_name))
        return false;
    
    MCAutoStringRefAsCString t_classname;
    if (!t_classname.Lock(*t_proxy_name))
        return false;
    
    Class t_proxy_class = objc_getClass(*t_classname);
    if (t_proxy_class == nullptr)
        return false;
    
    MCAutoStringRefAsCString t_protocol_name;
    if (!t_protocol_name.Lock(MCNameGetString(p_proxy_class)))
        return false;
    Protocol *t_protocol = objc_getProtocol(*t_protocol_name);
    
    // Lambda to add a callback to an instance for the given selector name
    auto t_add_handler = [&](MCHandlerRef p_handler, Class p_proxy_class, MCStringRef p_selector)
    {
        CFStringRef t_string;
        MCStringConvertToCFStringRef(p_selector, t_string);
        SEL t_proxy_selector = NSSelectorFromString((NSString *)t_string);
        
        objc_method_description t_desc =
            protocol_getMethodDescription(t_protocol, t_proxy_selector, NO, YES);

        class_addMethod(p_proxy_class, t_proxy_selector,
                        (IMP)callbackIMP, t_desc.types);
        
        MCAutoStringRef t_callback_name;
        MCStringFormat(&t_callback_name, "%@_callback", p_selector);
        MCAutoStringRefAsCString t_cstring;
        t_cstring.Lock(*t_callback_name);
        SEL t_callback_selector = sel_registerName(*t_cstring);
        class_addMethod(p_proxy_class, t_callback_selector,
                        imp_implementationWithBlock(^ void* (id mapview, id overlay) {
            
            MCAutoProperListRef t_list;
            if (!MCProperListCreateMutable(&t_list))
                return nullptr;
            
            const char* t_type = t_desc.types;
            
            for (uint32_t i = 0; i < strlen(t_desc.types); ++i)
            {
                MCValueRef t_value;
                switch (*t_type++)
                {
                    case '@':
                        // objc object
                        id t_id = nullptr;
                        if (i == 0)
                            t_id = mapview;
                        else
                            t_id = overlay;
                        MCObjcObjectCreateWithId(t_id, (MCObjcObjectRef&)t_value);
                        break;
                }
                
                if (!MCProperListPushElementOntoBack(*t_list, t_value))
                    return nullptr;
            }
            
            MCProperListRef t_mutable_list;
            if (!MCProperListMutableCopy(*t_list, t_mutable_list))
                return nullptr;
            
            MCValueRef t_result;
            MCHandlerTryToExternalInvokeWithList(p_handler, t_mutable_list, t_result);
            MCValueRelease(t_mutable_list);
            return (void *)t_result;
        }), "^v@:@@");
    };
    
    // The array keys are not added in any particular order
    MCNameRef t_key;
    MCValueRef t_value;
    uintptr_t t_iterator = 0;
    while (MCArrayIterate(t_handlers, t_iterator, t_key, t_value))
    {
        t_add_handler((MCHandlerRef)t_value, t_proxy_class, MCNameGetString(t_key));
    }
    
    class_addProtocol(t_proxy_class, t_protocol);
    
    id t_instance = class_createInstance(t_proxy_class, 0);
    *(static_cast<id *>(r_result)) = t_instance;
    return true;
}
