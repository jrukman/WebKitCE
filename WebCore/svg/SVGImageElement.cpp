/*
    Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006, 2007, 2008, 2009 Rob Buis <buis@kde.org>
                  2006 Alexander Kellett <lypanov@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(SVG)
#include "SVGImageElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "RenderSVGImage.h"
#include "SVGDocument.h"
#include "SVGLength.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGSVGElement.h"
#include "XLinkNames.h"

namespace WebCore {

SVGImageElement::SVGImageElement(const QualifiedName& tagName, Document* doc)
    : SVGStyledTransformableElement(tagName, doc)
    , SVGTests()
    , SVGLangSpace()
    , SVGExternalResourcesRequired()
    , SVGURIReference()
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth)
    , m_height(LengthModeHeight)
    , m_imageLoader(this)
{
}

SVGImageElement::~SVGImageElement()
{
}

void SVGImageElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::preserveAspectRatioAttr)
        SVGPreserveAspectRatio::parsePreserveAspectRatio(this, attr->value());
    else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(LengthModeWidth, attr->value()));
        addCSSProperty(attr, CSSPropertyWidth, attr->value());
        if (widthBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for image attribute <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(LengthModeHeight, attr->value()));
        addCSSProperty(attr, CSSPropertyHeight, attr->value());
        if (heightBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for image attribute <height> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        if (SVGURIReference::parseMappedAttribute(attr))
            return;
        SVGStyledTransformableElement::parseMappedAttribute(attr);
    }
}

void SVGImageElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::svgAttributeChanged(attrName);

    if (SVGURIReference::isKnownAttribute(attrName))
        m_imageLoader.updateFromElementIgnoringPreviousError();

    bool isLengthAttribute = attrName == SVGNames::xAttr
                          || attrName == SVGNames::yAttr
                          || attrName == SVGNames::widthAttr
                          || attrName == SVGNames::heightAttr;

    if (isLengthAttribute)
        updateRelativeLengthsInformation();

    RenderObject* renderer = this->renderer();
    if (!renderer)
        return;

    if (SVGStyledTransformableElement::isKnownAttribute(attrName)) {
        renderer->setNeedsTransformUpdate();
        renderer->setNeedsLayout(true);
        return;
    }

    if (isLengthAttribute
        || attrName == SVGNames::preserveAspectRatioAttr
        || SVGTests::isKnownAttribute(attrName)
        || SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName))
        renderer->setNeedsLayout(true);
}

void SVGImageElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX();
        synchronizeY();
        synchronizeWidth();
        synchronizeHeight();
        synchronizePreserveAspectRatio();
        synchronizeExternalResourcesRequired();
        synchronizeHref();
        return;
    }

    if (attrName == SVGNames::xAttr)
        synchronizeX();
    else if (attrName == SVGNames::yAttr)
        synchronizeY();
    else if (attrName == SVGNames::widthAttr)
        synchronizeWidth();
    else if (attrName == SVGNames::heightAttr)
        synchronizeHeight();
    else if (attrName == SVGNames::preserveAspectRatioAttr)
        synchronizePreserveAspectRatio();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGURIReference::isKnownAttribute(attrName))
        synchronizeHref();
}

bool SVGImageElement::selfHasRelativeLengths() const
{
    return x().isRelative()
        || y().isRelative()
        || width().isRelative()
        || height().isRelative();
}

RenderObject* SVGImageElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGImage(this);
}

bool SVGImageElement::haveLoadedRequiredResources()
{
    return !externalResourcesRequiredBaseValue() || m_imageLoader.haveFiredLoadEvent();
}

void SVGImageElement::attach()
{
    SVGStyledTransformableElement::attach();

    if (RenderImage* imageObj = toRenderImage(renderer())) {
        if (imageObj->hasImage())
            return;

        imageObj->setCachedImage(m_imageLoader.image());
    }
}

void SVGImageElement::insertedIntoDocument()
{
    SVGStyledTransformableElement::insertedIntoDocument();

    // Update image loader, as soon as we're living in the tree.
    // We can only resolve base URIs properly, after that!
    m_imageLoader.updateFromElement();
}

const QualifiedName& SVGImageElement::imageSourceAttributeName() const
{
    return XLinkNames::hrefAttr;
}

void SVGImageElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    SVGStyledTransformableElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document()->completeURL(href()));
}

void SVGImageElement::willMoveToNewOwnerDocument()
{
    m_imageLoader.elementWillMoveToNewOwnerDocument();
    SVGStyledTransformableElement::willMoveToNewOwnerDocument();
}

}

#endif // ENABLE(SVG)
