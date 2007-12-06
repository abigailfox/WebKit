/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007 Apple Inc. All rights reserved.
 *  Copyright (C) 2003 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "array_object.h"
#include "array_object.lut.h"

#include "error_object.h"
#include "lookup.h"
#include "operations.h"
#include <stdio.h>
#include <wtf/Assertions.h>
#include <wtf/HashSet.h>

namespace KJS {

// ------------------------------ ArrayPrototype ----------------------------

const ClassInfo ArrayPrototype::info = {"Array", &ArrayInstance::info, &arrayTable};

/* Source for array_object.lut.h
@begin arrayTable 16
  toString       &ArrayProtoFuncToString::create       DontEnum|Function 0
  toLocaleString &ArrayProtoFuncToLocaleString::create DontEnum|Function 0
  concat         &ArrayProtoFuncConcat::create         DontEnum|Function 1
  join           &ArrayProtoFuncJoin::create           DontEnum|Function 1
  pop            &ArrayProtoFuncPop::create            DontEnum|Function 0
  push           &ArrayProtoFuncPush::create           DontEnum|Function 1
  reverse        &ArrayProtoFuncReverse::create        DontEnum|Function 0
  shift          &ArrayProtoFuncShift::create          DontEnum|Function 0
  slice          &ArrayProtoFuncSlice::create          DontEnum|Function 2
  sort           &ArrayProtoFuncSort::create           DontEnum|Function 1
  splice         &ArrayProtoFuncSplice::create         DontEnum|Function 2
  unshift        &ArrayProtoFuncUnShift::create        DontEnum|Function 1
  every          &ArrayProtoFuncEvery::create          DontEnum|Function 1
  forEach        &ArrayProtoFuncForEach::create        DontEnum|Function 1
  some           &ArrayProtoFuncSome::create           DontEnum|Function 1
  indexOf        &ArrayProtoFuncIndexOf::create        DontEnum|Function 1
  lastIndexOf    &ArrayProtoFuncLastIndexOf::create    DontEnum|Function 1
  filter         &ArrayProtoFuncFilter::create         DontEnum|Function 1
  map            &ArrayProtoFuncMap::create            DontEnum|Function 1
@end
*/

// ECMA 15.4.4
ArrayPrototype::ArrayPrototype(ExecState*, ObjectPrototype* objProto)
  : ArrayInstance(objProto, 0)
{
}

bool ArrayPrototype::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticFunctionSlot<ArrayInstance>(exec, &arrayTable, this, propertyName, slot);
}


// ------------------------------ Array Functions ----------------------------

// Helper function
static JSValue* getProperty(ExecState* exec, JSObject* obj, unsigned index)
{
    PropertySlot slot;
    if (!obj->getPropertySlot(exec, index, slot))
        return 0;
    return slot.getValue(exec, obj, index);
}

JSValue* ArrayProtoFuncToString::callAsFunction(ExecState* exec, JSObject* thisObj, const List&)
{
    if (!thisObj->inherits(&ArrayInstance::info))
        return throwError(exec, TypeError);

    static HashSet<JSObject*> visitedElems;
    static const UString* empty = new UString("");
    static const UString* comma = new UString(",");
    bool alreadyVisited = !visitedElems.add(thisObj).second;
    if (alreadyVisited)
        return jsString(*empty);
    UString separator = *comma;
    UString str = *empty;

    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    for (unsigned int k = 0; k < length; k++) {
        if (k >= 1)
            str += separator;
        if (str.isNull()) {
            JSObject *error = Error::create(exec, GeneralError, "Out of memory");
            exec->setException(error);
            break;
        }

        JSValue* element = thisObj->get(exec, k);
        if (element->isUndefinedOrNull())
            continue;

        str += element->toString(exec);

        if (str.isNull()) {
            JSObject *error = Error::create(exec, GeneralError, "Out of memory");
            exec->setException(error);
        }

        if (exec->hadException())
            break;
    }
    visitedElems.remove(thisObj);
    return jsString(str);
}

