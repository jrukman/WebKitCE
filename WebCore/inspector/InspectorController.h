/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorController_h
#define InspectorController_h

#include "Console.h"
#include "Cookie.h"
#include "InspectorDOMAgent.h"
#include "PlatformString.h"
#include "ScriptArray.h"
#include "ScriptBreakpoint.h"
#include "ScriptObject.h"
#include "ScriptProfile.h"
#include "ScriptState.h"
#include "ScriptValue.h"
#include "StringHash.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

#if ENABLE(JAVASCRIPT_DEBUGGER)
#include "ScriptDebugListener.h"
#endif

namespace WebCore {

class CachedResource;
class ConsoleMessage;
class Database;
class Document;
class DocumentLoader;
class Element;
class GraphicsContext;
class HitTestResult;
class InjectedScript;
class InjectedScriptHost;
class InspectorBackend;
class InspectorClient;
class InspectorCSSStore;
class InspectorDOMStorageResource;
class InspectorDatabaseResource;
class InspectorFrontend;
class InspectorFrontendClient;
class InspectorResource;
class InspectorTimelineAgent;
class InspectorValue;
class InspectorWorkerResource;
class KURL;
class Node;
class Page;
class RemoteInspectorFrontend;
class ResourceRequest;
class ResourceResponse;
class ResourceError;
class ScriptCallStack;
class ScriptString;
class SharedBuffer;
class Storage;
class StorageArea;

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
class InspectorApplicationCacheAgent;
#endif

class InspectorController
#if ENABLE(JAVASCRIPT_DEBUGGER)
                          : ScriptDebugListener, public Noncopyable
#else
                          : public Noncopyable
#endif
                                                    {
public:
    typedef HashMap<unsigned long, RefPtr<InspectorResource> > ResourcesMap;
    typedef HashMap<RefPtr<Frame>, ResourcesMap*> FrameResourcesMap;
    typedef HashMap<int, RefPtr<InspectorDatabaseResource> > DatabaseResourcesMap;
    typedef HashMap<int, RefPtr<InspectorDOMStorageResource> > DOMStorageResourcesMap;

    typedef enum {
        AuditsPanel,
        CurrentPanel,
        ConsolePanel,
        ElementsPanel,
        ResourcesPanel,
        ScriptsPanel,
        TimelinePanel,
        ProfilesPanel,
        StoragePanel
    } SpecialPanels;

    InspectorController(Page*, InspectorClient*);
    ~InspectorController();

    InspectorBackend* inspectorBackend() { return m_inspectorBackend.get(); }
    InjectedScriptHost* injectedScriptHost() { return m_injectedScriptHost.get(); }

    void inspectedPageDestroyed();

    bool enabled() const;

    Page* inspectedPage() const { return m_inspectedPage; }
    void reloadPage();

    String setting(const String& key) const;
    void setSetting(const String& key, const String& value);
    void saveApplicationSettings(const String& settings);
    void saveSessionSettings(const String&);


    void inspect(Node*);
    void highlight(Node*);
    void hideHighlight();
    void highlightDOMNode(long nodeId);
    void hideDOMNodeHighlight() { hideHighlight(); }

    void show();
    void showPanel(SpecialPanels);
    void close();

    // We are in transition from JS transport via webInspector to native
    // transport via InspectorClient. After migration, webInspector parameter should
    // be removed.
    void connectFrontend(const ScriptObject& webInspector);
    void disconnectFrontend();

    void addMessageToConsole(MessageSource, MessageType, MessageLevel, ScriptCallStack*);
    void addMessageToConsole(MessageSource, MessageType, MessageLevel, const String& message, unsigned lineNumber, const String& sourceID);
    void clearConsoleMessages();
    const Vector<OwnPtr<ConsoleMessage> >& consoleMessages() const { return m_consoleMessages; }

    bool searchingForNodeInPage() const { return m_searchingForNode; }
    void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags);
    void handleMousePress();

