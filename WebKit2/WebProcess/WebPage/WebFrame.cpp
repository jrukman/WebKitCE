/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "WebFrame.h"

#include "WebPage.h"
#include <WebCore/AnimationController.h>
#include <WebCore/Frame.h>
#include <WebCore/HTMLFrameOwnerElement.h>
#include <WebCore/PlatformString.h>

#ifndef NDEBUG
#include <wtf/RefCountedLeakCounter.h>
#endif

using namespace WebCore;

namespace WebKit {

#ifndef NDEBUG
static WTF::RefCountedLeakCounter webFrameCounter("WebFrame");
#endif

static uint64_t generateFrameID()
{
    static uint64_t uniqueFrameID = 1;
    return uniqueFrameID++;
}

static uint64_t generateListenerID()
{
    static uint64_t uniqueListenerID = 1;
    return uniqueListenerID++;
}

PassRefPtr<WebFrame> WebFrame::createMainFrame(WebPage* page)
{
    return create(page, String(), 0);
}

PassRefPtr<WebFrame> WebFrame::createSubframe(WebPage* page, const String& frameName, HTMLFrameOwnerElement* ownerElement)
{
    return create(page, frameName, ownerElement);
}

PassRefPtr<WebFrame> WebFrame::create(WebPage* page, const String& frameName, HTMLFrameOwnerElement* ownerElement)
{
    RefPtr<WebFrame> frame = adoptRef(new WebFrame(page, frameName, ownerElement));
    
    // Add explict ref() that will be balanced in WebFrameLoaderClient::frameLoaderDestroyed().
    frame->ref();
    
    return frame.release();
}

WebFrame::WebFrame(WebPage* page, const String& frameName, HTMLFrameOwnerElement* ownerElement)
    : m_page(page)
    , m_coreFrame(0)
    , m_policyListenerID(0)
    , m_policyFunction(0)
    , m_frameLoaderClient(this)
    , m_loadListener(0)
    , m_frameID(generateFrameID())
{
    m_page->addWebFrame(m_frameID, this);

    RefPtr<Frame> frame = Frame::create(page->corePage(), ownerElement, &m_frameLoaderClient);
    m_coreFrame = frame.get();

    frame->tree()->setName(frameName);

    if (ownerElement) {
        ASSERT(ownerElement->document()->frame());
        ownerElement->document()->frame()->tree()->appendChild(frame);
    }

    frame->init();

#ifndef NDEBUG
    webFrameCounter.increment();
#endif
}

WebFrame::~WebFrame()
{
    ASSERT(!m_coreFrame);

#ifndef NDEBUG
    webFrameCounter.decrement();
#endif
}

void WebFrame::invalidate()
{
    m_page->removeWebFrame(m_frameID);
    m_coreFrame = 0;
}

uint64_t WebFrame::setUpPolicyListener(WebCore::FramePolicyFunction policyFunction)
{
    // FIXME: <rdar://5634381> We need to support multiple active policy listeners.

    invalidatePolicyListener();

    m_policyListenerID = generateListenerID();
    m_policyFunction = policyFunction;
    return m_policyListenerID;
}

void WebFrame::invalidatePolicyListener()
{
    if (!m_policyListenerID)
        return;

    m_policyListenerID = 0;
    m_policyFunction = 0;
}

void WebFrame::didReceivePolicyDecision(uint64_t listenerID, PolicyAction action)
{
    if (!m_coreFrame)
        return;

    if (!m_policyListenerID)
        return;

    if (listenerID != m_policyListenerID)
        return;

    ASSERT(m_policyFunction);

    FramePolicyFunction function = m_policyFunction;

    invalidatePolicyListener();

    (m_coreFrame->loader()->policyChecker()->*function)(action);
}

bool WebFrame::isMainFrame() const
{
    return m_page->mainFrame() == this;
}

String WebFrame::url() const
{
    if (!m_coreFrame)
        return String();

    return m_coreFrame->loader()->url().string();
}

String WebFrame::innerText() const
{
    if (!m_coreFrame)
        return String();

    return m_coreFrame->document()->documentElement()->innerText();
}

static void childFrameRef(const void* frame)
{
    static_cast<WebFrame*>(const_cast<void*>(frame))->ref();
}

static void childFrameDeref(const void* frame)
{
    static_cast<WebFrame*>(const_cast<void*>(frame))->deref();
}

PassRefPtr<ImmutableArray> WebFrame::childFrames()
{
    if (!m_coreFrame)
        return ImmutableArray::create();

    size_t size = m_coreFrame->tree()->childCount();
    if (!size)
        return ImmutableArray::create();

    void** array = new void*[size];
    unsigned i = 0;
    for (Frame* child = m_coreFrame->tree()->firstChild(); child; child = child->tree()->nextSibling(), ++i) {
        WebFrame* webFrame = static_cast<WebFrameLoaderClient*>(child->loader()->client())->webFrame();
        webFrame->ref();
        array[i] = webFrame;
    }

    ImmutableArray::ImmutableArrayCallbacks callbacks = {
        childFrameRef,
        childFrameDeref
    };
    return ImmutableArray::adopt(array, size, &callbacks);
}

unsigned WebFrame::numberOfActiveAnimations()
{
    if (!m_coreFrame)
        return 0;

    AnimationController* controller = m_coreFrame->animation();
    if (!controller)
        return 0;

    return controller->numberOfActiveAnimations();
}

bool WebFrame::pauseAnimationOnElementWithId(const String& animationName, const String& elementID, double time)
{
    if (!m_coreFrame)
        return false;

    AnimationController* controller = m_coreFrame->animation();
    if (!controller)
        return false;

    if (!m_coreFrame->document())
        return false;

    Node* coreNode = m_coreFrame->document()->getElementById(elementID);
    if (!coreNode || !coreNode->renderer())
        return false;

    return controller->pauseAnimationAtTime(coreNode->renderer(), animationName, time);
}


} // namespace WebKit