JSValue* ArrayProtoFuncToLocaleString::callAsFunction(ExecState* exec, JSObject* thisObj, const List&)
{
    if (!thisObj->inherits(&ArrayInstance::info))
        return throwError(exec, TypeError);

    static HashSet<JSObject*> visitedElems;
    static const UString* empty = new UString("");
    static const UString* comma = new UString(",");
    bool alreadyVisited = !visitedElems.add(thisObj).second;
    if (alreadyVisited)
        return jsString(*empty);
    UString separator = *comma;
    UString str = *empty;

    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    for (unsigned int k = 0; k < length; k++) {
        if (k >= 1)
            str += separator;
        if (str.isNull()) {
            JSObject *error = Error::create(exec, GeneralError, "Out of memory");
            exec->setException(error);
            break;
        }

        JSValue* element = thisObj->get(exec, k);
        if (element->isUndefinedOrNull())
            continue;

        bool fallback = false;
        JSObject* o = element->toObject(exec);
        JSValue* conversionFunction = o->get(exec, exec->propertyNames().toLocaleString);
        if (conversionFunction->isObject() && static_cast<JSObject*>(conversionFunction)->implementsCall()) {
            List args;
            str += static_cast<JSObject*>(conversionFunction)->call(exec, o, args)->toString(exec);
        } else {
            // try toString() fallback
            fallback = true;
        }

        if (fallback)
            str += element->toString(exec);

        if (str.isNull()) {
            JSObject *error = Error::create(exec, GeneralError, "Out of memory");
            exec->setException(error);
        }

        if (exec->hadException())
            break;
    }
    visitedElems.remove(thisObj);
    return jsString(str);
}

JSValue* ArrayProtoFuncJoin::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    static HashSet<JSObject*> visitedElems;
    static const UString* empty = new UString("");
    static const UString* comma = new UString(",");
    bool alreadyVisited = !visitedElems.add(thisObj).second;
    if (alreadyVisited)
        return jsString(*empty);
    UString separator = *comma;
    UString str = *empty;

    if (!args[0]->isUndefined())
        separator = args[0]->toString(exec);

    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    for (unsigned int k = 0; k < length; k++) {
        if (k >= 1)
            str += separator;
        if (str.isNull()) {
            JSObject *error = Error::create(exec, GeneralError, "Out of memory");
            exec->setException(error);
            break;
        }

        JSValue* element = thisObj->get(exec, k);
        if (element->isUndefinedOrNull())
            continue;

        str += element->toString(exec);

        if (str.isNull()) {
            JSObject *error = Error::create(exec, GeneralError, "Out of memory");
            exec->setException(error);
        }

        if (exec->hadException())
            break;
    }
    visitedElems.remove(thisObj);
    return jsString(str);
}

JSValue* ArrayProtoFuncConcat::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    JSObject* arr = static_cast<JSObject*>(exec->lexicalGlobalObject()->arrayConstructor()->construct(exec, List::empty()));
    int n = 0;
    JSValue *curArg = thisObj;
    JSObject *curObj = static_cast<JSObject *>(thisObj);
    List::const_iterator it = args.begin();
    List::const_iterator end = args.end();
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    for (;;) {
        if (curArg->isObject() &&
            curObj->inherits(&ArrayInstance::info)) {
            unsigned int k = 0;
            // Older versions tried to optimize out getting the length of thisObj
            // by checking for n != 0, but that doesn't work if thisObj is an empty array.
            length = curObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
            while (k < length) {
                if (JSValue *v = getProperty(exec, curObj, k))
                    arr->put(exec, n, v);
                n++;
                k++;
            }
        } else {
            arr->put(exec, n, curArg);
            n++;
        }
        if (it == end)
            break;
        curArg = *it;
        curObj = static_cast<JSObject*>(curArg); // may be 0
        ++it;
    }
    arr->put(exec, exec->propertyNames().length, jsNumber(n), DontEnum | DontDelete);
    return arr;
}

JSValue* ArrayProtoFuncPop::callAsFunction(ExecState* exec, JSObject* thisObj, const List&)
{
    JSValue* result = 0;
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    if (length == 0) {
      thisObj->put(exec, exec->propertyNames().length, jsNumber(length), DontEnum | DontDelete);
      result = jsUndefined();
    } else {
      result = thisObj->get(exec, length - 1);
      thisObj->put(exec, exec->propertyNames().length, jsNumber(length - 1), DontEnum | DontDelete);
    }
    return result;
}

