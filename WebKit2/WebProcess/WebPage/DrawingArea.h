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

#ifndef DrawingArea_h
#define DrawingArea_h

#include <WebCore/IntRect.h>

namespace WebCore {
    class IntRect;
    class IntSize;
#if USE(ACCELERATED_COMPOSITING)
    class GraphicsLayer;
#endif
}

namespace CoreIPC {
    class ArgumentDecoder;
    class Connection;
    class MessageID;
}

namespace WebKit {

class WebPage;

class DrawingArea {
public:
    enum Type {
        ChunkedUpdateDrawingAreaType,
#if USE(ACCELERATED_COMPOSITING)
        LayerBackedDrawingAreaType,
#endif
    };

    // FIXME: It might make sense to move this create function into a factory style class. 
    static DrawingArea* create(Type, WebPage*);
    virtual ~DrawingArea();
    
    virtual void invalidateWindow(const WebCore::IntRect& rect, bool immediate) = 0;
    virtual void invalidateContentsAndWindow(const WebCore::IntRect& rect, bool immediate) = 0;
    virtual void invalidateContentsForSlowScroll(const WebCore::IntRect& rect, bool immediate) = 0;
    virtual void scroll(const WebCore::IntSize& scrollDelta, const WebCore::IntRect& rectToScroll, const WebCore::IntRect& clipRect) = 0;

    virtual void setNeedsDisplay(const WebCore::IntRect&) = 0;
    virtual void display() = 0;

#if USE(ACCELERATED_COMPOSITING)
    virtual void attachCompositingContext(WebCore::GraphicsLayer*) = 0;
    virtual void detachCompositingContext() = 0;
    virtual void scheduleCompositingLayerSync() = 0;
    virtual void syncCompositingLayers() = 0;
#endif

    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder&) = 0;

    // The DrawingArea should never be encoded itself. Instead, the DrawingAreaProxy should be encoded.
    // The DrawingArea should also never be decoded itself. Instead, the DrawingArea::Type should be decoded.

protected:
    DrawingArea(Type, WebPage*);

    Type m_type;
    WebPage* m_webPage;
};

} // namespace WebKit

#endif // DrawingArea_h
