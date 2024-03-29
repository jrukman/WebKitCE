/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(VIDEO)
#include "HTMLVideoElement.h"

#include "Attribute.h"
#include "CSSHelper.h"
#include "CSSPropertyNames.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "HTMLImageLoader.h"
#include "HTMLNames.h"
#include "Page.h"
#include "RenderImage.h"
#include "RenderVideo.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLVideoElement::HTMLVideoElement(const QualifiedName& tagName, Document* document)
    : HTMLMediaElement(tagName, document)
    , m_shouldDisplayPosterImage(false)
{
    ASSERT(hasTagName(videoTag));
}

PassRefPtr<HTMLVideoElement> HTMLVideoElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLVideoElement(tagName, document));
}

bool HTMLVideoElement::rendererIsNeeded(RenderStyle* style) 
{
    return HTMLElement::rendererIsNeeded(style); 
}

#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
RenderObject* HTMLVideoElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderVideo(this);
}
#endif

void HTMLVideoElement::attach()
{
    HTMLMediaElement::attach();

#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    updatePosterImage();
    if (m_shouldDisplayPosterImage) {
        if (!m_imageLoader)
            m_imageLoader.set(new HTMLImageLoader(this));
        m_imageLoader->updateFromElement();
        if (renderer()) {
            RenderImage* imageRenderer = toRenderImage(renderer());
            imageRenderer->setCachedImage(m_imageLoader->image()); 
        }
    }
#endif
}

void HTMLVideoElement::detach()
{
    HTMLMediaElement::detach();
    
    if (!m_shouldDisplayPosterImage)
        if (m_imageLoader)
            m_imageLoader.clear();
}

void HTMLVideoElement::parseMappedAttribute(Attribute* attr)
{
    const QualifiedName& attrName = attr->name();

    if (attrName == posterAttr) {
        m_posterURL = getNonEmptyURLAttribute(posterAttr);
        updatePosterImage();
        if (m_shouldDisplayPosterImage) {
#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
            if (!m_imageLoader)
                m_imageLoader.set(new HTMLImageLoader(this));
            m_imageLoader->updateFromElementIgnoringPreviousError();
#else
            if (player())
                player()->setPoster(poster());
#endif
        }
    } else if (attrName == widthAttr)
        addCSSLength(attr, CSSPropertyWidth, attr->value());
    else if (attrName == heightAttr)
        addCSSLength(attr, CSSPropertyHeight, attr->value());
    else
        HTMLMediaElement::parseMappedAttribute(attr);
}

bool HTMLVideoElement::supportsFullscreen() const
{
    Page* page = document() ? document()->page() : 0;
    if (!page) 
        return false;

    if (!player() || !player()->supportsFullscreen() || !player()->hasVideo())
        return false;

    // Check with the platform client.
    return page->chrome()->client()->supportsFullscreenForNode(this);
}

unsigned HTMLVideoElement::videoWidth() const
{
    if (!player())
        return 0;
    return player()->naturalSize().width();
}

unsigned HTMLVideoElement::videoHeight() const
{
    if (!player())
        return 0;
    return player()->naturalSize().height();
}

unsigned HTMLVideoElement::width() const
{
    bool ok;
    unsigned w = getAttribute(widthAttr).string().toUInt(&ok);
    return ok ? w : 0;
}
    
unsigned HTMLVideoElement::height() const
{
    bool ok;
    unsigned h = getAttribute(heightAttr).string().toUInt(&ok);
    return ok ? h : 0;
}
    
bool HTMLVideoElement::isURLAttribute(Attribute* attribute) const
{
    return HTMLMediaElement::isURLAttribute(attribute)
        || attribute->name() == posterAttr;
}

const QualifiedName& HTMLVideoElement::imageSourceAttributeName() const
{
    return posterAttr;
}

void HTMLVideoElement::updatePosterImage()
{
#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    bool oldShouldShowPosterImage = m_shouldDisplayPosterImage;
#endif

    m_shouldDisplayPosterImage = !poster().isEmpty() && !hasAvailableVideoFrame();

#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    if (renderer() && oldShouldShowPosterImage != m_shouldDisplayPosterImage)
        renderer()->updateFromElement();
#endif
}

void HTMLVideoElement::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& destRect)
{
    MediaPlayer* player = HTMLMediaElement::player();
    if (!player)
        return;
    
    player->setVisible(true); // Make player visible or it won't draw.
    player->paintCurrentFrameInContext(context, destRect);
}

bool HTMLVideoElement::hasAvailableVideoFrame() const
{
    if (!player())
        return false;
    
    return player()->hasAvailableVideoFrame();
}

void HTMLVideoElement::webkitEnterFullscreen(bool isUserGesture, ExceptionCode& ec)
{
    if (isFullscreen())
        return;

    // Generate an exception if this isn't called in response to a user gesture, or if the 
    // element does not support fullscreen.
    if (!isUserGesture || !supportsFullscreen()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    enterFullscreen();
}

void HTMLVideoElement::webkitExitFullscreen()
{
    if (isFullscreen())
        exitFullscreen();
}

bool HTMLVideoElement::webkitSupportsFullscreen()
{
    return supportsFullscreen();
}

bool HTMLVideoElement::webkitDisplayingFullscreen()
{
    return isFullscreen();
}

void HTMLVideoElement::willMoveToNewOwnerDocument()
{
    if (m_imageLoader)
        m_imageLoader->elementWillMoveToNewOwnerDocument();
    HTMLMediaElement::willMoveToNewOwnerDocument();
}

}

#endif