JSValue* ArrayProtoFuncPush::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    for (unsigned int n = 0; n < args.size(); n++)
        thisObj->put(exec, length + n, args[n]);
    length += args.size();
    thisObj->put(exec, exec->propertyNames().length, jsNumber(length), DontEnum | DontDelete);
    return jsNumber(length);
}

JSValue* ArrayProtoFuncReverse::callAsFunction(ExecState* exec, JSObject* thisObj, const List&)
{
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    unsigned int middle = length / 2;

    for (unsigned int k = 0; k < middle; k++) {
        unsigned lk1 = length - k - 1;
        JSValue *obj2 = getProperty(exec, thisObj, lk1);
        JSValue *obj = getProperty(exec, thisObj, k);

        if (obj2) 
            thisObj->put(exec, k, obj2);
        else
            thisObj->deleteProperty(exec, k);

        if (obj)
            thisObj->put(exec, lk1, obj);
        else
            thisObj->deleteProperty(exec, lk1);
    }
    return thisObj;
}

JSValue* ArrayProtoFuncShift::callAsFunction(ExecState* exec, JSObject* thisObj, const List&)
{
    JSValue* result = 0;

    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    if (length == 0) {
      thisObj->put(exec, exec->propertyNames().length, jsNumber(length), DontEnum | DontDelete);
      result = jsUndefined();
    } else {
      result = thisObj->get(exec, 0);
      for(unsigned int k = 1; k < length; k++) {
        if (JSValue *obj = getProperty(exec, thisObj, k))
          thisObj->put(exec, k-1, obj);
        else
          thisObj->deleteProperty(exec, k-1);
      }
      thisObj->deleteProperty(exec, length - 1);
      thisObj->put(exec, exec->propertyNames().length, jsNumber(length - 1), DontEnum | DontDelete);
    }
    return result;
}

JSValue* ArrayProtoFuncSlice::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    // http://developer.netscape.com/docs/manuals/js/client/jsref/array.htm#1193713 or 15.4.4.10

    // We return a new array
    JSObject *resObj = static_cast<JSObject *>(exec->lexicalGlobalObject()->arrayConstructor()->construct(exec,List::empty()));
    JSValue* result = resObj;
    double begin = args[0]->toInteger(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    if (begin >= 0) {
        if (begin > length)
            begin = length;
    } else {
        begin += length;
        if (begin < 0)
            begin = 0;
    }
    double end;
    if (args[1]->isUndefined())
        end = length;
    else {
        end = args[1]->toInteger(exec);
        if (end < 0) {
            end += length;
            if (end < 0)
                end = 0;
        } else {
            if (end > length)
                end = length;
        }
    }

    int n = 0;
    int b = static_cast<int>(begin);
    int e = static_cast<int>(end);
    for(int k = b; k < e; k++, n++) {
        if (JSValue *v = getProperty(exec, thisObj, k))
            resObj->put(exec, n, v);
    }
    resObj->put(exec, exec->propertyNames().length, jsNumber(n), DontEnum | DontDelete);
    return result;
}

JSValue* ArrayProtoFuncSort::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
#if 0
    printf("KJS Array::Sort length=%d\n", length);
    for ( unsigned int i = 0 ; i<length ; ++i )
      printf("KJS Array::Sort: %d: %s\n", i, thisObj->get(exec, i)->toString(exec).ascii() );
