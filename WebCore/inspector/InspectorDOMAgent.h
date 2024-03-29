/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef InspectorDOMAgent_h
#define InspectorDOMAgent_h

#include "AtomicString.h"
#include "Document.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "InspectorCSSStore.h"
#include "InspectorValues.h"
#include "NodeList.h"
#include "ScriptState.h"
#include "Timer.h"

#include <wtf/Deque.h>
#include <wtf/ListHashSet.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    class ContainerNode;
    class CSSRule;
    class CSSRuleList;
    class CSSStyleDeclaration;
    class CSSStyleRule;
    class CSSStyleSheet;
    class Element;
    class Event;
    class RemoteInspectorFrontend;
    class MatchJob;
    class NameNodeMap;
    class Node;
    class Page;

    struct EventListenerInfo {
        EventListenerInfo(Node* node, const AtomicString& eventType, const EventListenerVector& eventListenerVector)
            : node(node)
            , eventType(eventType)
            , eventListenerVector(eventListenerVector)
        {
        }

        Node* node;
        const AtomicString eventType;
        const EventListenerVector eventListenerVector;
    };

    class InspectorDOMAgent : public EventListener {
    public:
        static PassRefPtr<InspectorDOMAgent> create(InspectorCSSStore* cssStore, RemoteInspectorFrontend* frontend)
        {
            return adoptRef(new InspectorDOMAgent(cssStore, frontend));
        }

        static const InspectorDOMAgent* cast(const EventListener* listener)
        {
            return listener->type() == InspectorDOMAgentType
                ? static_cast<const InspectorDOMAgent*>(listener)
                : 0;
        }

        InspectorDOMAgent(InspectorCSSStore* cssStore, RemoteInspectorFrontend* frontend);
        ~InspectorDOMAgent();

        void reset();

        virtual bool operator==(const EventListener& other);

        // Methods called from the frontend for DOM nodes inspection.
        void getChildNodes(long callId, long nodeId);
        void setAttribute(long callId, long elementId, const String& name, const String& value);
        void removeAttribute(long callId, long elementId, const String& name);
        void removeNode(long callId, long nodeId);
        void changeTagName(long callId, long nodeId, const String& tagName);
        void getOuterHTML(long callId, long nodeId);
        void setOuterHTML(long callId, long nodeId, const String& outerHTML);
        void setTextNodeValue(long callId, long nodeId, const String& value);
        void getEventListenersForNode(long callId, long nodeId);
        void addInspectedNode(long nodeId);
        void performSearch(const String& whitespaceTrimmedQuery, bool runSynchronously);
        void searchCanceled();

        // Methods called from the frontend for CSS styles inspection.
        void getStyles(long callId, long nodeId, bool authorOnly);
        void getAllStyles(long callId);
        void getInlineStyle(long callId, long nodeId);
        void getComputedStyle(long callId, long nodeId);
        void getStyleSheet(long callId, long styleSheetId);
        void getRuleRangesForStyleSheetId(long callId, long styleSheetId);
        void applyStyleText(long callId, long styleId, const String& styleText, const String& propertyName);
        void setStyleText(long callId, long styleId, const String& cssText);
        void setStyleProperty(long callId, long styleId, const String& name, const String& value);
        void toggleStyleEnabled(long callId, long styleId, const String& propertyName, bool disabled);
        void setRuleSelector(long callId, long ruleId, const String& selector, long selectedNodeId);
        void addRule(long callId, const String& selector, long selectedNodeId);

        // Methods called from the InspectorController.
        void setDocument(Document* document);
        void releaseDanglingNodes();

        void didInsertDOMNode(Node*);
        void didRemoveDOMNode(Node*);
        void didModifyDOMAttr(Element*);

        Node* nodeForId(long nodeId);
        long pushNodePathToFrontend(Node* node);
        void pushChildNodesToFrontend(long nodeId);
        void pushNodeByPathToFrontend(long callId, const String& path);
        long inspectedNode(unsigned long num);
        void copyNode(long nodeId);

    private:
        static CSSStyleSheet* getParentStyleSheet(CSSStyleDeclaration*);
        void startListening(Document* document);
        void stopListening(Document* document);

        virtual void handleEvent(ScriptExecutionContext*, Event* event);

        // Node-related methods.
        typedef HashMap<RefPtr<Node>, long> NodeToIdMap;
        long bind(Node* node, NodeToIdMap* nodesMap);
        void unbind(Node* node, NodeToIdMap* nodesMap);

        bool pushDocumentToFrontend();

        PassRefPtr<InspectorObject> buildObjectForAttributeStyles(Element* element);
        PassRefPtr<InspectorArray> buildArrayForCSSRules(Document* ownerDocument, CSSRuleList*);
        PassRefPtr<InspectorArray> buildArrayForPseudoElements(Element* element, bool authorOnly);

        PassRefPtr<InspectorObject> buildObjectForNode(Node* node, int depth, NodeToIdMap* nodesMap);
        PassRefPtr<InspectorArray> buildArrayForElementAttributes(Element* element);
        PassRefPtr<InspectorArray> buildArrayForContainerChildren(Node* container, int depth, NodeToIdMap* nodesMap);
        PassRefPtr<InspectorObject> buildObjectForEventListener(const RegisteredEventListener& registeredEventListener, const AtomicString& eventType, Node* node);

        // We represent embedded doms as a part of the same hierarchy. Hence we treat children of frame owners differently.
        // We also skip whitespace text nodes conditionally. Following methods encapsulate these specifics.
        Node* innerFirstChild(Node* node);
        Node* innerNextSibling(Node* node);
        Node* innerPreviousSibling(Node* node);
        unsigned innerChildNodeCount(Node* node);
        Node* innerParentNode(Node* node);
        bool isWhitespace(Node* node);

        Document* mainFrameDocument() const;
        String documentURLString(Document* document) const;
        InspectorCSSStore* cssStore() { return m_cssStore; }

        void onMatchJobsTimer(Timer<InspectorDOMAgent>*);
        void reportNodesAsSearchResults(ListHashSet<Node*>& resultCollector);

        PassRefPtr<InspectorObject> buildObjectForStyle(CSSStyleDeclaration*, bool bind);
        void populateObjectWithStyleProperties(CSSStyleDeclaration*, InspectorObject* result);
        PassRefPtr<InspectorArray> buildArrayForDisabledStyleProperties(DisabledStyleDeclaration*);
        PassRefPtr<InspectorObject> buildObjectForRule(Document* ownerDocument, CSSStyleRule*);
        PassRefPtr<InspectorObject> buildObjectForStyleSheet(Document* ownerDocument, CSSStyleSheet*);
        Vector<String> longhandProperties(CSSStyleDeclaration*, const String& shorthandProperty);
        String shorthandValue(CSSStyleDeclaration*, const String& shorthandProperty);
        String shorthandPriority(CSSStyleDeclaration*, const String& shorthandProperty);
        bool ruleAffectsNode(CSSStyleRule*, Node*);
        Node* nodeForPath(const String& path);
        PassRefPtr<InspectorArray> toArray(const Vector<String>& data);

        void discardBindings();

        InspectorCSSStore* m_cssStore;
        RemoteInspectorFrontend* m_frontend;
        NodeToIdMap m_documentNodeToIdMap;
        // Owns node mappings for dangling nodes.
        Vector<NodeToIdMap*> m_danglingNodeToIdMaps;
        HashMap<long, Node*> m_idToNode;
        HashMap<long, NodeToIdMap*> m_idToNodesMap;
        HashSet<long> m_childrenRequested;
        long m_lastNodeId;
        ListHashSet<RefPtr<Document> > m_documents;
        Deque<MatchJob*> m_pendingMatchJobs;
        Timer<InspectorDOMAgent> m_matchJobsTimer;
        HashSet<RefPtr<Node> > m_searchResults;
        Vector<long> m_inspectedNodes;
    };

} // namespace WebCore

#endif // !defined(InspectorDOMAgent_h)
