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

#include "CommandLine.h"

#include "WebProcessMain.h"

#if PLATFORM(MAC)
#include <objc/objc-auto.h>
#endif

using namespace WebCore;
using namespace WebKit;

#if PLATFORM(MAC)

extern "C" WK_EXPORT int WebKitMain(int argc, char** argv);

int WebKitMain(int argc, char** argv)
{
    ASSERT(!objc_collectingEnabled());
    
    CommandLine commandLine;
    if (!commandLine.parse(argc, argv))
        return EXIT_FAILURE;
    
    String mode = commandLine["mode"];
    if (mode == "legacywebprocess")
        return WebProcessMain(&commandLine);
    
    return EXIT_FAILURE;
}

#elif PLATFORM(WIN)

static void enableTerminationOnHeapCorruption()
{
    // Enable termination on heap corruption on OSes that support it (Vista and XPSP3).
    // http://msdn.microsoft.com/en-us/library/aa366705(VS.85).aspx

    const HEAP_INFORMATION_CLASS heapEnableTerminationOnCorruption = static_cast<HEAP_INFORMATION_CLASS>(1);

    HMODULE hMod = ::GetModuleHandleW(L"kernel32.dll");
    if (!hMod)
        return;

    typedef BOOL (WINAPI*HSI)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
    HSI heapSetInformation = reinterpret_cast<HSI>(::GetProcAddress(hMod, "HeapSetInformation"));
    if (!heapSetInformation)
        return;

    heapSetInformation(0, heapEnableTerminationOnCorruption, 0, 0);
}

extern "C" __declspec(dllexport) 
int WebKitMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpstrCmdLine, int nCmdShow)
{
    enableTerminationOnHeapCorruption();

    CommandLine commandLine;
    if (!commandLine.parse(lpstrCmdLine))
        return EXIT_FAILURE;

    String mode = commandLine["mode"];
    if (mode == "webprocess")
        return WebKit::WebProcessMain(&commandLine);

    return EXIT_FAILURE;
}

#endif