#endif
    JSObject *sortFunction = NULL;
    if (!args[0]->isUndefined())
      {
        sortFunction = args[0]->toObject(exec);
        if (!sortFunction->implementsCall())
          sortFunction = NULL;
      }
    
    if (thisObj->classInfo() == &ArrayInstance::info) {
      if (sortFunction)
        ((ArrayInstance *)thisObj)->sort(exec, sortFunction);
      else
        ((ArrayInstance *)thisObj)->sort(exec);
      return thisObj;
    }

    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);

    if (length == 0) {
      thisObj->put(exec, exec->propertyNames().length, jsNumber(0), DontEnum | DontDelete);
      return thisObj;
    }

    // "Min" sort. Not the fastest, but definitely less code than heapsort
    // or quicksort, and much less swapping than bubblesort/insertionsort.
    for ( unsigned int i = 0 ; i<length-1 ; ++i )
      {
        JSValue *iObj = thisObj->get(exec,i);
        unsigned int themin = i;
        JSValue *minObj = iObj;
        for ( unsigned int j = i+1 ; j<length ; ++j )
          {
            JSValue *jObj = thisObj->get(exec,j);
            double cmp;
            if (jObj->isUndefined()) {
              cmp = 1; // don't check minObj because there's no need to differentiate == (0) from > (1)
            } else if (minObj->isUndefined()) {
              cmp = -1;
            } else if (sortFunction) {
                List l;
                l.append(jObj);
                l.append(minObj);
                cmp = sortFunction->call(exec, exec->dynamicGlobalObject(), l)->toNumber(exec);
            } else {
              cmp = (jObj->toString(exec) < minObj->toString(exec)) ? -1 : 1;
            }
            if ( cmp < 0 )
              {
                themin = j;
                minObj = jObj;
              }
          }
        // Swap themin and i
        if ( themin > i )
          {
            //printf("KJS Array::Sort: swapping %d and %d\n", i, themin );
            thisObj->put( exec, i, minObj );
            thisObj->put( exec, themin, iObj );
          }
    }
#if 0
    printf("KJS Array::Sort -- Resulting array:\n");
    for ( unsigned int i = 0 ; i<length ; ++i )
      printf("KJS Array::Sort: %d: %s\n", i, thisObj->get(exec, i)->toString(exec).ascii() );
#endif
    return thisObj;
}

JSValue* ArrayProtoFuncSplice::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    // 15.4.4.12 - oh boy this is huge
    JSObject *resObj = static_cast<JSObject *>(exec->lexicalGlobalObject()->arrayConstructor()->construct(exec, List::empty()));
    JSValue* result = resObj;
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    int begin = args[0]->toUInt32(exec);
    if ( begin < 0 )
      begin = maxInt( begin + length, 0 );
    else
      begin = minInt( begin, length );
    unsigned int deleteCount = minInt( maxInt( args[1]->toUInt32(exec), 0 ), length - begin );

    //printf( "Splicing from %d, deleteCount=%d \n", begin, deleteCount );
    for(unsigned int k = 0; k < deleteCount; k++) {
      if (JSValue *v = getProperty(exec, thisObj, k+begin))
        resObj->put(exec, k, v);
    }
    resObj->put(exec, exec->propertyNames().length, jsNumber(deleteCount), DontEnum | DontDelete);

    unsigned int additionalArgs = maxInt( args.size() - 2, 0 );
    if ( additionalArgs != deleteCount )
    {
      if ( additionalArgs < deleteCount )
      {
        for ( unsigned int k = begin; k < length - deleteCount; ++k )
        {
          if (JSValue *v = getProperty(exec, thisObj, k+deleteCount))
            thisObj->put(exec, k+additionalArgs, v);
          else
            thisObj->deleteProperty(exec, k+additionalArgs);
        }
        for ( unsigned int k = length ; k > length - deleteCount + additionalArgs; --k )
          thisObj->deleteProperty(exec, k-1);
      }
      else
      {
        for ( unsigned int k = length - deleteCount; (int)k > begin; --k )
        {
          if (JSValue *obj = getProperty(exec, thisObj, k + deleteCount - 1))
            thisObj->put(exec, k + additionalArgs - 1, obj);
          else
            thisObj->deleteProperty(exec, k+additionalArgs-1);
        }
      }
    }
    for ( unsigned int k = 0; k < additionalArgs; ++k )
    {
      thisObj->put(exec, k+begin, args[k+2]);
    }
    thisObj->put(exec, exec->propertyNames().length, jsNumber(length - deleteCount + additionalArgs), DontEnum | DontDelete);
    return result;
}

