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

'module' generate

'use'
    types
    support

'export'
    GeneratePackages

--------------------------------------------------------------------------------

'var' ModuleDependencyList : NAMELIST

'var' GeneratingPackageIndex : INT

'action' GeneratePackages(PACKAGELIST)

    'rule' GeneratePackages(List):
        OutputLCBBegin()
        GeneratingPackageIndex <- 1
        GenerateForEachPackage(List)
        OutputEnd()
    
'action' GenerateForEachPackage(PACKAGELIST)

    'rule' GenerateForEachPackage(packagelist(Head, Rest)):
       	GenerateSinglePackage(Head)
        GeneratingPackageIndex -> CurrentIndex
        GeneratingPackageIndex <- CurrentIndex + 1
        GenerateForEachPackage(Rest)
        
    'rule' GenerateForEachPackage(nil):
        -- do nothing

'action' GenerateSinglePackage(PACKAGE)

    'rule' GenerateSinglePackage(Package:package(_, _, WrappedId, Definitions)):
        ModuleDependencyList <- nil
        ResolveIdName(WrappedId -> Name)
        OutputWriteI("module ", Name, "\n\n")
        OutputWrite("use com.livecode.java\n")
        CollectImports(Definitions)
        ModuleDependencyList -> List
        OutputImports(List)
        OutputWrite("\n")
        GenerateForeignHandlers(Definitions)
        OutputWrite("\n")
        GenerateDefinitions(Definitions)
        OutputWrite("end module\n\n")

----------

'condition' IsNameInList(NAME, NAMELIST)
    'rule' IsNameInList(Id, namelist(Head, Tail)):
        IsNameEqualToName(Id, Head)
    'rule' IsNameInList(Id, namelist(Head, Tail)):
        IsNameInList(Id, Tail)

'action' AddModuleToDependencyList(NAME)

    'rule' AddModuleToDependencyList(Name):
        ModuleDependencyList -> List
        IsNameInList(Name, List)
        
    'rule' AddModuleToDependencyList(Name):
        ModuleDependencyList -> List
        ModuleDependencyList <- namelist(Name, List)

'action' CollectImports(DEFINITION)

    'rule' CollectImports(nil):
        -- done

    'rule' CollectImports(sequence(Head, Tail)):
        CollectImport(Head)
        CollectImports(Tail)

'action' CollectImport(DEFINITION)

    'rule' CollectImport(use(_, Id))
        QueryId(Id -> Meaning)
        QuerySymbolId(Id -> Info)
        Info'Type -> Type

        Info'Parent -> PackageId

        QueryPackageId(PackageId -> PackageInfo)
        PackageInfo'Alias -> Alias
        ResolveIdName(Alias -> Name)

        AddModuleToDependencyList(Name)

        -- Fetch the info about the symbol.
        QuerySymbolId(Id -> SymbolInfo)
        SymbolInfo'Kind -> SymbolKind
        SymbolInfo'Type -> SymbolType

        GeneratingPackageIndex -> Generator
        SymbolInfo'Generator <- Generator

    'rule' CollectImport(Id):
        -- If we get here then either the id isn't imported, or we have previously
        -- generated it.

'action' OutputImports(NAMELIST)

    'rule' OutputImports(nil):
        -- done

    'rule' OutputImports(namelist(Head, Tail)):
        OutputImport(Head)
        OutputImports(Tail)

'action' OutputImport(NAME)

    'rule' OutputImport(Name):
        OutputWriteI("use ", Name, "\n")
----

'action' GenerateForeignHandlers(DEFINITION)

    'rule' GenerateForeignHandlers(sequence(Head, Tail)):
    	GenerateForeignHandlers(Head)
    	GenerateForeignHandlers(Tail)
    	
    'rule' GenerateForeignHandlers(class(_, _, Type, Definitions, _, _)):
        GenerateForeignHandlersOfClass(Type, Definitions)

    'rule' GenerateForeignHandlers(Definition):
        -- foreign handler declaration not needed
/*
    'rule' GenerateForeignHandlers(class(_, _, Type, Definitions, _, _)):
        GenerateForeignHandlersOfClass(Type, Definitions)

    'rule' GenerateForeignHandlers(use(_, _)):
        -- foreign handler declaration not needed
*/

