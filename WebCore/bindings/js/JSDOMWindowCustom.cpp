/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "JSDOMWindowCustom.h"

#include "AtomicString.h"
#include "Chrome.h"
#include "Database.h"
#include "DOMWindow.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLCollection.h"
#include "HTMLDocument.h"
#include "History.h"
#include "JSAudioConstructor.h"
#if ENABLE(DATABASE)
#include "JSDatabase.h"
#include "JSDatabaseCallback.h"
#endif
#include "JSDOMWindowShell.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include "JSEventSource.h"
#include "JSHTMLCollection.h"
#include "JSHistory.h"
#include "JSImageConstructor.h"
#include "JSLocation.h"
#include "JSMessageChannel.h"
#include "JSMessagePort.h"
#include "JSMessagePortCustom.h"
#include "JSOptionConstructor.h"

#if ENABLE(SHARED_WORKERS)
#include "JSSharedWorker.h"
#endif

#if ENABLE(3D_CANVAS)
#include "JSArrayBuffer.h"
#include "JSInt8Array.h"
#include "JSUint8Array.h"
#include "JSInt32Array.h"
#include "JSUint32Array.h"
#include "JSInt16Array.h"
#include "JSUint16Array.h"
#include "JSFloat32Array.h"
#endif
#include "JSWebKitCSSMatrix.h"
#include "JSWebKitPoint.h"
#if ENABLE(WEB_SOCKETS)
#include "JSWebSocket.h"
#endif
#include "JSWorker.h"
#include "JSXMLHttpRequest.h"
#include "JSXSLTProcessor.h"
#include "Location.h"
#include "MediaPlayer.h"
#include "MessagePort.h"
#include "NotificationCenter.h"
#include "Page.h"
#include "PlatformScreen.h"
#include "RegisteredEventListener.h"
#include "ScheduledAction.h"
#include "ScriptController.h"
#include "SerializedScriptValue.h"
#include "Settings.h"
#include "SharedWorkerRepository.h"
#include "WindowFeatures.h"
#include <runtime/Error.h>
#include <runtime/JSFunction.h>
#include <runtime/JSObject.h>
#include <runtime/PrototypeFunction.h>

using namespace JSC;

namespace WebCore {

void JSDOMWindow::markChildren(MarkStack& markStack)
{
    Base::markChildren(markStack);

    impl()->markJSEventListeners(markStack);

    JSGlobalData& globalData = *Heap::heap(this)->globalData();

    markDOMObjectWrapper(markStack, globalData, impl()->optionalConsole());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalHistory());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalLocationbar());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalMenubar());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalNavigator());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalPersonalbar());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalScreen());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalScrollbars());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalSelection());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalStatusbar());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalToolbar());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalLocation());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalMedia());
#if ENABLE(DOM_STORAGE)
    markDOMObjectWrapper(markStack, globalData, impl()->optionalSessionStorage());
    markDOMObjectWrapper(markStack, globalData, impl()->optionalLocalStorage());
#endif
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    markDOMObjectWrapper(markStack, globalData, impl()->optionalApplicationCache());
#endif
}

template<NativeFunction nativeFunction, int length>
JSValue nonCachingStaticFunctionGetter(ExecState* exec, JSValue, const Identifier& propertyName)
{
    return new (exec) NativeFunctionWrapper(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->prototypeFunctionStructure(), length, propertyName, nativeFunction);
}

static JSValue childFrameGetter(ExecState* exec, JSValue slotBase, const Identifier& propertyName)
{
    return toJS(exec, static_cast<JSDOMWindow*>(asObject(slotBase))->impl()->frame()->tree()->child(identifierToAtomicString(propertyName))->domWindow());
}

static JSValue indexGetter(ExecState* exec, JSValue slotBase, unsigned index)
{
    return toJS(exec, static_cast<JSDOMWindow*>(asObject(slotBase))->impl()->frame()->tree()->child(index)->domWindow());
}

static JSValue namedItemGetter(ExecState* exec, JSValue slotBase, const Identifier& propertyName)
{
    JSDOMWindowBase* thisObj = static_cast<JSDOMWindow*>(asObject(slotBase));
    Document* document = thisObj->impl()->frame()->document();

    ASSERT(thisObj->allowsAccessFrom(exec));
    ASSERT(document);
    ASSERT(document->isHTMLDocument());

    RefPtr<HTMLCollection> collection = document->windowNamedItems(identifierToString(propertyName));
    if (collection->length() == 1)
        return toJS(exec, collection->firstItem());
    return toJS(exec, collection.get());
}