JSValue* ArrayProtoFuncUnShift::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    // 15.4.4.13
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    unsigned int nrArgs = args.size();
    for ( unsigned int k = length; k > 0; --k )
    {
      if (JSValue *v = getProperty(exec, thisObj, k - 1))
        thisObj->put(exec, k+nrArgs-1, v);
      else
        thisObj->deleteProperty(exec, k+nrArgs-1);
    }
    for ( unsigned int k = 0; k < nrArgs; ++k )
      thisObj->put(exec, k, args[k]);
    JSValue* result = jsNumber(length + nrArgs);
    thisObj->put(exec, exec->propertyNames().length, result, DontEnum | DontDelete);
    return result;
}

JSValue* ArrayProtoFuncFilter::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    JSObject* eachFunction = args[0]->toObject(exec);
    
    if (!eachFunction->implementsCall())
        return throwError(exec, TypeError);
    
    JSObject *applyThis = args[1]->isUndefinedOrNull() ? exec->dynamicGlobalObject() :  args[1]->toObject(exec);
    JSObject *resultArray = static_cast<JSObject*>(exec->lexicalGlobalObject()->arrayConstructor()->construct(exec, List::empty()));

    unsigned filterIndex = 0;
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    for (unsigned k = 0; k < length && !exec->hadException(); ++k) {
        PropertySlot slot;

        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;
        
        JSValue *v = slot.getValue(exec, thisObj, k);
        
        List eachArguments;
        
        eachArguments.append(v);
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);
      
        JSValue *result = eachFunction->call(exec, applyThis, eachArguments);
      
        if (result->toBoolean(exec)) 
            resultArray->put(exec, filterIndex++, v);
    }
    return resultArray;
}

JSValue* ArrayProtoFuncMap::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    JSObject* eachFunction = args[0]->toObject(exec);
    if (!eachFunction->implementsCall())
        return throwError(exec, TypeError);
    
    JSObject *applyThis = args[1]->isUndefinedOrNull() ? exec->dynamicGlobalObject() :  args[1]->toObject(exec);

    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);

    List mapArgs;
    mapArgs.append(jsNumber(length));
    JSObject* resultArray = static_cast<JSObject*>(exec->lexicalGlobalObject()->arrayConstructor()->construct(exec, mapArgs));

    for (unsigned k = 0; k < length && !exec->hadException(); ++k) {
        PropertySlot slot;
        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;

        JSValue *v = slot.getValue(exec, thisObj, k);

        List eachArguments;

        eachArguments.append(v);
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);

        JSValue *result = eachFunction->call(exec, applyThis, eachArguments);
        resultArray->put(exec, k, result);
    }

    return resultArray;
}

// Documentation for these three is available at:
// http://developer-test.mozilla.org/en/docs/Core_JavaScript_1.5_Reference:Objects:Array:every
// http://developer-test.mozilla.org/en/docs/Core_JavaScript_1.5_Reference:Objects:Array:forEach
// http://developer-test.mozilla.org/en/docs/Core_JavaScript_1.5_Reference:Objects:Array:some

JSValue* ArrayProtoFuncEvery::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    JSObject *eachFunction = args[0]->toObject(exec);
    
    if (!eachFunction->implementsCall())
        return throwError(exec, TypeError);
    
    JSObject *applyThis = args[1]->isUndefinedOrNull() ? exec->dynamicGlobalObject() :  args[1]->toObject(exec);
    
    JSValue* result = jsBoolean(true);
    
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    for (unsigned k = 0; k < length && !exec->hadException(); ++k) {
        PropertySlot slot;
          
        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;
      
        List eachArguments;
        
        eachArguments.append(slot.getValue(exec, thisObj, k));
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);
      
        bool predicateResult = eachFunction->call(exec, applyThis, eachArguments)->toBoolean(exec);
      
        if (!predicateResult) {
            result = jsBoolean(false);
            break;
        }
    }

    return result;
}

JSValue* ArrayProtoFuncForEach::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    JSObject* eachFunction = args[0]->toObject(exec);

    if (!eachFunction->implementsCall())
        return throwError(exec, TypeError);

    JSObject* applyThis = args[1]->isUndefinedOrNull() ? exec->dynamicGlobalObject() :  args[1]->toObject(exec);

    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    for (unsigned k = 0; k < length && !exec->hadException(); ++k) {
        PropertySlot slot;
        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;

        List eachArguments;
        eachArguments.append(slot.getValue(exec, thisObj, k));
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);

        eachFunction->call(exec, applyThis, eachArguments);
    }
    return jsUndefined();
}