    void setInspectorFrontendClient(PassOwnPtr<InspectorFrontendClient> client);
    bool hasInspectorFrontendClient() const { return m_inspectorFrontendClient; }

    void inspectedWindowScriptObjectCleared(Frame*);

    bool windowVisible();

    void didCommitLoad(DocumentLoader*);
    void frameDetachedFromParent(Frame*);
    void didLoadResourceFromMemoryCache(DocumentLoader*, const CachedResource*);

    void identifierForInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&);
    void willSendRequest(unsigned long identifier, ResourceRequest&, const ResourceResponse& redirectResponse);
    void didReceiveResponse(unsigned long identifier, const ResourceResponse&);
    void didReceiveContentLength(unsigned long identifier, int lengthReceived);
    void didFinishLoading(unsigned long identifier);
    void didFailLoading(unsigned long identifier, const ResourceError&);
    void resourceRetrievedByXMLHttpRequest(unsigned long identifier, const ScriptString& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber);
    void scriptImported(unsigned long identifier, const String& sourceString);

    void enableResourceTracking(bool always = false, bool reload = true);
    void disableResourceTracking(bool always = false);
    bool resourceTrackingEnabled() const { return m_resourceTrackingEnabled; }

    void ensureSettingsLoaded();

    void startTimelineProfiler();
    void stopTimelineProfiler();
    InspectorTimelineAgent* timelineAgent() { return m_timelineAgent.get(); }

    void getCookies(long callId);
    void deleteCookie(const String& cookieName, const String& domain);

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    InspectorApplicationCacheAgent* applicationCacheAgent() { return m_applicationCacheAgent.get(); }
#endif

    void mainResourceFiredLoadEvent(DocumentLoader*, const KURL&);
    void mainResourceFiredDOMContentEvent(DocumentLoader*, const KURL&);

    void didInsertDOMNode(Node*);
    void didRemoveDOMNode(Node*);
    void didModifyDOMAttr(Element*);
#if ENABLE(WORKERS)
    enum WorkerAction { WorkerCreated, WorkerDestroyed };

    void postWorkerNotificationToFrontend(const InspectorWorkerResource&, WorkerAction);
    void didCreateWorker(intptr_t, const String& url, bool isSharedWorker);
    void didDestroyWorker(intptr_t);
#endif

#if ENABLE(DATABASE)
    void didOpenDatabase(Database*, const String& domain, const String& name, const String& version);
#endif
#if ENABLE(DOM_STORAGE)
    void didUseDOMStorage(StorageArea* storageArea, bool isLocalStorage, Frame* frame);
    void selectDOMStorage(Storage* storage);
    void getDOMStorageEntries(long callId, long storageId);
    void setDOMStorageItem(long callId, long storageId, const String& key, const String& value);
    void removeDOMStorageItem(long callId, long storageId, const String& key);
#endif

    const ResourcesMap& resources() const { return m_resources; }
    InspectorResource* resourceForURL(const String& url);
    InspectorFrontend* inspectorFrontend() { return m_frontend.get(); }
    RemoteInspectorFrontend* remoteInspectorFrontend() { return m_remoteFrontend.get(); }

    void drawNodeHighlight(GraphicsContext&) const;

    void count(const String& title, unsigned lineNumber, const String& sourceID);

    void startTiming(const String& title);
    bool stopTiming(const String& title, double& elapsed);

    void startGroup(MessageSource source, ScriptCallStack* callFrame, bool collapsed = false);
    void endGroup(MessageSource source, unsigned lineNumber, const String& sourceURL);

    void markTimeline(const String& message);

