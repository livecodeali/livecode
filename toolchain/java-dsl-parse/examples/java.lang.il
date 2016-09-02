foreign package java.lang named com.livecode.wrapped.java.lang

use java.lang.reflect.GenericDeclaration
use java.lang.reflect.Type
use java.lang.reflect.TypeVariable

class Object
	constructor Object() returns Object
	
	final method getClass() returns Class
	method hashCode() returns int
	method equals(obj as Object) returns boolean
	protected method clone() returns Object
	method toString() returns String
	final method notify() returns nothing
	final method notifyAll() returns nothing
	final method wait(timeout as long) returns nothing
	final method wait(timeout as long, nanos as int) returns nothing
	final method wait() returns nothing
	protected method finalize() returns nothing
	
end class

-- TODO: final
class Class<T> inherits Object implements GenericDeclaration, Type
	method asSubclass(clazz as Class<U>) returns Class<U>
	method cast(obj as Object) returns T
	method desiredAssertionStatus() returns boolean
	class method forName(className as String) returns Class
	class method forName(name as String, initialize as boolean, loader as ClassLoader) named forNameOptions returns ClassLoader
	method getCanonicalName() returns String
	method getClasses() returns Class[]
	method getClassLoader() returns ClassLoader
	method getComponentType() returns Class
	--method getConstructor
	method getConstructors() returns Constructor[]
	method getDeclaredClasses() returns Class[]
	--method getDeclaredConstructor
	method getDeclaredConstructors() returns Constructor[]
	method getDeclaredField(name as String) returns Field
	method getDeclaredFields() returns Field[]
	--method getDeclaredMethod
	method getDeclaredMethods() returns Method[]
	method getDeclaringClass() returns Class
	method getEnclosingClass() returns Class
	method getEnclosingConstructor() returns Constructor
	method getEnclosingMethod() returns Method
	method getEnumConstants() returns T[]
	method getField(name as String) returns Field
	method getFields() returns Field[]
	method getGenericInterfaces() returns Type[]
	method getGenericSuperclass() returns Type
	method getInterfaces() returns Class[]
	--method getMethod
	method getMethods() returns Method[]
	method getModifiers() returns int
	method getName() returns String
	method getPackage() returns Package
	method getSimpleName() returns String
	method getSuperclass() returns Class<T>
	method getTypeParameters() returns TypeVariable<Class<T>>[]
	method isAnonymousClass() returns boolean
	method isArray() returns boolean
	method isAssignableFrom(cls as Class<U>) returns boolean
	method isEnum() returns boolean
	method isInstance(obj as Object) returns boolean
	method isInterface() returns boolean
	method isLocalClass() returns boolean
	method isMemberClass() returns boolean
	method isPrimitive() returns boolean
	method isSynthetic() returns boolean
	method newInstance() returns T
	method toString() returns String
end class

class ClassLoader
	protected constructor ClassLoader()
	protected constructor ClassLoader(parent as ClassLoader) named ClassLoaderChild
	
	method clearAssertionStatus() returns void
	protected Class<?>	defineClass(String name, byte[] b, int off, int len)
	protected Class<?>	defineClass(String name, byte[] b, int off, int len, ProtectionDomain protectionDomain)
	protected Class<?>	defineClass(String name, ByteBuffer b, ProtectionDomain protectionDomain)
	protected Package	definePackage(String name, String specTitle, String specVersion, String specVendor, String implTitle, String implVersion, String implVendor, URL sealBase)
	protected method findClass(name as String) returns Class<?>
	protected method findLibrary(libname as String) returns String
	protected method findLoadedClass(name as String) returns Class<?>
	protected method findResource(name as String) returns URL
	protected method findResources(name as String) returns Enumeration<URL>
	protected method findSystemClass(name as String) returns Class<?>
	protected method getClassLoadingLock(className as String) returns Object
	protected method getPackage(name as String) returns Package
	method getPackages() returns protected Package[]
	method getParent() returns ClassLoader
	method getResource(name as String) returns URL
	method getResourceAsStream(name as String) returns InputStream
	method getResources(name as String) returns Enumeration<URL>
	method getSystemClassLoader() returns static ClassLoader
	static method getSystemResource(name as String) returns URL
	static method getSystemResourceAsStream(name as String) returns InputStream
	static method getSystemResources(name as String) returns Enumeration<URL>
	method loadClass(name as String) returns Class<?>
	protected method loadClass(name as String, resolve as boolean) returns Class<?>
	method registerAsParallelCapable() returns protected static boolean
	protected void	resolveClass(Class<?> c)
	method setClassAssertionStatus(className as String, enabled as boolean) returns void
	method setDefaultAssertionStatus(enabled as boolean) returns void
	method setPackageAssertionStatus(packageName as String, enabled as boolean) returns void
	protected void	setSigners(Class<?> c, Object[] signers)
end class