JSValue* ArrayProtoFuncSome::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    JSObject* eachFunction = args[0]->toObject(exec);

    if (!eachFunction->implementsCall())
        return throwError(exec, TypeError);

    JSObject* applyThis = args[1]->isUndefinedOrNull() ? exec->dynamicGlobalObject() :  args[1]->toObject(exec);

    JSValue* result = jsBoolean(false);

    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    for (unsigned k = 0; k < length && !exec->hadException(); ++k) {
        PropertySlot slot;
        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;

        List eachArguments;
        eachArguments.append(slot.getValue(exec, thisObj, k));
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);

        bool predicateResult = eachFunction->call(exec, applyThis, eachArguments)->toBoolean(exec);

        if (predicateResult) {
            result = jsBoolean(true);
            break;
        }
    }
    return result;
}

JSValue* ArrayProtoFuncIndexOf::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    // JavaScript 1.5 Extension by Mozilla
    // Documentation: http://developer.mozilla.org/en/docs/Core_JavaScript_1.5_Reference:Global_Objects:Array:indexOf

    unsigned index = 0;
    double d = args[1]->toInteger(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    if (d < 0)
        d += length;
    if (d > 0) {
        if (d > length)
            index = length;
        else
            index = static_cast<unsigned>(d);
    }

    JSValue* searchElement = args[0];
    for (; index < length; ++index) {
        JSValue* e = getProperty(exec, thisObj, index);
        if (!e)
            continue;
        if (strictEqual(exec, searchElement, e))
            return jsNumber(index);
    }

    return jsNumber(-1);
}

JSValue* ArrayProtoFuncLastIndexOf::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    // JavaScript 1.6 Extension by Mozilla
    // Documentation: http://developer.mozilla.org/en/docs/Core_JavaScript_1.5_Reference:Global_Objects:Array:lastIndexOf 

    unsigned length = thisObj->get(exec, exec->propertyNames().length)->toUInt32(exec);
    int index = length - 1;
    double d = args[1]->toIntegerPreserveNaN(exec);

    if (d < 0) {
        d += length;
        if (d < 0) 
            return jsNumber(-1);
    }
    if (d < length)
        index = static_cast<int>(d);
          
    JSValue* searchElement = args[0];
    for (; index >= 0; --index) {
        JSValue* e = getProperty(exec, thisObj, index);
        if (!e)
            continue;
        if (strictEqual(exec, searchElement, e))
            return jsNumber(index);
    }
          
    return jsNumber(-1);
}

// ------------------------------ ArrayObjectImp -------------------------------

ArrayObjectImp::ArrayObjectImp(ExecState *exec,
                               FunctionPrototype *funcProto,
                               ArrayPrototype *arrayProto)
  : InternalFunctionImp(funcProto)
{
  // ECMA 15.4.3.1 Array.prototype
  put(exec, exec->propertyNames().prototype, arrayProto, DontEnum|DontDelete|ReadOnly);

  // no. of arguments for constructor
  put(exec, exec->propertyNames().length, jsNumber(1), ReadOnly|DontDelete|DontEnum);
}

bool ArrayObjectImp::implementsConstruct() const
{
  return true;
}

// ECMA 15.4.2
JSObject *ArrayObjectImp::construct(ExecState *exec, const List &args)
{
  // a single numeric argument denotes the array size (!)
  if (args.size() == 1 && args[0]->isNumber()) {
    uint32_t n = args[0]->toUInt32(exec);
    if (n != args[0]->toNumber(exec))
      return throwError(exec, RangeError, "Array size is not a small enough positive integer.");
    return new ArrayInstance(exec->lexicalGlobalObject()->arrayPrototype(), n);
  }

  // otherwise the array is constructed with the arguments in it
  return new ArrayInstance(exec->lexicalGlobalObject()->arrayPrototype(), args);
}

// ECMA 15.6.1
JSValue *ArrayObjectImp::callAsFunction(ExecState *exec, JSObject *, const List &args)
{
  // equivalent to 'new Array(....)'
  return construct(exec,args);
}

}
