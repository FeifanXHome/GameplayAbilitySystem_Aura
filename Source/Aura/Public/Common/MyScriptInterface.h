// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScriptInterface.h: Script interface definitions.
=============================================================================*/

#pragma once

#include "UObject/UObjectGlobals.h"
#include "Templates/Casts.h"
#include "Templates/UnrealTemplate.h"

#include <type_traits>

/**
 * MyFScriptInterface
 *
 * This utility class stores the FProperty data for a native interface property.  ObjectPointer and InterfacePointer point to different locations in the same UObject.
 */
class MyFScriptInterface
{
private:
	/**
	 * A pointer to a UObject that implements a native interface.
	 */
	UObject*	ObjectPointer = nullptr;

	/**
	 * Pointer to the location of the interface object within the UObject referenced by ObjectPointer.
	 */
	void*		InterfacePointer = nullptr;

	/**
	 * Serialize ScriptInterface
	 */
	FArchive& Serialize(FArchive& Ar, class UClass* InterfaceType);

public:
	/**
	 * Default constructor
	 */
	MyFScriptInterface() = default;

	/**
	 * Construction from object and interface
	 */
	MyFScriptInterface( UObject* InObjectPointer, void* InInterfacePointer )
	: ObjectPointer(InObjectPointer), InterfacePointer(InInterfacePointer)
	{}

	/**
	 * Copyable
	 */
	MyFScriptInterface(const MyFScriptInterface&) = default;
	MyFScriptInterface& operator=(const MyFScriptInterface&) = default;

	/**
	 * Returns the ObjectPointer contained by this MyFScriptInterface
	 */
	FORCEINLINE UObject* GetObject() const
	{
		return ObjectPointer;
	}

	/**
	 * Returns the ObjectPointer contained by this MyFScriptInterface
	 */
	FORCEINLINE UObject*& GetObjectRef()
	{
		return ObjectPointer;
	}

	/**
	 * Returns the pointer to the interface
	 */
	FORCEINLINE void* GetInterface() const
	{
		// only allow access to InterfacePointer if we have a valid ObjectPointer.  This is necessary because the garbage collector will set ObjectPointer to NULL
		// without using the accessor methods
		return ObjectPointer ? InterfacePointer : nullptr;
	}

	/**
	 * Sets the value of the ObjectPointer for this MyFScriptInterface
	 */
	FORCEINLINE void SetObject( UObject* InObjectPointer )
	{
		ObjectPointer = InObjectPointer;
		if ( ObjectPointer == nullptr )
		{
			SetInterface(nullptr);
		}
	}

	/**
	 * Sets the value of the InterfacePointer for this MyFScriptInterface
	 */
	FORCEINLINE void SetInterface( void* InInterfacePointer )
	{
		InterfacePointer = InInterfacePointer;
	}

	/**
	 * Comparison operator, taking a reference to another MyFScriptInterface
	 */
	FORCEINLINE bool operator==( const MyFScriptInterface& Other ) const
	{
		return GetInterface() == Other.GetInterface() && ObjectPointer == Other.GetObject();
	}
	FORCEINLINE bool operator!=( const MyFScriptInterface& Other ) const
	{
		return GetInterface() != Other.GetInterface() || ObjectPointer != Other.GetObject();
	}

	void AddReferencedObjects(FReferenceCollector& Collector)
	{
		Collector.AddReferencedObject(ObjectPointer);
	}

	friend inline uint32 GetTypeHash(const MyFScriptInterface& Instance)
	{
		return GetTypeHash(Instance.InterfacePointer);
	}
};



template<> struct TIsPODType<class MyFScriptInterface> { enum { Value = true }; };
template<> struct TIsZeroConstructType<class MyFScriptInterface> { enum { Value = true }; };

/**
 * Templated version of MyFScriptInterface, which provides accessors and operators for referencing the interface portion of a UObject that implements a native interface.
 */
template <typename InterfaceType>
class MyTScriptInterface : public MyFScriptInterface
{
public:
	/**
	 * Default constructor
	 */
	MyTScriptInterface() = default;

	/**
	 * Construction from nullptr
	 */
	MyTScriptInterface(TYPE_OF_NULLPTR) {}

	/**
	 * Assignment from an object type that implements the InterfaceType native interface class
	 */
	template <
		typename U,
		decltype(ImplicitConv<UObject*>(std::declval<U>()))* = nullptr
	>
	FORCENOINLINE MyTScriptInterface(U&& Source)
	{
		UObject* SourceObject = ImplicitConv<UObject*>(Source);
		SetObject(SourceObject);

		InterfaceType* SourceInterface = Cast<InterfaceType>(SourceObject);
		SetInterface(SourceInterface);
	}

