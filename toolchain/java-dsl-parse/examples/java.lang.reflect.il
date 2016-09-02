foreign package java.lang.reflect named com.livecode.wrapped.java.reflect

interface Type
	default method getTypeName() returns String
end interface

interface GenericArrayType inherits Type
	method getGenericComponentType() returns Type
end interface

interface TypeVariable<D> inherits Type
	method getBounds() returns Type[]
	method getGenericDeclaration() returns D
	method getName() returns String
end interface

interface GenericDeclaration
	method getTypeParameters() returns TypeVariable[]
end interface

interface Member
	constant DECLARED as int
	constant PUBLIC as int
	
	method getDeclaringClass() returns Class
end interface

class Constructor
	method equals(obj as Object) returns boolean
	method getDeclaringClass() returns Class<T>
	method getExceptionTypes() returns Class<?>[]
	method getGenericExceptionTypes() returns Type[]
	method getGenericParameterTypes() returns Type[]
	method getModifiers() returns int
	method getName() returns String
	method getParameterTypes() returns Class<?>[]
	method getTypeParameters() returns TypeVariable<Constructor<T>>[]
	method hashCode() returns int
	method isSynthetic() returns boolean
	method isVarArgs() returns boolean
	method newInstance(Object... initargs) returns T
	method toGenericString() returns String
	method toString() returns String
end class

class Field inherits AccessibleObject implements Member
	method equals(obj as Object) returns boolean
	method get(obj as Object) returns Object
	method getBoolean(obj as Object) returns boolean
	method getByte(obj as Object) returns byte
	method getChar(obj as Object) returns char
	method getDeclaringClass() returns Class<?>
	method getDouble(obj as Object) returns double
	method getFloat(obj as Object) returns float
	method getGenericType() returns Type
	method getInt(obj as Object) returns int
	method getLong(obj as Object) returns long
	method getModifiers() returns int
	method getName() returns String
	method getShort(obj as Object) returns short
	method getType() returns Class<?>
	method hashCode() returns int
	method isEnumConstant() returns boolean
	method isSynthetic() returns boolean
	method set(obj as Object, value as Object) returns void
	method setBoolean(obj as Object, z as boolean) returns void
	method setByte(obj as Object, b as byte) returns void
	method setChar(obj as Object, c as char) returns void
	method setDouble(obj as Object, d as double) returns void
	method setFloat(obj as Object, f as float) returns void
	method setInt(obj as Object, i as int) returns void
	method setLong(obj as Object, l as long) returns void
	method setShort(obj as Object, s as short) returns void
	method toGenericString() returns String
	method toString() returns String
end class

class Method
	method equals(obj as Object) returns boolean
	method getDeclaringClass() returns Class<?>
	method getDefaultValue() returns Object
	method getExceptionTypes() returns Class<?>[]
	method getGenericExceptionTypes() returns Type[]
	method getGenericParameterTypes() returns Type[]
	method getGenericReturnType() returns Type
	method getModifiers() returns int
	method getName() returns String
	method getParameterTypes() returns Class<?>[]
	method getReturnType() returns Class<?>
	method getTypeParameters() returns TypeVariable<Method>[]
	method hashCode() returns int
	method invoke(Object obj, Object... args) as Object
	method isBridge() returns boolean
	method isSynthetic() returns boolean
	method isVarArgs() returns boolean
	method toGenericString() returns String
	method toString() returns String
end class