#if ENABLE(JAVASCRIPT_DEBUGGER)
    void addProfile(PassRefPtr<ScriptProfile>, unsigned lineNumber, const String& sourceURL);
    void addProfileFinishedMessageToConsole(PassRefPtr<ScriptProfile>, unsigned lineNumber, const String& sourceURL);
    void addStartProfilingMessageToConsole(const String& title, unsigned lineNumber, const String& sourceURL);
    void removeProfile(unsigned);
    void clearProfiles();

    bool isRecordingUserInitiatedProfile() const { return m_recordingUserInitiatedProfile; }

    String getCurrentUserInitiatedProfileName(bool incrementProfileNumber);
    void startUserInitiatedProfiling(Timer<InspectorController>* = 0);
    void stopUserInitiatedProfiling();
    void startProfiling() { startUserInitiatedProfiling(); }
    void stopProfiling() { stopUserInitiatedProfiling(); }

    void enableProfiler(bool always = false, bool skipRecompile = false);
    void disableProfiler(bool always = false);
    bool profilerEnabled() const { return enabled() && m_profilerEnabled; }

    void takeHeapSnapshot();

    void enableDebugger();
    void disableDebugger(bool always = false);
    bool debuggerEnabled() const { return m_debuggerEnabled; }

    void editScriptSource(long callId, const String& sourceID, const String& newContent);
    void getScriptSource(long callId, const String& sourceID);

    void resume();
    void setPauseOnExceptionsState(long pauseState);
    PassRefPtr<SerializedScriptValue> currentCallFrames();

    virtual void didParseSource(const String& sourceID, const String& url, const String& data, int firstLine, ScriptWorldType);
    virtual void failedToParseSource(const String& url, const String& data, int firstLine, int errorLine, const String& errorMessage);
    virtual void didPause(ScriptState*);
    virtual void didContinue();
#endif

    void evaluateForTestInFrontend(long callId, const String& script);

    InjectedScript injectedScriptForNodeId(long id);
    void addScriptToEvaluateOnLoad(const String& source);
    void removeAllScriptsToEvaluateOnLoad();

    static const String& inspectorStartsAttachedSettingName();

private:
    static const String& frontendSettingsSettingName();

    friend class InspectorBackend;
    friend class InjectedScriptHost;

    void populateScriptObjects();
    void unbindAllResources();

    // Following are used from InspectorBackend and internally.
    void setSearchingForNode(bool enabled);
    void enableSearchingForNode() { setSearchingForNode(true); }
    void disableSearchingForNode() { setSearchingForNode(false); }

    void setMonitoringXHR(bool enabled);
    void enableMonitoringXHR() { setMonitoringXHR(true); }
    void disableMonitoringXHR() { setMonitoringXHR(false); }
    void storeLastActivePanel(const String& panelName);
    InspectorDOMAgent* domAgent() { return m_domAgent.get(); }
    void releaseFrontendLifetimeAgents();

#if ENABLE(JAVASCRIPT_DEBUGGER)
    void setBreakpoint(long callId, const String& sourceID, unsigned lineNumber, bool enabled, const String& condition);
    void removeBreakpoint(const String& sourceID, unsigned lineNumber);

    typedef HashMap<unsigned int, RefPtr<ScriptProfile> > ProfilesMap;

    void startUserInitiatedProfilingSoon();
    void toggleRecordButton(bool);
    void enableDebuggerFromFrontend(bool always);
    void getProfileHeaders(long callId);
    void getProfile(long callId, unsigned uid);
    ScriptObject createProfileHeader(const ScriptProfile& profile);
#endif
#if ENABLE(DATABASE)
    void selectDatabase(Database* database);
    Database* databaseForId(long databaseId);
#endif
#if ENABLE(DOM_STORAGE)
    InspectorDOMStorageResource* getDOMStorageResourceForId(long storageId);
#endif

    ScriptObject buildObjectForCookie(const Cookie&);
    ScriptArray buildArrayForCookies(ListHashSet<Cookie>&);

    void focusNode();

    void addConsoleMessage(ScriptState*, PassOwnPtr<ConsoleMessage>);

    void addResource(InspectorResource*);
    void removeResource(InspectorResource*);
    InspectorResource* getTrackedResource(unsigned long identifier);
    void getResourceContent(long callId, unsigned long identifier);

    void pruneResources(ResourcesMap*, DocumentLoader* loaderToKeep = 0);
    void removeAllResources(ResourcesMap* map) { pruneResources(map); }

    bool isMainResourceLoader(DocumentLoader* loader, const KURL& requestUrl);

    SpecialPanels specialPanelForJSName(const String& panelName);

    void didEvaluateForTestInFrontend(long callId, const String& jsonResult);