'action' GenerateForeignHandlersOfClass(TYPE, DEFINITION)

    'rule' GenerateForeignHandlersOfClass(Type, sequence(Head, Tail)):
    	GenerateForeignHandlerOfClass(Type, Head)
    	GenerateForeignHandlersOfClass(Type, Tail)

    'rule' GenerateForeignHandlersOfClass(Type, nil):
		-- finished

'action' GenerateForeignHandlerOfClass(TYPE, DEFINITION)

    'rule' GenerateForeignHandlerOfClass(Type, method(_, _, Id, Signature, Alias, _)):
    	(|
            where(Alias -> id(AliasName))
            ResolveIdName(AliasName -> MethodName)
        ||
            ResolveIdName(Id -> MethodName)
        |)
        
        OutputWrite("foreign handler ")
        OutputForeignHandlerName(Type, MethodName)
        OutputForeignSignatureWithParameter(Type, Signature)
        OutputBindingString(Type, MethodName, Signature)
        OutputWrite("\n")

    'rule' GenerateForeignHandlerOfClass(Type, Definition):
        -- nothing yet

'action' OutputForeignHandlerName(TYPE, NAME)

	'rule' OutputForeignHandlerName(Type, MethodName):
	    TypeToUnqualifiedName(Type -> ClassName)
		OutputWriteI("_JNI_", ClassName, "")
		OutputWriteI("_", MethodName, "")

    'rule' OutputForeignHandlerName(Type, MethodName):
    	print("fail")
    	print(MethodName)
		
'action' OutputForeignSignatureWithParameter(TYPE, SIGNATURE)

    'rule' OutputForeignSignatureWithParameter(ObjType, signature(Params, ReturnType)):
        OutputWrite("(in pObj as ")
        GenerateType(ObjType)
        (|
            where(Params -> nil)
        ||
            OutputWrite(", ")
            GenerateParams(Params)
        |)
        OutputWrite(")")
        GenerateReturns(ReturnType)
        
'action' OutputBindingString(TYPE, NAME, SIGNATURE)

	'rule' OutputBindingString(ClassType, MethodName, Signature):
        TypeToQualifiedName(ClassType -> QualifiedClass)
		OutputWriteI(" binds to \"java:", QualifiedClass, ">")
		OutputWriteI("", MethodName, "")
		OutputJavaSignature(Signature)
		OutputWrite("\"")
		
'action' OutputJavaSignature(SIGNATURE)
	
	'rule' OutputJavaSignature(signature(Params, ReturnType))
		OutputWrite("(")
        OutputJavaParams(Params)
        OutputWrite(")")
        OutputJavaTypeCode(ReturnType)
		
'action' OutputJavaParams(PARAMETERLIST)

    'rule' OutputJavaParams(nil):

    'rule' OutputJavaParams(parameterlist(Head, nil)):
        OutputJavaParam(Head)

    'rule' OutputJavaParams(parameterlist(Head, Tail)):
        OutputJavaParam(Head)
        OutputJavaParams(Tail)
	
'action' OutputJavaParam(PARAMETER)

	'rule' OutputJavaParam(parameter(_, _, Type)):
		OutputJavaTypeCode(Type)
----

