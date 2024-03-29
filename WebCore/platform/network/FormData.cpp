/*
 * Copyright (C) 2004, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "FormData.h"

#include "BlobItem.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "FileSystem.h"
#include "FormDataBuilder.h"
#include "MIMETypeRegistry.h"
#include "Page.h"
#include "TextEncoding.h"

namespace WebCore {

#if ENABLE(BLOB_SLICE)
const long long FormDataElement::toEndOfFile = -1;
const double FormDataElement::doNotCheckFileChange = 0;
#endif

inline FormData::FormData()
    : m_identifier(0)
    , m_hasGeneratedFiles(false)
    , m_alwaysStream(false)
{
}

inline FormData::FormData(const FormData& data)
    : RefCounted<FormData>()
    , m_elements(data.m_elements)
    , m_identifier(data.m_identifier)
    , m_hasGeneratedFiles(false)
    , m_alwaysStream(false)
{
    // We shouldn't be copying FormData that hasn't already removed its generated files
    // but just in case, make sure the new FormData is ready to generate its own files.
    if (data.m_hasGeneratedFiles) {
        size_t n = m_elements.size();
        for (size_t i = 0; i < n; ++i) {
            FormDataElement& e = m_elements[i];
            if (e.m_type == FormDataElement::encodedFile)
                e.m_generatedFilename = String();
        }
    }
}

FormData::~FormData()
{
    // This cleanup should've happened when the form submission finished.
    // Just in case, let's assert, and do the cleanup anyway in release builds.
    ASSERT(!m_hasGeneratedFiles);
    removeGeneratedFilesIfNeeded();
}

PassRefPtr<FormData> FormData::create()
{
    return adoptRef(new FormData);
}

PassRefPtr<FormData> FormData::create(const void* data, size_t size)
{
    RefPtr<FormData> result = create();
    result->appendData(data, size);
    return result.release();
}

PassRefPtr<FormData> FormData::create(const CString& string)
{
    RefPtr<FormData> result = create();
    result->appendData(string.data(), string.length());
    return result.release();
}

PassRefPtr<FormData> FormData::create(const Vector<char>& vector)
{
    RefPtr<FormData> result = create();
    result->appendData(vector.data(), vector.size());
    return result.release();
}

PassRefPtr<FormData> FormData::create(const BlobItemList& items, const TextEncoding& encoding)
{
    RefPtr<FormData> result = create();
    result->appendKeyValuePairItems(items, encoding, false, 0);
    return result.release();
}

PassRefPtr<FormData> FormData::createMultiPart(const BlobItemList& items, const TextEncoding& encoding, Document* document)
{
    RefPtr<FormData> result = create();
    result->appendKeyValuePairItems(items, encoding, true, document);
    return result.release();
}

PassRefPtr<FormData> FormData::copy() const
{
    return adoptRef(new FormData(*this));
}

PassRefPtr<FormData> FormData::deepCopy() const
{
    RefPtr<FormData> formData(create());

    formData->m_alwaysStream = m_alwaysStream;

    size_t n = m_elements.size();
    formData->m_elements.reserveInitialCapacity(n);
    for (size_t i = 0; i < n; ++i) {
        const FormDataElement& e = m_elements[i];
        switch (e.m_type) {
        case FormDataElement::data:
            formData->m_elements.append(FormDataElement(e.m_data));
            break;
        case FormDataElement::encodedFile:
#if ENABLE(BLOB_SLICE)
            formData->m_elements.append(FormDataElement(e.m_filename, e.m_fileStart, e.m_fileLength, e.m_expectedFileModificationTime, e.m_shouldGenerateFile));
#else
            formData->m_elements.append(FormDataElement(e.m_filename, e.m_shouldGenerateFile));
#endif
            break;
        }
    }
    return formData.release();
}

void FormData::appendData(const void* data, size_t size)
{
    if (m_elements.isEmpty() || m_elements.last().m_type != FormDataElement::data)
        m_elements.append(FormDataElement());
    FormDataElement& e = m_elements.last();
    size_t oldSize = e.m_data.size();
    e.m_data.grow(oldSize + size);
    memcpy(e.m_data.data() + oldSize, data, size);
}

void FormData::appendFile(const String& filename, bool shouldGenerateFile)
{
#if ENABLE(BLOB_SLICE)
    m_elements.append(FormDataElement(filename, 0, FormDataElement::toEndOfFile, FormDataElement::doNotCheckFileChange, shouldGenerateFile));
#else
    m_elements.append(FormDataElement(filename, shouldGenerateFile));
#endif
}

void FormData::appendItems(const BlobItemList& items)
{
    for (BlobItemList::const_iterator iter(items.begin()); iter != items.end(); ++iter)
        appendItem(iter->get(), false);
}

void FormData::appendItem(const BlobItem* item, bool shouldGenerateFile)
{
    const DataBlobItem* dataItem = item->toDataBlobItem();
    if (dataItem) {
        appendData(dataItem->data(), static_cast<size_t>(dataItem->size()));
        return;
    }

    const FileBlobItem* fileItem = item->toFileBlobItem();
    ASSERT(fileItem);
    if (fileItem->path().isEmpty()) {
        // If the path is empty do not add the item.
        return;
    }

#if ENABLE(BLOB_SLICE)
    const FileRangeBlobItem* fileRangeItem = item->toFileRangeBlobItem();
    if (fileRangeItem) {
        appendFileRange(fileItem->path(), fileRangeItem->start(), fileRangeItem->size(), fileRangeItem->snapshotModificationTime(), shouldGenerateFile);
        return;
    }
#endif

    appendFile(fileItem->path(), shouldGenerateFile);
}

#if ENABLE(BLOB_SLICE)
void FormData::appendFileRange(const String& filename, long long start, long long length, double expectedModificationTime, bool shouldGenerateFile)
{
    m_elements.append(FormDataElement(filename, start, length, expectedModificationTime, shouldGenerateFile));
}
#endif

void FormData::appendKeyValuePairItems(const BlobItemList& items, const TextEncoding& encoding, bool isMultiPartForm, Document* document)
{
    if (isMultiPartForm)
        m_boundary = FormDataBuilder::generateUniqueBoundaryString();

    Vector<char> encodedData;

    size_t formDataListSize = items.size();
    ASSERT(!(formDataListSize % 2));
    for (size_t i = 0; i < formDataListSize; i += 2) {
        const StringBlobItem* key = items[i]->toStringBlobItem();
        const BlobItem* value = items[i + 1].get();
        ASSERT(key);
        if (isMultiPartForm) {
            Vector<char> header;
            FormDataBuilder::beginMultiPartHeader(header, m_boundary.data(), key->cstr());

            bool shouldGenerateFile = false;
            // If the current type is FILE, then we also need to include the filename
            const FileBlobItem* fileItem = value->toFileBlobItem();
            if (fileItem) {
                const String& path = fileItem->path();
                String fileName = fileItem->name();

                // Let the application specify a filename if it's going to generate a replacement file for the upload.
                if (!path.isEmpty()) {
                    if (Page* page = document->page()) {
                        String generatedFileName;
                        shouldGenerateFile = page->chrome()->client()->shouldReplaceWithGeneratedFileForUpload(path, generatedFileName);
                        if (shouldGenerateFile)
                            fileName = generatedFileName;
                    }
                }

                // We have to include the filename=".." part in the header, even if the filename is empty
                FormDataBuilder::addFilenameToMultiPartHeader(header, encoding, fileName);

                // If the item is sliced from a file, do not add the content type.
#if ENABLE(BLOB_SLICE)
                if (!fileName.isEmpty() && !value->toFileRangeBlobItem()) {
#else
                if (!fileName.isEmpty()) {
#endif
                    // FIXME: The MIMETypeRegistry function's name makes it sound like it takes a path,
                    // not just a basename. But filename is not the path. But note that it's not safe to
                    // just use path instead since in the generated-file case it will not reflect the
                    // MIME type of the generated file.
                    String mimeType = MIMETypeRegistry::getMIMETypeForPath(fileName);
                    if (!mimeType.isEmpty())
                        FormDataBuilder::addContentTypeToMultiPartHeader(header, mimeType.latin1());
                }
            }

            FormDataBuilder::finishMultiPartHeader(header);

            // Append body
            appendData(header.data(), header.size());
            appendItem(value, shouldGenerateFile);
            appendData("\r\n", 2);
        } else {
            // Omit the name "isindex" if it's the first form data element.
            // FIXME: Why is this a good rule? Is this obsolete now?
            const StringBlobItem* stringValue = value->toStringBlobItem();
            if (!stringValue)
                continue;
            if (encodedData.isEmpty() && key->cstr() == "isindex")
                FormDataBuilder::encodeStringAsFormData(encodedData, stringValue->cstr());
            else
                FormDataBuilder::addKeyValuePairAsFormData(encodedData, key->cstr(), stringValue->cstr());
        }
    }

    if (isMultiPartForm)
        FormDataBuilder::addBoundaryToMultiPartHeader(encodedData, m_boundary.data(), true);

    appendData(encodedData.data(), encodedData.size());
}

void FormData::flatten(Vector<char>& data) const
{
    // Concatenate all the byte arrays, but omit any files.
    data.clear();
    size_t n = m_elements.size();
    for (size_t i = 0; i < n; ++i) {
        const FormDataElement& e = m_elements[i];
        if (e.m_type == FormDataElement::data)
            data.append(e.m_data.data(), static_cast<size_t>(e.m_data.size()));
    }
}

String FormData::flattenToString() const
{
    Vector<char> bytes;
    flatten(bytes);
    return Latin1Encoding().decode(bytes.data(), bytes.size());
}

void FormData::generateFiles(Document* document)
{
    ASSERT(!m_hasGeneratedFiles);

    if (m_hasGeneratedFiles)
        return;

    Page* page = document->page();
    if (!page)
        return;
    ChromeClient* client = page->chrome()->client();

    size_t n = m_elements.size();
    for (size_t i = 0; i < n; ++i) {
        FormDataElement& e = m_elements[i];
        if (e.m_type == FormDataElement::encodedFile && e.m_shouldGenerateFile) {
            e.m_generatedFilename = client->generateReplacementFile(e.m_filename);
            m_hasGeneratedFiles = true;
        }
    }
}

void FormData::removeGeneratedFilesIfNeeded()
{
    if (!m_hasGeneratedFiles)
        return;

    size_t n = m_elements.size();
    for (size_t i = 0; i < n; ++i) {
        FormDataElement& e = m_elements[i];
        if (e.m_type == FormDataElement::encodedFile && !e.m_generatedFilename.isEmpty()) {
            ASSERT(e.m_shouldGenerateFile);
            String directory = directoryName(e.m_generatedFilename);
            deleteFile(e.m_generatedFilename);
            deleteEmptyDirectory(directory);
            e.m_generatedFilename = String();
        }
    }
    m_hasGeneratedFiles = false;
}

} // namespace WebCore
