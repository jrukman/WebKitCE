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

#ifndef WebPageProxy_h
#define WebPageProxy_h

#include "DrawingAreaProxy.h"
#include "GenericCallback.h"
#include "WKBase.h"
#include "WebEvent.h"
#include "WebFrameProxy.h"
#include "WebHistoryClient.h"
#include "WebLoaderClient.h"
#include "WebPolicyClient.h"
#include "WebUIClient.h"
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/PlatformString.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace CoreIPC {
    class ArgumentDecoder;
    class Connection;
    class MessageID;
}

namespace WebCore {
    class IntSize;
    class Cursor;
}

struct WKContextStatistics;

namespace WebKit {

class DrawingAreaProxy;
class PageClient;
class WebBackForwardList;
class WebBackForwardListItem;
class WebKeyboardEvent;
class WebMouseEvent;
class WebPageNamespace;
class WebProcessProxy;
class WebWheelEvent;
struct WebNavigationDataStore;

typedef GenericCallback<WKStringRef> RenderTreeExternalRepresentationCallback;
typedef GenericCallback<WKStringRef> ScriptReturnValueCallback;

class WebPageProxy : public RefCounted<WebPageProxy> {
public:
    static PassRefPtr<WebPageProxy> create(WebPageNamespace*, uint64_t pageID);
    ~WebPageProxy();

    uint64_t pageID() const { return m_pageID; }

    WebFrameProxy* webFrame(uint64_t) const;
    WebFrameProxy* mainFrame() const { return m_mainFrame.get(); }

    DrawingAreaProxy* drawingArea() { return m_drawingArea.get(); }

    WebBackForwardList* backForwardList() { return m_backForwardList.get(); }

    void setPageClient(PageClient*);
    void initializeLoaderClient(WKPageLoaderClient*);
    void initializePolicyClient(WKPagePolicyClient*);
    void initializeUIClient(WKPageUIClient*);
    void initializeHistoryClient(WKPageHistoryClient*);

    void revive();

    void initializeWebPage(const WebCore::IntSize&, PassOwnPtr<DrawingAreaProxy>);
    void reinitializeWebPage(const WebCore::IntSize&);

    void close();
    bool tryClose();
    bool isClosed() const { return m_closed; }

    void loadURL(const WebCore::String&);
    void stopLoading();
    void reload(bool reloadFromOrigin);

    void goForward();
    bool canGoForward() const;
    void goBack();
    bool canGoBack() const;

    void goToBackForwardItem(WebBackForwardListItem*);

    void setFocused(bool isFocused);
    void setActive(bool active);
    void setIsInWindow(bool isInWindow);

    void mouseEvent(const WebMouseEvent&);
    void wheelEvent(const WebWheelEvent&);
    void keyEvent(const WebKeyboardEvent&);

    const WebCore::String& pageTitle() const { return m_pageTitle; }
    const WebCore::String& toolTip() const { return m_toolTip; }

    double estimatedProgress() const { return m_estimatedProgress; }

    void terminateProcess();

    void runJavaScriptInMainFrame(const WebCore::String&, PassRefPtr<ScriptReturnValueCallback>);
    void getRenderTreeExternalRepresentation(PassRefPtr<RenderTreeExternalRepresentationCallback>);

    void receivedPolicyDecision(WebCore::PolicyAction, WebFrameProxy*, uint64_t listenerID);

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder&);
    void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder&, CoreIPC::ArgumentEncoder&);

    void processDidBecomeUnresponsive();
    void processDidBecomeResponsive();
    void processDidExit();
    void processDidRevive();

    WebProcessProxy* process() const;
    WebPageNamespace* pageNamespace() const { return m_pageNamespace.get(); }

    bool isValid();

    // REMOVE: For demo purposes only.
    const WebCore::String& urlAtProcessExit() const { return m_urlAtProcessExit; }

    void preferencesDidChange();

    void getStatistics(WKContextStatistics*);