bool JSDOMWindow::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    // When accessing a Window cross-domain, functions are always the native built-in ones, and they
    // are not affected by properties changed on the Window or anything in its prototype chain.
    // This is consistent with the behavior of Firefox.

    const HashEntry* entry;

    // We don't want any properties other than "close" and "closed" on a closed window.
    if (!impl()->frame()) {
        // The following code is safe for cross-domain and same domain use.
        // It ignores any custom properties that might be set on the DOMWindow (including a custom prototype).
        entry = s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && !(entry->attributes() & Function) && entry->propertyGetter() == jsDOMWindowClosed) {
            slot.setCustom(this, entry->propertyGetter());
            return true;
        }
        entry = JSDOMWindowPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && (entry->attributes() & Function) && entry->function() == jsDOMWindowPrototypeFunctionClose) {
            slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
            return true;
        }

        // FIXME: We should have a message here that explains why the property access/function call was
        // not allowed. 
        slot.setUndefined();
        return true;
    }

    // We need to check for cross-domain access here without printing the generic warning message
    // because we always allow access to some function, just different ones depending whether access
    // is allowed.
    String errorMessage;
    bool allowsAccess = allowsAccessFrom(exec, errorMessage);

    // Look for overrides before looking at any of our own properties, but ignore overrides completely
    // if this is cross-domain access.
    if (allowsAccess && JSGlobalObject::getOwnPropertySlot(exec, propertyName, slot))
        return true;

    // We need this code here because otherwise JSDOMWindowBase will stop the search before we even get to the
    // prototype due to the blanket same origin (allowsAccessFrom) check at the end of getOwnPropertySlot.
    // Also, it's important to get the implementation straight out of the DOMWindow prototype regardless of
    // what prototype is actually set on this object.
    entry = JSDOMWindowPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        if (entry->attributes() & Function) {
            if (entry->function() == jsDOMWindowPrototypeFunctionBlur) {
                if (!allowsAccess) {
                    slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionBlur, 0>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionClose) {
                if (!allowsAccess) {
                    slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionFocus) {
                if (!allowsAccess) {
                    slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionFocus, 0>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionPostMessage) {
                if (!allowsAccess) {
                    slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionPostMessage, 2>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionShowModalDialog) {
                if (!DOMWindow::canShowModalDialog(impl()->frame())) {
                    slot.setUndefined();
                    return true;
                }
            }
        }
    } else {
        // Allow access to toString() cross-domain, but always Object.prototype.toString.
        if (propertyName == exec->propertyNames().toString) {
            if (!allowsAccess) {
                slot.setCustom(this, objectToStringFunctionGetter);
                return true;
            }
        }
    }

    entry = JSDOMWindow::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        slot.setCustom(this, entry->propertyGetter());
        return true;
    }

    // Check for child frames by name before built-in properties to
    // match Mozilla. This does not match IE, but some sites end up
    // naming frames things that conflict with window properties that
    // are in Moz but not IE. Since we have some of these, we have to do
    // it the Moz way.
    if (impl()->frame()->tree()->child(identifierToAtomicString(propertyName))) {
        slot.setCustom(this, childFrameGetter);
        return true;
    }

    // Do prototype lookup early so that functions and attributes in the prototype can have
    // precedence over the index and name getters.  
    JSValue proto = prototype();
    if (proto.isObject()) {
        if (asObject(proto)->getPropertySlot(exec, propertyName, slot)) {
            if (!allowsAccess) {
                printErrorMessage(errorMessage);
                slot.setUndefined();
            }
            return true;
        }
    }

    // FIXME: Search the whole frame hierarchy somewhere around here.
    // We need to test the correct priority order.

    // allow window[1] or parent[1] etc. (#56983)
    bool ok;
    unsigned i = propertyName.toArrayIndex(&ok);
    if (ok && i < impl()->frame()->tree()->childCount()) {
        slot.setCustomIndex(this, i, indexGetter);
        return true;
    }

    if (!allowsAccess) {
        printErrorMessage(errorMessage);
        slot.setUndefined();
        return true;
    }

    // Allow shortcuts like 'Image1' instead of document.images.Image1
    Document* document = impl()->frame()->document();
    if (document->isHTMLDocument()) {
        AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
        if (atomicPropertyName && (static_cast<HTMLDocument*>(document)->hasNamedItem(atomicPropertyName) || document->hasElementWithId(atomicPropertyName))) {
            slot.setCustom(this, namedItemGetter);
            return true;
        }
    }

    return Base::getOwnPropertySlot(exec, propertyName, slot);
}

bool JSDOMWindow::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    // Never allow cross-domain getOwnPropertyDescriptor
    if (!allowsAccessFrom(exec))
        return false;

    const HashEntry* entry;
    
    // We don't want any properties other than "close" and "closed" on a closed window.
    if (!impl()->frame()) {
        // The following code is safe for cross-domain and same domain use.
        // It ignores any custom properties that might be set on the DOMWindow (including a custom prototype).
        entry = s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && !(entry->attributes() & Function) && entry->propertyGetter() == jsDOMWindowClosed) {
            descriptor.setDescriptor(jsBoolean(true), ReadOnly | DontDelete | DontEnum);
            return true;
        }
        entry = JSDOMWindowPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && (entry->attributes() & Function) && entry->function() == jsDOMWindowPrototypeFunctionClose) {
            PropertySlot slot;
            slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
            return true;
        }
        descriptor.setUndefined();
        return true;
    }

    entry = JSDOMWindow::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        PropertySlot slot;
        slot.setCustom(this, entry->propertyGetter());
        descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
        return true;
    }
    
    // Check for child frames by name before built-in properties to
    // match Mozilla. This does not match IE, but some sites end up
    // naming frames things that conflict with window properties that
    // are in Moz but not IE. Since we have some of these, we have to do
    // it the Moz way.
    if (impl()->frame()->tree()->child(identifierToAtomicString(propertyName))) {
        PropertySlot slot;
        slot.setCustom(this, childFrameGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
        return true;
    }
    
    bool ok;
    unsigned i = propertyName.toArrayIndex(&ok);
    if (ok && i < impl()->frame()->tree()->childCount()) {
        PropertySlot slot;
        slot.setCustomIndex(this, i, indexGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
        return true;
    }

    // Allow shortcuts like 'Image1' instead of document.images.Image1
    Document* document = impl()->frame()->document();
    if (document->isHTMLDocument()) {
        AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
        if (atomicPropertyName && (static_cast<HTMLDocument*>(document)->hasNamedItem(atomicPropertyName) || document->hasElementWithId(atomicPropertyName))) {
            PropertySlot slot;
            slot.setCustom(this, namedItemGetter);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
            return true;
        }
    }
    
    return Base::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

void JSDOMWindow::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    if (!impl()->frame())
        return;

    // Optimization: access JavaScript global variables directly before involving the DOM.
    if (JSGlobalObject::hasOwnPropertyForWrite(exec, propertyName)) {
        if (allowsAccessFrom(exec))
            JSGlobalObject::put(exec, propertyName, value, slot);
        return;
    }

    if (lookupPut<JSDOMWindow>(exec, propertyName, value, s_info.propHashTable(exec), this))
        return;

    if (allowsAccessFrom(exec))
        Base::put(exec, propertyName, value, slot);
}

bool JSDOMWindow::deleteProperty(ExecState* exec, const Identifier& propertyName)
{
    // Only allow deleting properties by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return false;
    return Base::deleteProperty(exec, propertyName);
}

void JSDOMWindow::getPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    // Only allow the window to enumerated by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return;
    Base::getPropertyNames(exec, propertyNames, mode);
}

