//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// This is the list of internal properties used in the Chakra engine.
// They become nameless compile time known PropertyRecords, stored as static
// fields on the InternalPropertyRecords class.

// NOTE: When new property is added here, please evaluate if the property's value needs to be restored to nullptr
// when it gets reset to undefined inside DynamicObject::ResetObject()

INTERNALPROPERTY(TypeOfPrototypeObjectInlined)     // Used to store the type of the prototype object in the prototype objects slots. Only DynamicTypes having TypeIds_Object are saved in this slot.
// Used to store the type of the prototype object in the prototype objects slots. Everything else (except ExternalType) are stored in this slot as Dictionary.
// Key in the Dictionary is combination of Type and TypeId and value is dynamicType object.
INTERNALPROPERTY(TypeOfPrototypeObjectDictionary)  
INTERNALPROPERTY(NonExtensibleType)               // Used to store shared non-extensible type in PathTypeHandler::propertySuccessors map.
INTERNALPROPERTY(SealedType)                      // Used to store shared sealed type in PathTypeHandler::propertySuccessors map.
INTERNALPROPERTY(FrozenType)                      // Used to store shared frozen type in PathTypeHandler::propertySuccessors map.
INTERNALPROPERTY(StackTrace)                      // Stack trace object for Error.stack generation
INTERNALPROPERTY(StackTraceCache)                 // Cache of Error.stack string
INTERNALPROPERTY(WeakMapKeyMap)                   // WeakMap data stored on WeakMap key objects
INTERNALPROPERTY(HiddenObject)                    // Used to store internal slot data for JS library code (Intl as an example will use this)
INTERNALPROPERTY(CachedUCollator)                 // Used to store cached UCollator objects for Intl.Collator
INTERNALPROPERTY(CachedUNumberFormat)             // Used to store cached UNumberFormat objects for Intl.NumberFormat and Intl.PluralRules
INTERNALPROPERTY(CachedUDateFormat)               // Used to store cached UDateFormat objects for Intl.DateTimeFormat
INTERNALPROPERTY(CachedUPluralRules)              // Used to store cached UPluralRules objects for Intl.PluralRules
INTERNALPROPERTY(RevocableProxy)                  // Internal slot for [[RevokableProxy]] for revocable proxy in ES6
INTERNALPROPERTY(MutationBp)                      // Used to store strong reference to the mutation breakpoint object
INTERNALPROPERTY(EmbedderData)                    // Holds embedder data here.

#undef INTERNALPROPERTY