private:
    WebPageProxy(WebPageNamespace*, uint64_t pageID);

    void didCreateMainFrame(uint64_t frameID);
    void didCreateSubFrame(uint64_t frameID);

    void didStartProvisionalLoadForFrame(WebFrameProxy*, const WebCore::String&);
    void didReceiveServerRedirectForProvisionalLoadForFrame(WebFrameProxy*);
    void didFailProvisionalLoadForFrame(WebFrameProxy*);
    void didCommitLoadForFrame(WebFrameProxy*);
    void didFinishLoadForFrame(WebFrameProxy*);
    void didFailLoadForFrame(WebFrameProxy*);
    void didReceiveTitleForFrame(WebFrameProxy*, const WebCore::String&);
    void didFirstLayoutForFrame(WebFrameProxy*);
    void didFirstVisuallyNonEmptyLayoutForFrame(WebFrameProxy*);
    void didStartProgress();
    void didChangeProgress(double);
    void didFinishProgress();
    
    void decidePolicyForNavigationAction(WebFrameProxy*, uint32_t navigationType, const WebCore::String& url, uint64_t listenerID);
    void decidePolicyForNewWindowAction(WebFrameProxy*, uint32_t navigationType, const WebCore::String& url, uint64_t listenerID);
    void decidePolicyForMIMEType(WebFrameProxy*, const WebCore::String& MIMEType, const WebCore::String& url, uint64_t listenerID);

    WebPageProxy* createNewPage();
    void showPage();
    void closePage();
    void runJavaScriptAlert(WebFrameProxy*, const WebCore::String&);

    void didNavigateWithNavigationData(WebFrameProxy*, const WebNavigationDataStore&); 
    void didPerformClientRedirect(WebFrameProxy*, const WebCore::String& sourceURLString, const WebCore::String& destinationURLString);
    void didPerformServerRedirect(WebFrameProxy*, const WebCore::String& sourceURLString, const WebCore::String& destinationURLString);
    void didUpdateHistoryTitle(WebFrameProxy*, const WebCore::String& title, const WebCore::String& url);

    void addItemToBackForwardList(WebBackForwardListItem*);
    void goToItemInBackForwardList(WebBackForwardListItem*);

    void takeFocus(bool direction);
    void setToolTip(const WebCore::String&);
    void setCursor(const WebCore::Cursor&);

    void didReceiveEvent(WebEvent::Type);
    void didRunJavaScriptInMainFrame(const WebCore::String&, uint64_t);
    void didGetRenderTreeExternalRepresentation(const WebCore::String&, uint64_t);


#if PLATFORM(WIN)
    PageClient* m_pageClient;
#else
    // FIXME: This should not be an OwnPtr to the PageClient.  We need to find a better way to clean up m_pageClient on mac.
    // http://bugs.webkit.org/show_bug.cgi?id=40185
    OwnPtr<PageClient> m_pageClient;
#endif
    WebLoaderClient m_loaderClient;
    WebPolicyClient m_policyClient;
    WebUIClient m_uiClient;
    WebHistoryClient m_historyClient;

    OwnPtr<DrawingAreaProxy> m_drawingArea;
    RefPtr<WebPageNamespace> m_pageNamespace;
    RefPtr<WebFrameProxy> m_mainFrame;
    HashMap<uint64_t, RefPtr<WebFrameProxy> > m_frameMap;
    WebCore::String m_pageTitle;

    HashMap<uint64_t, RefPtr<ScriptReturnValueCallback> > m_scriptReturnValueCallbacks;
    HashMap<uint64_t, RefPtr<RenderTreeExternalRepresentationCallback> > m_renderTreeExternalRepresentationCallbacks;

    double m_estimatedProgress;

    // Whether the web page is contained in a top-level window.
    bool m_isInWindow;

    bool m_canGoBack;
    bool m_canGoForward;
    RefPtr<WebBackForwardList> m_backForwardList;

    WebCore::String m_toolTip;

    // REMOVE: For demo purposes only.
    WebCore::String m_urlAtProcessExit;
    
    bool m_valid;
    bool m_closed;

    uint64_t m_pageID;
};

} // namespace WebKit

#endif // WebPageProxy_h