void JSDOMWindow::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    // Only allow the window to enumerated by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return;
    Base::getOwnPropertyNames(exec, propertyNames, mode);
}

void JSDOMWindow::defineGetter(ExecState* exec, const Identifier& propertyName, JSObject* getterFunction, unsigned attributes)
{
    // Only allow defining getters by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return;

    // Don't allow shadowing location using defineGetter.
    if (propertyName == "location")
        return;

    Base::defineGetter(exec, propertyName, getterFunction, attributes);
}

void JSDOMWindow::defineSetter(ExecState* exec, const Identifier& propertyName, JSObject* setterFunction, unsigned attributes)
{
    // Only allow defining setters by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return;
    Base::defineSetter(exec, propertyName, setterFunction, attributes);
}

bool JSDOMWindow::defineOwnProperty(JSC::ExecState* exec, const JSC::Identifier& propertyName, JSC::PropertyDescriptor& descriptor, bool shouldThrow)
{
    // Only allow defining properties in this way by frames in the same origin, as it allows setters to be introduced.
    if (!allowsAccessFrom(exec))
        return false;
    return Base::defineOwnProperty(exec, propertyName, descriptor, shouldThrow);
}

JSValue JSDOMWindow::lookupGetter(ExecState* exec, const Identifier& propertyName)
{
    // Only allow looking-up getters by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return jsUndefined();
    return Base::lookupGetter(exec, propertyName);
}