	/**
	 * Assignment from an object type that implements the InterfaceType native interface class
	 */
	template <typename ObjectType>
	MyTScriptInterface(TObjectPtr<ObjectType> SourceObject)
	{
		SetObject(SourceObject);

		InterfaceType* SourceInterface = Cast<InterfaceType>(ToRawPtr(SourceObject));
		SetInterface(SourceInterface);
	}

	/**
	 * Copyable
	 */
	MyTScriptInterface(const MyTScriptInterface&) = default;
	MyTScriptInterface& operator=(const MyTScriptInterface&) = default;

	/**
	 * Assignment from nullptr
	 */
	MyTScriptInterface& operator=(TYPE_OF_NULLPTR)
	{
		*this = MyTScriptInterface();
		return *this;
	}

	/**
	 * Assignment from an object type that implements the InterfaceType native interface class
	 */
	template <
		typename U,
		decltype(ImplicitConv<UObject*>(std::declval<U>()))* = nullptr
	>
	MyTScriptInterface& operator=(U&& Source)
	{
		*this = MyTScriptInterface(Source);
		return *this;
	}

	/**
	 * Assignment from an object type that implements the InterfaceType native interface class
	 */
	template <typename ObjectType>
	MyTScriptInterface& operator=(TObjectPtr<ObjectType> SourceObject)
	{
		*this = MyTScriptInterface(SourceObject);
		return *this;
	}

	/**
	 * Comparison operator, taking a pointer to InterfaceType
	 */
	template <typename OtherInterface, typename = decltype(ImplicitConv<InterfaceType*>((OtherInterface*)nullptr))>
	FORCEINLINE bool operator==( const OtherInterface* Other ) const
	{
		return GetInterface() == Other;
	}
	template <typename OtherInterface, typename = decltype(ImplicitConv<InterfaceType*>((OtherInterface*)nullptr))>
	FORCEINLINE bool operator!=( const OtherInterface* Other ) const
	{
		return GetInterface() != Other;
	}

	/**
	 * Comparison operator, taking a reference to another MyTScriptInterface
	 */
	FORCEINLINE bool operator==( const MyTScriptInterface& Other ) const
	{
		return GetInterface() == Other.GetInterface() && GetObject() == Other.GetObject();
	}
	FORCEINLINE bool operator!=( const MyTScriptInterface& Other ) const
	{
		return GetInterface() != Other.GetInterface() || GetObject() != Other.GetObject();
	}

	/**
	 * Comparison operator, taking a nullptr
	 */
	FORCEINLINE bool operator==(TYPE_OF_NULLPTR) const
	{
		return GetInterface() == nullptr;
	}
	FORCEINLINE bool operator!=(TYPE_OF_NULLPTR) const
	{
		return GetInterface() != nullptr;
	}

	/**
	 * Member access operator.  Provides transparent access to the interface pointer contained by this MyTScriptInterface
	 */
	FORCEINLINE InterfaceType* operator->() const
	{
		return GetInterface();
	}

	/**
	 * Dereference operator.  Provides transparent access to the interface pointer contained by this MyTScriptInterface
	 *
	 * @return	a reference (of type InterfaceType) to the object pointed to by InterfacePointer
	 */
	FORCEINLINE InterfaceType& operator*() const
	{
		return *GetInterface();
	}

	/**
	 * Returns the pointer to the interface
	 */
	FORCEINLINE InterfaceType* GetInterface() const
	{
		return (InterfaceType*)MyFScriptInterface::GetInterface();
	}

	/**
	 * Sets the value of the InterfacePointer for this MyTScriptInterface
	 */
	FORCEINLINE void SetInterface(InterfaceType* InInterfacePointer)
	{
		MyFScriptInterface::SetInterface(InInterfacePointer);
	}

	/**
	 * Boolean operator.  Provides transparent access to the interface pointer contained by this MyTScriptInterface.
	 *
	 * @return	true if InterfacePointer is non-NULL.
	 */
	FORCEINLINE explicit operator bool() const
	{
		return GetInterface() != nullptr;
	}

	friend FArchive& operator<<( FArchive& Ar, MyTScriptInterface& Interface )
	{
		return Interface.Serialize(Ar, InterfaceType::UClassType::StaticClass());
	}
};

#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#include "CoreMinimal.h"
#endif
