/*
    Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>

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
#include "SVGRectElement.h"

#include "Attribute.h"
#include "RenderPath.h"
#include "SVGLength.h"
#include "SVGNames.h"

namespace WebCore {

SVGRectElement::SVGRectElement(const QualifiedName& tagName, Document *doc)
    : SVGStyledTransformableElement(tagName, doc)
    , SVGTests()
    , SVGLangSpace()
    , SVGExternalResourcesRequired()
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth)
    , m_height(LengthModeHeight)
    , m_rx(LengthModeWidth)
    , m_ry(LengthModeHeight)
{
}

SVGRectElement::~SVGRectElement()
{
}

void SVGRectElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::rxAttr) {
        setRxBaseValue(SVGLength(LengthModeWidth, attr->value()));
        if (rxBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <rx> is not allowed");
    } else if (attr->name() == SVGNames::ryAttr) {
        setRyBaseValue(SVGLength(LengthModeHeight, attr->value()));
        if (ryBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <ry> is not allowed");
    } else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(LengthModeWidth, attr->value()));
        if (widthBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(LengthModeHeight, attr->value()));
        if (heightBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <height> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        SVGStyledTransformableElement::parseMappedAttribute(attr);
    }
}

void SVGRectElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::svgAttributeChanged(attrName);

    bool isLengthAttribute = attrName == SVGNames::xAttr
                          || attrName == SVGNames::yAttr
                          || attrName == SVGNames::widthAttr
                          || attrName == SVGNames::heightAttr
                          || attrName == SVGNames::rxAttr
                          || attrName == SVGNames::ryAttr;

    if (isLengthAttribute)
        updateRelativeLengthsInformation();

    RenderPath* renderer = static_cast<RenderPath*>(this->renderer());
    if (!renderer)
        return;

    if (SVGStyledTransformableElement::isKnownAttribute(attrName)) {
        renderer->setNeedsTransformUpdate();
        renderer->setNeedsLayout(true);
        return;
    }

    if (isLengthAttribute) {
        renderer->setNeedsPathUpdate();
        renderer->setNeedsLayout(true);
        return;
    }

    if (SVGTests::isKnownAttribute(attrName)
        || SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName))
        renderer->setNeedsLayout(true);
}

void SVGRectElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX();
        synchronizeY();
        synchronizeWidth();
        synchronizeHeight();
        synchronizeRx();
        synchronizeRy();
        synchronizeExternalResourcesRequired();
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
    else if (attrName == SVGNames::rxAttr)
        synchronizeRx();
    else if (attrName == SVGNames::ryAttr)
        synchronizeRy();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
}

Path SVGRectElement::toPathData() const
{
    FloatRect rect(x().value(this), y().value(this), width().value(this), height().value(this));

    bool hasRx = hasAttribute(SVGNames::rxAttr);
    bool hasRy = hasAttribute(SVGNames::ryAttr);
    if (hasRx || hasRy) {
        float _rx = hasRx ? rx().value(this) : ry().value(this);
        float _ry = hasRy ? ry().value(this) : rx().value(this);
        return Path::createRoundedRectangle(rect, FloatSize(_rx, _ry));
    }

    return Path::createRectangle(rect);
}

bool SVGRectElement::selfHasRelativeLengths() const
{
    return x().isRelative()
        || y().isRelative()
        || width().isRelative()
        || height().isRelative()
        || rx().isRelative()
        || ry().isRelative();
}

}

#endif // ENABLE(SVG)