#if ENABLE(JAVASCRIPT_DEBUGGER)
    String breakpointsSettingKey();
    void loadBreakpoints();
    void saveBreakpoints();
#endif

    Page* m_inspectedPage;
    InspectorClient* m_client;
    OwnPtr<InspectorFrontendClient> m_inspectorFrontendClient;
    bool m_openingFrontend;
    OwnPtr<InspectorFrontend> m_frontend;
    OwnPtr<RemoteInspectorFrontend> m_remoteFrontend;
    RefPtr<InspectorDOMAgent> m_domAgent;
    OwnPtr<InspectorCSSStore> m_cssStore;
    OwnPtr<InspectorTimelineAgent> m_timelineAgent;

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    OwnPtr<InspectorApplicationCacheAgent> m_applicationCacheAgent;
#endif

    RefPtr<Node> m_nodeToFocus;
    RefPtr<InspectorResource> m_mainResource;
    ResourcesMap m_resources;
    HashSet<String> m_knownResources;
    FrameResourcesMap m_frameResources;
    Vector<OwnPtr<ConsoleMessage> > m_consoleMessages;
    unsigned m_expiredConsoleMessageCount;
    HashMap<String, double> m_times;
    HashMap<String, unsigned> m_counts;
#if ENABLE(DATABASE)
    DatabaseResourcesMap m_databaseResources;
#endif
#if ENABLE(DOM_STORAGE)
    DOMStorageResourcesMap m_domStorageResources;
#endif
    SpecialPanels m_showAfterVisible;
    RefPtr<Node> m_highlightedNode;
#if ENABLE(INSPECTOR)
    RefPtr<InspectorValue> m_sessionSettings;
#endif
    unsigned m_groupLevel;
    bool m_searchingForNode;
    bool m_monitoringXHR;
    ConsoleMessage* m_previousMessage;
    bool m_resourceTrackingEnabled;
    bool m_settingsLoaded;
    RefPtr<InspectorBackend> m_inspectorBackend;
    RefPtr<InjectedScriptHost> m_injectedScriptHost;

    typedef HashMap<String, String> Settings;
    mutable Settings m_settings;

    Vector<pair<long, String> > m_pendingEvaluateTestCommands;
    Vector<String> m_scriptsToEvaluateOnLoad;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    bool m_debuggerEnabled;
    bool m_attachDebuggerWhenShown;
    ScriptState* m_pausedScriptState;
    HashMap<String, String> m_sourceIDToURL;
    HashMap<String, String> m_scriptIDToContent;
    HashMap<String, SourceBreakpoints> m_stickyBreakpoints;
    HashMap<String, unsigned> m_breakpointsMapping;
    bool m_breakpointsLoaded;

    bool m_profilerEnabled;
    bool m_recordingUserInitiatedProfile;
    int m_currentUserInitiatedProfileNumber;
    unsigned m_nextUserInitiatedProfileNumber;
    Timer<InspectorController> m_startProfiling;
    ProfilesMap m_profiles;
#endif
#if ENABLE(WORKERS)
    typedef HashMap<intptr_t, RefPtr<InspectorWorkerResource> > WorkersMap;

    WorkersMap m_workers;
#endif
};

inline void InspectorController::didInsertDOMNode(Node* node)
{
    if (m_domAgent)
        m_domAgent->didInsertDOMNode(node);
}

inline void InspectorController::didRemoveDOMNode(Node* node)
{
    if (m_domAgent)
        m_domAgent->didRemoveDOMNode(node);
}

inline void InspectorController::didModifyDOMAttr(Element* element)
{
    if (m_domAgent)
        m_domAgent->didModifyDOMAttr(element);
}

} // namespace WebCore

#endif // !defined(InspectorController_h)