JSValue JSDOMWindow::lookupSetter(ExecState* exec, const Identifier& propertyName)
{
    // Only allow looking-up setters by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return jsUndefined();
    return Base::lookupSetter(exec, propertyName);
}

// Custom Attributes

JSValue JSDOMWindow::history(ExecState* exec) const
{
    History* history = impl()->history();
    if (DOMObject* wrapper = getCachedDOMObjectWrapper(exec, history))
        return wrapper;

    JSDOMWindow* window = const_cast<JSDOMWindow*>(this);
    JSHistory* jsHistory = new (exec) JSHistory(getDOMStructure<JSHistory>(exec, window), window, history);
    cacheDOMObjectWrapper(exec, history, jsHistory);
    return jsHistory;
}

JSValue JSDOMWindow::location(ExecState* exec) const
{
    Location* location = impl()->location();
    if (DOMObject* wrapper = getCachedDOMObjectWrapper(exec, location))
        return wrapper;

    JSDOMWindow* window = const_cast<JSDOMWindow*>(this);
    JSLocation* jsLocation = new (exec) JSLocation(getDOMStructure<JSLocation>(exec, window), window, location);
    cacheDOMObjectWrapper(exec, location, jsLocation);
    return jsLocation;
}

void JSDOMWindow::setLocation(ExecState* exec, JSValue value)
{
    Frame* lexicalFrame = toLexicalFrame(exec);
    if (!lexicalFrame)
        return;

#if ENABLE(DASHBOARD_SUPPORT)
    // To avoid breaking old widgets, make "var location =" in a top-level frame create
    // a property named "location" instead of performing a navigation (<rdar://problem/5688039>).
    if (Settings* settings = lexicalFrame->settings()) {
        if (settings->usesDashboardBackwardCompatibilityMode() && !lexicalFrame->tree()->parent()) {
            if (allowsAccessFrom(exec))
                putDirect(Identifier(exec, "location"), value);
            return;
        }
    }
#endif

    Frame* frame = impl()->frame();
    ASSERT(frame);

    KURL url = completeURL(exec, ustringToString(value.toString(exec)));
    if (url.isNull())
        return;

    if (!shouldAllowNavigation(exec, frame))
        return;

    if (!protocolIsJavaScript(url) || allowsAccessFrom(exec)) {
        // We want a new history item if this JS was called via a user gesture
        frame->redirectScheduler()->scheduleLocationChange(url, lexicalFrame->loader()->outgoingReferrer(), !lexicalFrame->script()->anyPageIsProcessingUserGesture(), false, processingUserGesture(exec));
    }
}

JSValue JSDOMWindow::crypto(ExecState*) const
{
    return jsUndefined();
}

JSValue JSDOMWindow::event(ExecState* exec) const
{
    Event* event = currentEvent();
    if (!event)
        return jsUndefined();
    return toJS(exec, event);
}

#if ENABLE(EVENTSOURCE)
JSValue JSDOMWindow::eventSource(ExecState* exec) const
{
    return getDOMConstructor<JSEventSourceConstructor>(exec, this);
}
#endif

JSValue JSDOMWindow::image(ExecState* exec) const
{
    return getDOMConstructor<JSImageConstructor>(exec, this);
}

JSValue JSDOMWindow::option(ExecState* exec) const
{
    return getDOMConstructor<JSOptionConstructor>(exec, this);
}

#if ENABLE(VIDEO)
JSValue JSDOMWindow::audio(ExecState* exec) const
{
    if (!MediaPlayer::isAvailable())
        return jsUndefined();
    return getDOMConstructor<JSAudioConstructor>(exec, this);
}
#endif

JSValue JSDOMWindow::webKitPoint(ExecState* exec) const
{
    return getDOMConstructor<JSWebKitPointConstructor>(exec, this);
}

JSValue JSDOMWindow::webKitCSSMatrix(ExecState* exec) const
{
    return getDOMConstructor<JSWebKitCSSMatrixConstructor>(exec, this);
}
 
#if ENABLE(3D_CANVAS)
JSValue JSDOMWindow::arrayBuffer(ExecState* exec) const
{
    return getDOMConstructor<JSArrayBufferConstructor>(exec, this);
}
 
