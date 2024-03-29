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

#include "WKURLCF.h"

#include "WKAPICast.h"
#include <WebCore/PlatformString.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>

using namespace WebCore;
using namespace WebKit;

WKURLRef WKURLCreateWithCFURL(CFURLRef cfURL)
{
    String urlString(CFURLGetString(cfURL));
    RefPtr<StringImpl> urlStringImpl = urlString.impl();
    return toURLRef(urlStringImpl.release().releaseRef());
}

CFURLRef WKURLCopyCFURL(CFAllocatorRef allocatorRef, WKURLRef URLRef)
{
    // We first create a CFString and then create the CFURL from it. This will ensure that the CFURL is stored in 
    // UTF-8 which uses less memory and is what WebKit clients might expect.
    RetainPtr<CFStringRef> urlString(AdoptCF, CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault, reinterpret_cast<const UniChar*>(toWK(URLRef)->characters()), 
                                                                                toWK(URLRef)->length(), kCFAllocatorNull));

    return CFURLCreateWithString(allocatorRef, urlString.get(), 0);
}