'action' GenerateClassDefinitions(TYPE, DEFINITION)

    'rule' GenerateClassDefinitions(ObjType, sequence(Left, Right)):
        GenerateClassDefinitions(ObjType, Left)
        GenerateClassDefinitions(ObjType, Right)

    'rule' GenerateClassDefinitions(ObjType, constant(_, Id, Type, Value)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        TypeToUnqualifiedName(ObjType -> ObjName)
        (|
            where(Value -> nil)
            OutputWriteI("handler ", ObjName, "_")
            OutputWriteI("Get", SymbolName, "(in pObj as ")
            GenerateType(ObjType)
            OutputWrite(")")
            GenerateReturns(Type)
            OutputWrite("\n")
            OutputWrite("end handler")
        ||
            OutputWriteI("constant ", SymbolName, "")
            OutputWrite(" as ")
            GenerateType(Type)
            OutputWrite(" is ")
            GenerateValue(Value)
        |)
        OutputWrite("\n")

    'rule' GenerateClassDefinitions(ObjType, variable(_, Modifiers, Id, Type)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        TypeToUnqualifiedName(ObjType -> ObjName)
        OutputWriteI("handler ", ObjName, "_")
        OutputWriteI("Get", SymbolName, "(in pObj as ")
        GenerateReturns(Type)
        OutputWrite("\n")
        OutputWrite("end handler")
        OutputWrite("\n\n")

    'rule' GenerateClassDefinitions(ObjType, method(_, Modifiers, Id, Signature, Alias, Throws)):
        (|
            where(Alias -> id(AliasName))
            ResolveIdName(AliasName -> Name)
        ||
            ResolveIdName(Id -> Name)
        |)

        OutputWriteI("handler ", Name, "")
        GenerateSignatureWithParameter(ObjType, Signature)

        OutputWrite("\n\t")
        OutputCallForeignHandler(ObjType, Name, Signature)
        OutputWrite("end handler")
        OutputWrite("\n\n")

    'rule' GenerateClassDefinitions(ObjType, constructor(_, Modifiers, Id, Signature, Alias)):
        (|
            where(Alias -> id(AliasName))
            ResolveIdName(AliasName -> Name)
        ||
            ResolveIdName(Id -> Name)
        |)
        OutputWriteI("handler ", Name, "")
        GenerateSignatureWithReturnType(ObjType, Signature)

        OutputWrite("\n")
        OutputWrite("end handler")
        OutputWrite("\n\n")

    'rule' GenerateClassDefinitions(ObjType, nil):
        -- do nothing

'action' OutputCallForeignHandler(TYPE, NAME, SIGNATURE)

	'rule' OutputCallForeignHandler(ObjType, Name, signature(Params, ReturnType))
        (|
            where(ReturnType -> nil)
            OutputWrite("unsafe\n\t\t")
            OutputForeignHandlerName(ObjType, Name)	
            OutputWrite("()\n")
            OutputWrite("\tend unsafe\n")
        ||
            OutputWrite("variable tJNIResult as ")
            GenerateJavaType(ReturnType)
            OutputWrite("\n\tunsafe\n")
            OutputWrite("\t\tput ")
            OutputForeignHandlerName(ObjType, Name)	
            OutputWrite("() into tJNIResult\n")
            OutputWrite("\tend unsafe\n")

			OutputWrapperReturn(ReturnType)
        |)	
        
'action' OutputWrapperReturn(TYPE)

	'rule' OutputWrapperReturn(named(_, _, _))
		OutputWrite("\treturn tJNIResult\n")

	'rule' OutputWrapperReturn(template(_, _, _))
		OutputWrite("\treturn tJNIResult\n")

	'rule' OutputWrapperReturn(ReturnType)
		OutputWrite("\treturn ")
       	OutputConvertJava(ReturnType)
       	OutputWrite("(tJNIResult)\n")

'action' OutputConvertJava(TYPE)

	'rule' OutputConvertJava(Type):
		GenerateType(Type)
		OutputWrite("FromJ")
		GenerateJavaType(Type)

'action' GenerateSignatureWithParameter(TYPE, SIGNATURE)

    'rule' GenerateSignatureWithParameter(ObjType, signature(Params, ReturnType)):
        OutputWrite("(in pObj as ")
        GenerateType(ObjType)
        (|
            where(Params -> nil)
        ||
            OutputWrite(", ")
            GenerateParams(Params)
        |)
        OutputWrite(")")
        GenerateReturns(ReturnType)

'action' GenerateSignatureWithReturnType(TYPE, SIGNATURE)

    'rule' GenerateSignatureWithReturnType(ReturnType, signature(Params, nil)):
        OutputWrite("(")
        GenerateParams(Params)
        OutputWrite(")")
        GenerateReturns(ReturnType)

'action' GenerateParams(PARAMETERLIST)

    'rule' GenerateParams(nil):

    'rule' GenerateParams(parameterlist(Head, nil)):
        GenerateParam(Head)

    'rule' GenerateParams(parameterlist(Head, Tail)):
        GenerateParam(Head)
        OutputWrite(", ")
        GenerateParams(Tail)

'action' GenerateParam(PARAMETER)

    'rule' GenerateParam(parameter(_, Id, Type)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("in ", SymbolName, "")
        OutputWrite(" as ")
        GenerateType(Type)

'action' GenerateDefinitions(DEFINITION)

    'rule' GenerateDefinitions(sequence(Left, Right)):
        GenerateDefinitions(Left)
        GenerateDefinitions(Right)

    'rule' GenerateDefinitions(class(_, Modifiers, Type, Definitions, Inherits, Implements)):
        /*
        OutputWrite("class ")
        GenerateType(Type)
        GenerateInherits(Inherits)
        GenerateImplements(Implements)
        OutputWrite("\n")
        GenerateClassDefinitions(Type, Definitions)
        OutputWrite("end class\n\n")
        */
        GenerateClassDefinitions(Type, Definitions)

    'rule' GenerateDefinitions(interface(_, Type, Definitions, Inherits)):
        /*
        OutputWrite("interface ")
        GenerateType(Type)
        GenerateInherits(Inherits)
        OutputWrite("\n")
        GenerateClassDefinitions(Type, Definitions)
        OutputWrite("end interface\n\n")
        */
        GenerateClassDefinitions(Type, Definitions)

    'rule' GenerateDefinitions(use(_,_)):
        -- do nothing

    'rule' GenerateDefinitions(nil):
        -- do nothing

'action' GenerateInherits(TYPELIST)

    'rule' GenerateInherits(nil):

    'rule' GenerateInherits(Typelist)
        OutputWrite(" inherits ")
        GenerateTypelist(Typelist)

'action' GenerateImplements(TYPELIST)

    'rule' GenerateImplements(nil):

    'rule' GenerateImplements(Typelist)
        OutputWrite(" implements ")
        GenerateTypelist(Typelist)

'action' GenerateTypelist(TYPELIST)

    'rule' GenerateTypelist(typelist(Head, Tail)):
        GenerateType(Head)
        (|
            where(Tail -> nil)
        ||
            OutputWrite(", ")
            GenerateTypelist(Tail)
        |)

'action' GenerateOptionalAlias(OPTIONALID)

    'rule' GenerateOptionalAlias(nil):
        -- do nothing

    'rule' GenerateOptionalAlias(id(Id)):
        ResolveIdName(Id -> AliasName)
        OutputWriteI(" named ", AliasName, "")

'action' GenerateOptionalThrows(OPTIONALID)

    'rule' GenerateOptionalThrows(nil):
        -- do nothing

    'rule' GenerateOptionalThrows(id(Id)):
        ResolveIdName(Id -> ThrowsName)
        OutputWriteI(" throws ", ThrowsName, "")

'action' GenerateReturns(TYPE)

    'rule' GenerateReturns(Type):
        OutputWrite(" returns ")
        GenerateType(Type)

'action' GenerateJavaType(TYPE)

    'rule' GenerateJavaType(byte):
    	OutputWrite("/* jbyte */ ")
        OutputWrite("JavaObject")

    'rule' GenerateJavaType(short):
        OutputWrite("/* jshort */ ")
        OutputWrite("JavaObject")

    'rule' GenerateJavaType(int):
        OutputWrite("/* jint */ ")
        OutputWrite("JavaObject")

    'rule' GenerateJavaType(long):
        OutputWrite("/* jlong */ ")
        OutputWrite("JavaObject")

    'rule' GenerateJavaType(float):
        OutputWrite("/* jfloat */ ")
        OutputWrite("JavaObject")

    'rule' GenerateJavaType(double):
        OutputWrite("/* jdouble */ ")
        OutputWrite("JavaObject")

    'rule' GenerateJavaType(boolean):
        OutputWrite("/* jboolean */ ")
        OutputWrite("JavaObject")

    'rule' GenerateJavaType(char):
        OutputWrite("/* jchar */ ")
        OutputWrite("JavaObject")

    'rule' GenerateJavaType(string):
        OutputWrite("/* jstring */ ")
        OutputWrite("JavaObject")
        
    'rule' GenerateJavaType(named(_, Id, Parameters)):
    	OutputWrite("/* ")
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")
        (|
            where(Parameters -> nil)
        ||
            OutputWrite("<")
            GenerateTypeList(Parameters)
            OutputWrite(">")
        |)
    	OutputWrite(" */ JavaObject")

    'rule' GenerateJavaType(template(_, Id, Parameters)):
    	OutputWrite("/* ")
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")
        (|
            where(Parameters -> nil)
        ||
            OutputWrite("<")
            GenerateTypeList(Parameters)
            OutputWrite(">")
        |)
    	OutputWrite(" */ JavaObject")
		
    'rule' GenerateJavaType(placeholder(_, Id)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")

    -- output java array
    'rule' GenerateJavaType(jarray(_, Type, Dimension)):
    	OutputWrite("/* ")
        GenerateJArray(Type, Dimension)
        OutputWrite(" */ List")

    'rule' GenerateJavaType(nil):
    	OutputWrite("nothing")

'action' GenerateType(TYPE)

    'rule' GenerateType(byte):
        OutputWrite("Number")

    'rule' GenerateType(short):
        OutputWrite("Number")

    'rule' GenerateType(int):
        OutputWrite("Number")

    'rule' GenerateType(long):
        OutputWrite("Number")

    'rule' GenerateType(float):
        OutputWrite("Number")

    'rule' GenerateType(double):
        OutputWrite("Number")

    'rule' GenerateType(boolean):
        OutputWrite("Boolean")

    'rule' GenerateType(char):
        OutputWrite("Number")

    'rule' GenerateType(string):
        OutputWrite("String")

    'rule' GenerateType(named(_, Id, Parameters)):
    	OutputWrite("/* ")
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")
        (|
            where(Parameters -> nil)
        ||
            OutputWrite("<")
            GenerateTypeList(Parameters)
            OutputWrite(">")
        |)
    	OutputWrite(" */ JavaObject")

    'rule' GenerateType(template(_, Id, Parameters)):
    	OutputWrite("/* ")
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")
        (|
            where(Parameters -> nil)
        ||
            OutputWrite("<")
            GenerateTypeList(Parameters)
            OutputWrite(">")
        |)
    	OutputWrite(" */ JavaObject")
		
    'rule' GenerateType(placeholder(_, Id)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")

    -- output java array
    'rule' GenerateType(jarray(_, Type, Dimension)):
    	OutputWrite("/* ")
        GenerateJArray(Type, Dimension)
        OutputWrite(" */ List")

    'rule' GenerateType(nil):
    	OutputWrite("nothing")
    	
    'rule' GenerateType(Type):
    	print(Type)

'action' OutputJavaTypeCode(TYPE)

    'rule' OutputJavaTypeCode(byte):
        OutputWrite("B")

    'rule' OutputJavaTypeCode(short):
        OutputWrite("S")

    'rule' OutputJavaTypeCode(int):
        OutputWrite("I")

    'rule' OutputJavaTypeCode(long):
        OutputWrite("J")

    'rule' OutputJavaTypeCode(float):
        OutputWrite("F")

    'rule' OutputJavaTypeCode(double):
        OutputWrite("D")

    'rule' OutputJavaTypeCode(boolean):
        OutputWrite("Z")

    'rule' OutputJavaTypeCode(char):
        OutputWrite("C")

    'rule' OutputJavaTypeCode(string):
        OutputWrite("Ljava/lang/String;")

    'rule' OutputJavaTypeCode(named(_, Id, _)):
		ResolveIdQualifiedName(Id -> Name)
		JavaQualifiedNameToClassPath(Name -> ClassPath)
		OutputWriteI("L", ClassPath, ";")

    'rule' OutputJavaTypeCode(template(_, Id, _)):
		ResolveIdQualifiedName(Id -> Name)
		JavaQualifiedNameToClassPath(Name -> ClassPath)
		OutputWriteI("L", ClassPath, ";")

    'rule' OutputJavaTypeCode(placeholder(_, Id)):
		print("skip generic placeholder")

    'rule' OutputJavaTypeCode(jarray(_, Type, Dimension)):
        OutputJavaArrayTypeCode(Type, Dimension)

    'rule' OutputJavaTypeCode(nil):

'action' OutputJavaArrayTypeCode(TYPE, INT)

    -- output java array
    'rule' OutputJavaArrayTypeCode(Type, Dimension):
        OutputWrite("[")
        (|
            ne(Dimension, 1)
            OutputJavaArrayTypeCode(Type, Dimension - 1)
        ||
            OutputJavaTypeCode(Type)
        |)

'action' GenerateTypeList(TYPELIST)

    'rule' GenerateTypeList(typelist(Head, Tail)):
        GenerateType(Head)
        (|
            where(Tail -> nil)
        ||
            OutputWrite(", ")
            GenerateTypeList(Tail)
        |)

'action' GenerateJArray(TYPE, INT)

    'rule' GenerateJArray(Type, Dim):
        OutputWrite("List of ")
        (|
            ne(Dim, 1)
            GenerateJArray(Type, Dim - 1)
        ||
            GenerateType(Type)
        |)


'action' GenerateValue(EXPRESSION)

    'rule' GenerateValue(true(_)):
        OutputWrite("true")

    'rule' GenerateValue(false(_)):
        OutputWrite("false")

    'rule' GenerateValue(real(_, Value)):
        OutputWriteD("", Value, "")

    'rule' GenerateValue(integer(_, Value)):
        OutputWriteN("", Value, "")

    'rule' GenerateValue(string(_, Value)):
        OutputWriteS("\"", Value, "\"")


'action' GenerateModifiers(MODIFIER)

    'rule' GenerateModifiers(classmodifiers(Access, StrictFP, Inherit, Modify, Instance)):
        GenerateModifiers(Access)
        GenerateModifiers(StrictFP)
        GenerateModifiers(Inherit)
        GenerateModifiers(Modify)
        GenerateModifiers(Instance)

    'rule' GenerateModifiers(interfacemethodmodifiers(Modifier)):
        GenerateModifiers(Modifier)

    'rule' GenerateModifiers(variablemodifiers(Access, Transient, Modify, Instance)):
        GenerateModifiers(Access)
        GenerateModifiers(Transient)
        GenerateModifiers(Modify)
        GenerateModifiers(Instance)

    'rule' GenerateModifiers(methodmodifiers(Access, Sync, Native, StrictFP, Inherit, Instance)):
        GenerateModifiers(Access)
        GenerateModifiers(Sync)
        GenerateModifiers(Native)
        GenerateModifiers(StrictFP)
        GenerateModifiers(Inherit)
        GenerateModifiers(Instance)

    'rule' GenerateModifiers(constructormodifiers(Access)):
        GenerateModifiers(Access)

    'rule' GenerateModifiers(protected):
        OutputWrite("protected ")

    'rule' GenerateModifiers(synchronized):
        OutputWrite("synchronized ")

    'rule' GenerateModifiers(native):
        OutputWrite("native ")

    'rule' GenerateModifiers(strictfp):
        OutputWrite("strictfp ")

    'rule' GenerateModifiers(abstract):
        OutputWrite("abstract ")

    'rule' GenerateModifiers(final):
        OutputWrite("final ")

    'rule' GenerateModifiers(class):
        OutputWrite("class ")

    'rule' GenerateModifiers(volatile):
        OutputWrite("volatile ")

    'rule' GenerateModifiers(transient):
        OutputWrite("transient ")

    'rule' GenerateModifiers(default):
        OutputWrite("default ")

    'rule' GenerateModifiers(public):

    'rule' GenerateModifiers(inferred):

--------------------------------------------------------------------------

'action' TypeToQualifiedName(TYPE -> NAME)

    'rule' TypeToQualifiedName(named(_, Id, _) -> Name)
		ResolveIdQualifiedName(Id -> Name)
		
    'rule' TypeToQualifiedName(template(_, Id, _) -> Name)
		ResolveIdQualifiedName(Id -> Name)

'action' TypeToUnqualifiedName(TYPE -> NAME)

    'rule' TypeToUnqualifiedName(named(_, Id, _) -> Name)
		ResolveIdName(Id -> Name)

    'rule' TypeToUnqualifiedName(template(_, Id, _) -> Name)
		ResolveIdName(Id -> Name)

--------------------------------------------------------------------------------
-- Defined in check.g
'action' QueryId(ID -> MEANING)
'action' QueryPackageOfId(ID -> ID)

'condition' QuerySymbolId(ID -> SYMBOLINFO)
'condition' QueryPackageId(ID -> PACKAGEINFO)

-- Defined in bind.g
'action' ResolveIdName(ID -> NAME)
'action' ResolveIdQualifiedName(ID -> NAME)
--------------------------------------------------------------------------------