JSValue JSDOMWindow::int8Array(ExecState* exec) const
{
    return getDOMConstructor<JSInt8ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::uint8Array(ExecState* exec) const
{
    return getDOMConstructor<JSUint8ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::int32Array(ExecState* exec) const
{
    return getDOMConstructor<JSInt32ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::uint32Array(ExecState* exec) const
{
    return getDOMConstructor<JSUint32ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::int16Array(ExecState* exec) const
{
    return getDOMConstructor<JSInt16ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::uint16Array(ExecState* exec) const
{
    return getDOMConstructor<JSUint16ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::float32Array(ExecState* exec) const
{
    return getDOMConstructor<JSFloat32ArrayConstructor>(exec, this);
}

// Temporary aliases to keep current WebGL content working during transition period to TypedArray spec.
// To be removed before WebGL spec is finalized. (FIXME)
JSValue JSDOMWindow::webGLArrayBuffer(ExecState* exec) const
{
    return getDOMConstructor<JSArrayBufferConstructor>(exec, this);
}

JSValue JSDOMWindow::webGLByteArray(ExecState* exec) const
{
    return getDOMConstructor<JSInt8ArrayConstructor>(exec, this);
}

JSValue JSDOMWindow::webGLUnsignedByteArray(ExecState* exec) const
{
    return getDOMConstructor<JSUint8ArrayConstructor>(exec, this);
}

JSValue JSDOMWindow::webGLIntArray(ExecState* exec) const
{
    return getDOMConstructor<JSInt32ArrayConstructor>(exec, this);
}

JSValue JSDOMWindow::webGLUnsignedIntArray(ExecState* exec) const
{
    return getDOMConstructor<JSUint32ArrayConstructor>(exec, this);
}

JSValue JSDOMWindow::webGLShortArray(ExecState* exec) const
{
    return getDOMConstructor<JSInt16ArrayConstructor>(exec, this);
}

JSValue JSDOMWindow::webGLUnsignedShortArray(ExecState* exec) const
{
    return getDOMConstructor<JSUint16ArrayConstructor>(exec, this);
}

JSValue JSDOMWindow::webGLFloatArray(ExecState* exec) const
{
    return getDOMConstructor<JSFloat32ArrayConstructor>(exec, this);
}
#endif
 
JSValue JSDOMWindow::xmlHttpRequest(ExecState* exec) const
{
    return getDOMConstructor<JSXMLHttpRequestConstructor>(exec, this);
}

#if ENABLE(XSLT)
JSValue JSDOMWindow::xsltProcessor(ExecState* exec) const
{
    return getDOMConstructor<JSXSLTProcessorConstructor>(exec, this);
}
#endif

#if ENABLE(CHANNEL_MESSAGING)
JSValue JSDOMWindow::messageChannel(ExecState* exec) const
{
    return getDOMConstructor<JSMessageChannelConstructor>(exec, this);
}
#endif

#if ENABLE(WORKERS)
JSValue JSDOMWindow::worker(ExecState* exec) const
{
    return getDOMConstructor<JSWorkerConstructor>(exec, this);
}
#endif

#if ENABLE(SHARED_WORKERS)
JSValue JSDOMWindow::sharedWorker(ExecState* exec) const
{
    if (SharedWorkerRepository::isAvailable())
        return getDOMConstructor<JSSharedWorkerConstructor>(exec, this);
    return jsUndefined();
}
#endif

#if ENABLE(WEB_SOCKETS)
JSValue JSDOMWindow::webSocket(ExecState* exec) const
{
    Frame* frame = impl()->frame();
    if (!frame)
        return jsUndefined();
    Settings* settings = frame->settings();
    if (!settings)
        return jsUndefined();
    return getDOMConstructor<JSWebSocketConstructor>(exec, this);
}
#endif

// Custom functions

// Helper for window.open() and window.showModalDialog()
static Frame* createWindow(ExecState* exec, Frame* lexicalFrame, Frame* dynamicFrame,
                           Frame* openerFrame, const String& url, const String& frameName, 
                           const WindowFeatures& windowFeatures, JSValue dialogArgs)
{
    ASSERT(lexicalFrame);
    ASSERT(dynamicFrame);

    ResourceRequest request;

    // For whatever reason, Firefox uses the dynamicGlobalObject to determine
    // the outgoingReferrer.  We replicate that behavior here.
    String referrer = dynamicFrame->loader()->outgoingReferrer();
    request.setHTTPReferrer(referrer);
    FrameLoader::addHTTPOriginIfNeeded(request, dynamicFrame->loader()->outgoingOrigin());
    FrameLoadRequest frameRequest(request, frameName);

    // FIXME: It's much better for client API if a new window starts with a URL, here where we
    // know what URL we are going to open. Unfortunately, this code passes the empty string
    // for the URL, but there's a reason for that. Before loading we have to set up the opener,
    // openedByDOM, and dialogArguments values. Also, to decide whether to use the URL we currently
    // do an allowsAccessFrom call using the window we create, which can't be done before creating it.
    // We'd have to resolve all those issues to pass the URL instead of "".

    bool created;
    // We pass in the opener frame here so it can be used for looking up the frame name, in case the active frame
    // is different from the opener frame, and the name references a frame relative to the opener frame, for example
    // "_self" or "_parent".
    Frame* newFrame = lexicalFrame->loader()->createWindow(openerFrame->loader(), frameRequest, windowFeatures, created);
    if (!newFrame)
        return 0;

    newFrame->loader()->setOpener(openerFrame);
    newFrame->page()->setOpenedByDOM();

    // FIXME: If a window is created from an isolated world, what are the consequences of this? 'dialogArguments' only appears back in the normal world?
    JSDOMWindow* newWindow = toJSDOMWindow(newFrame, normalWorld(exec->globalData()));

    if (dialogArgs)
        newWindow->putDirect(Identifier(exec, "dialogArguments"), dialogArgs);

    if (!protocolIsJavaScript(url) || newWindow->allowsAccessFrom(exec)) {
        KURL completedURL = url.isEmpty() ? KURL(ParsedURLString, "") : completeURL(exec, url);
        bool userGesture = processingUserGesture(exec);

        if (created)
            newFrame->loader()->changeLocation(completedURL, referrer, false, false, userGesture);
        else if (!url.isEmpty())
            newFrame->redirectScheduler()->scheduleLocationChange(completedURL.string(), referrer, !lexicalFrame->script()->anyPageIsProcessingUserGesture(), false, userGesture);
    }

    return newFrame;
}

static bool domWindowAllowPopUp(Frame* activeFrame, ExecState* exec)
{
    ASSERT(activeFrame);
    if (activeFrame->script()->processingUserGesture(currentWorld(exec)))
        return true;
    return DOMWindow::allowPopUp(activeFrame);
}

JSValue JSDOMWindow::open(ExecState* exec)
{
    String urlString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    AtomicString frameName = exec->argument(1).isUndefinedOrNull() ? "_blank" : ustringToAtomicString(exec->argument(1).toString(exec));
    WindowFeatures windowFeatures(valueToStringWithUndefinedOrNullCheck(exec, exec->argument(2)));

    Frame* frame = impl()->frame();
    if (!frame)
        return jsUndefined();
    Frame* lexicalFrame = toLexicalFrame(exec);
    if (!lexicalFrame)
        return jsUndefined();
    Frame* dynamicFrame = toDynamicFrame(exec);
    if (!dynamicFrame)
        return jsUndefined();

    Page* page = frame->page();

    // Because FrameTree::find() returns true for empty strings, we must check for empty framenames.
    // Otherwise, illegitimate window.open() calls with no name will pass right through the popup blocker.
    if (!domWindowAllowPopUp(dynamicFrame, exec) && (frameName.isEmpty() || !frame->tree()->find(frameName)))
        return jsUndefined();

    // Get the target frame for the special cases of _top and _parent.  In those
    // cases, we can schedule a location change right now and return early.
    bool topOrParent = false;
    if (frameName == "_top") {
        frame = frame->tree()->top();
        topOrParent = true;
    } else if (frameName == "_parent") {
        if (Frame* parent = frame->tree()->parent())
            frame = parent;
        topOrParent = true;
    }
    if (topOrParent) {
        String completedURL;
        if (!urlString.isEmpty())
            completedURL = completeURL(exec, urlString).string();

        if (!shouldAllowNavigation(exec, frame))
            return jsUndefined();

        const JSDOMWindow* targetedWindow = toJSDOMWindow(frame, currentWorld(exec));
        if (!completedURL.isEmpty() && (!protocolIsJavaScript(completedURL) || (targetedWindow && targetedWindow->allowsAccessFrom(exec)))) {
            bool userGesture = processingUserGesture(exec);

            // For whatever reason, Firefox uses the dynamicGlobalObject to
            // determine the outgoingReferrer.  We replicate that behavior
            // here.
            String referrer = dynamicFrame->loader()->outgoingReferrer();

            frame->redirectScheduler()->scheduleLocationChange(completedURL, referrer, !lexicalFrame->script()->anyPageIsProcessingUserGesture(), false, userGesture);
        }
        return toJS(exec, frame->domWindow());
    }

    // In the case of a named frame or a new window, we'll use the createWindow() helper
    FloatRect windowRect(windowFeatures.xSet ? windowFeatures.x : 0, windowFeatures.ySet ? windowFeatures.y : 0,
                         windowFeatures.widthSet ? windowFeatures.width : 0, windowFeatures.heightSet ? windowFeatures.height : 0);
    DOMWindow::adjustWindowRect(screenAvailableRect(page ? page->mainFrame()->view() : 0), windowRect, windowRect);

    windowFeatures.x = windowRect.x();
    windowFeatures.y = windowRect.y();
    windowFeatures.height = windowRect.height();
    windowFeatures.width = windowRect.width();

    frame = createWindow(exec, lexicalFrame, dynamicFrame, frame, urlString, frameName, windowFeatures, JSValue());

    if (!frame)
        return jsUndefined();

    return toJS(exec, frame->domWindow());
}

JSValue JSDOMWindow::showModalDialog(ExecState* exec)
{
    String url = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    JSValue dialogArgs = exec->argument(1);
    String featureArgs = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(2));

    Frame* frame = impl()->frame();
    if (!frame)
        return jsUndefined();
    Frame* lexicalFrame = toLexicalFrame(exec);
    if (!lexicalFrame)
        return jsUndefined();
    Frame* dynamicFrame = toDynamicFrame(exec);
    if (!dynamicFrame)
        return jsUndefined();

    if (!DOMWindow::canShowModalDialogNow(frame) || !domWindowAllowPopUp(dynamicFrame, exec))
        return jsUndefined();

    HashMap<String, String> features;
    DOMWindow::parseModalDialogFeatures(featureArgs, features);

    const bool trusted = false;

    // The following features from Microsoft's documentation are not implemented:
    // - default font settings
    // - width, height, left, and top specified in units other than "px"
    // - edge (sunken or raised, default is raised)
    // - dialogHide: trusted && boolFeature(features, "dialoghide"), makes dialog hide when you print
    // - help: boolFeature(features, "help", true), makes help icon appear in dialog (what does it do on Windows?)
    // - unadorned: trusted && boolFeature(features, "unadorned");

    FloatRect screenRect = screenAvailableRect(frame->view());

    WindowFeatures wargs;
    wargs.width = WindowFeatures::floatFeature(features, "dialogwidth", 100, screenRect.width(), 620); // default here came from frame size of dialog in MacIE
    wargs.widthSet = true;
    wargs.height = WindowFeatures::floatFeature(features, "dialogheight", 100, screenRect.height(), 450); // default here came from frame size of dialog in MacIE
    wargs.heightSet = true;

    wargs.x = WindowFeatures::floatFeature(features, "dialogleft", screenRect.x(), screenRect.right() - wargs.width, -1);
    wargs.xSet = wargs.x > 0;
    wargs.y = WindowFeatures::floatFeature(features, "dialogtop", screenRect.y(), screenRect.bottom() - wargs.height, -1);
    wargs.ySet = wargs.y > 0;

    if (WindowFeatures::boolFeature(features, "center", true)) {
        if (!wargs.xSet) {
            wargs.x = screenRect.x() + (screenRect.width() - wargs.width) / 2;
            wargs.xSet = true;
        }
        if (!wargs.ySet) {
            wargs.y = screenRect.y() + (screenRect.height() - wargs.height) / 2;
            wargs.ySet = true;
        }
    }

    wargs.dialog = true;
    wargs.resizable = WindowFeatures::boolFeature(features, "resizable");
    wargs.scrollbarsVisible = WindowFeatures::boolFeature(features, "scroll", true);
    wargs.statusBarVisible = WindowFeatures::boolFeature(features, "status", !trusted);
    wargs.menuBarVisible = false;
    wargs.toolBarVisible = false;
    wargs.locationBarVisible = false;
    wargs.fullscreen = false;

    Frame* dialogFrame = createWindow(exec, lexicalFrame, dynamicFrame, frame, url, "", wargs, dialogArgs);
    if (!dialogFrame)
        return jsUndefined();

    JSDOMWindow* dialogWindow = toJSDOMWindow(dialogFrame, currentWorld(exec));
    dialogFrame->page()->chrome()->runModal();

    Identifier returnValue(exec, "returnValue");
    if (dialogWindow->allowsAccessFromNoErrorMessage(exec)) {
        PropertySlot slot;
        // This is safe, we have already performed the origin security check and we are
        // not interested in any of the DOM properties of the window.
        if (dialogWindow->JSGlobalObject::getOwnPropertySlot(exec, returnValue, slot))
            return slot.getValue(exec, returnValue);
    }
    return jsUndefined();
}

JSValue JSDOMWindow::postMessage(ExecState* exec)
{
    DOMWindow* window = impl();

    DOMWindow* source = asJSDOMWindow(exec->lexicalGlobalObject())->impl();
    PassRefPtr<SerializedScriptValue> message = SerializedScriptValue::create(exec, exec->argument(0));

    if (exec->hadException())
        return jsUndefined();

    MessagePortArray messagePorts;
    if (exec->argumentCount() > 2)
        fillMessagePortArray(exec, exec->argument(1), messagePorts);
    if (exec->hadException())
        return jsUndefined();

    String targetOrigin = valueToStringWithUndefinedOrNullCheck(exec, exec->argument((exec->argumentCount() == 2) ? 1 : 2));
    if (exec->hadException())
        return jsUndefined();

    ExceptionCode ec = 0;
    window->postMessage(message, &messagePorts, targetOrigin, source, ec);
    setDOMException(exec, ec);

    return jsUndefined();
}

JSValue JSDOMWindow::setTimeout(ExecState* exec)
{
    OwnPtr<ScheduledAction> action = ScheduledAction::create(exec, currentWorld(exec));
    if (exec->hadException())
        return jsUndefined();
    int delay = exec->argument(1).toInt32(exec);

    ExceptionCode ec = 0;
    int result = impl()->setTimeout(action.release(), delay, ec);
    setDOMException(exec, ec);

    return jsNumber(exec, result);
}

JSValue JSDOMWindow::setInterval(ExecState* exec)
{
    OwnPtr<ScheduledAction> action = ScheduledAction::create(exec, currentWorld(exec));
    if (exec->hadException())
        return jsUndefined();
    int delay = exec->argument(1).toInt32(exec);

    ExceptionCode ec = 0;
    int result = impl()->setInterval(action.release(), delay, ec);
    setDOMException(exec, ec);

    return jsNumber(exec, result);
}

JSValue JSDOMWindow::addEventListener(ExecState* exec)
{
    Frame* frame = impl()->frame();
    if (!frame)
        return jsUndefined();

    JSValue listener = exec->argument(1);
    if (!listener.isObject())
        return jsUndefined();

    impl()->addEventListener(ustringToAtomicString(exec->argument(0).toString(exec)), JSEventListener::create(asObject(listener), this, false, currentWorld(exec)), exec->argument(2).toBoolean(exec));
    return jsUndefined();
}

JSValue JSDOMWindow::removeEventListener(ExecState* exec)
{
    Frame* frame = impl()->frame();
    if (!frame)
        return jsUndefined();

    JSValue listener = exec->argument(1);
    if (!listener.isObject())
        return jsUndefined();

    impl()->removeEventListener(ustringToAtomicString(exec->argument(0).toString(exec)), JSEventListener::create(asObject(listener), this, false, currentWorld(exec)).get(), exec->argument(2).toBoolean(exec));
    return jsUndefined();
}

#if ENABLE(DATABASE)
JSValue JSDOMWindow::openDatabase(ExecState* exec)
{
    if (!allowsAccessFrom(exec) || (exec->argumentCount() < 4)) {
        setDOMException(exec, SYNTAX_ERR);
        return jsUndefined();
    }

    String name = ustringToString(exec->argument(0).toString(exec));
    if (exec->hadException())
        return jsUndefined();

    String version = ustringToString(exec->argument(1).toString(exec));
    if (exec->hadException())
        return jsUndefined();

    String displayName = ustringToString(exec->argument(2).toString(exec));
    if (exec->hadException())
        return jsUndefined();

    // exec->argument(3) = estimated size
    unsigned long estimatedSize = exec->argument(3).toUInt32(exec);
    if (exec->hadException())
        return jsUndefined();

    RefPtr<DatabaseCallback> creationCallback;
    if (exec->argumentCount() >= 5) {
        if (!exec->argument(4).isObject()) {
            setDOMException(exec, TYPE_MISMATCH_ERR);
            return jsUndefined();
        }

        creationCallback = JSDatabaseCallback::create(asObject(exec->argument(4)), globalObject());
    }

    ExceptionCode ec = 0;
    JSValue result = toJS(exec, globalObject(), WTF::getPtr(impl()->openDatabase(name, version, displayName, estimatedSize, creationCallback.release(), ec)));

    setDOMException(exec, ec);
    return result;
}
#endif

DOMWindow* toDOMWindow(JSValue value)
{
    if (!value.isObject())
        return 0;
    JSObject* object = asObject(value);
    if (object->inherits(&JSDOMWindow::s_info))
        return static_cast<JSDOMWindow*>(object)->impl();
    if (object->inherits(&JSDOMWindowShell::s_info))
        return static_cast<JSDOMWindowShell*>(object)->impl();
    return 0;
}

} // namespace WebCore
