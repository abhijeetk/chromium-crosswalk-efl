// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_cache_listener.h"

#include "base/bind.h"
#include "base/debug/trace_event.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/service_worker/service_worker_cache_storage_manager.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "third_party/WebKit/public/platform/WebServiceWorkerCacheError.h"

namespace content {

using blink::WebServiceWorkerCacheError;

namespace {

WebServiceWorkerCacheError ToWebServiceWorkerCacheError(
    ServiceWorkerCacheStorage::CacheStorageError err) {
  switch (err) {
    case ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_NO_ERROR:
      NOTREACHED();
      return blink::WebServiceWorkerCacheErrorNotImplemented;
    case ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_NOT_IMPLEMENTED:
      return blink::WebServiceWorkerCacheErrorNotImplemented;
    case ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_NOT_FOUND:
      return blink::WebServiceWorkerCacheErrorNotFound;
    case ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_EXISTS:
      return blink::WebServiceWorkerCacheErrorExists;
    case ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_STORAGE:
      // TODO(jkarlin): Changethis to CACHE_STORAGE_ERROR_STORAGE once that's
      // added.
      return blink::WebServiceWorkerCacheErrorNotFound;
    case ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_CLOSING:
      // TODO(jkarlin): Update this to CACHE_STORAGE_ERROR_CLOSING once that's
      // added.
      return blink::WebServiceWorkerCacheErrorNotFound;
  }
  NOTREACHED();
  return blink::WebServiceWorkerCacheErrorNotImplemented;
}

}  // namespace

ServiceWorkerCacheListener::ServiceWorkerCacheListener(
    ServiceWorkerVersion* version,
    base::WeakPtr<ServiceWorkerContextCore> context)
    : version_(version),
      context_(context),
      next_cache_id_(0),
      weak_factory_(this) {
}

ServiceWorkerCacheListener::~ServiceWorkerCacheListener() {
}

bool ServiceWorkerCacheListener::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ServiceWorkerCacheListener, message)
    IPC_MESSAGE_HANDLER(ServiceWorkerHostMsg_CacheStorageGet,
                        OnCacheStorageGet)
    IPC_MESSAGE_HANDLER(ServiceWorkerHostMsg_CacheStorageHas,
                        OnCacheStorageHas)
    IPC_MESSAGE_HANDLER(ServiceWorkerHostMsg_CacheStorageCreate,
                        OnCacheStorageCreate)
    IPC_MESSAGE_HANDLER(ServiceWorkerHostMsg_CacheStorageDelete,
                        OnCacheStorageDelete)
    IPC_MESSAGE_HANDLER(ServiceWorkerHostMsg_CacheStorageKeys,
                        OnCacheStorageKeys)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void ServiceWorkerCacheListener::OnCacheStorageGet(
    int request_id,
    const base::string16& cache_name) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerCacheListener::OnCacheStorageGet");
  context_->cache_manager()->GetCache(
      version_->scope().GetOrigin(),
      base::UTF16ToUTF8(cache_name),
      base::Bind(&ServiceWorkerCacheListener::OnCacheStorageGetCallback,
                 weak_factory_.GetWeakPtr(),
                 request_id));
}

void ServiceWorkerCacheListener::OnCacheStorageHas(
    int request_id,
    const base::string16& cache_name) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerCacheListener::OnCacheStorageHas");
  context_->cache_manager()->HasCache(
      version_->scope().GetOrigin(),
      base::UTF16ToUTF8(cache_name),
      base::Bind(&ServiceWorkerCacheListener::OnCacheStorageHasCallback,
                 weak_factory_.GetWeakPtr(),
                 request_id));
}

void ServiceWorkerCacheListener::OnCacheStorageCreate(
    int request_id,
    const base::string16& cache_name) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerCacheListener::OnCacheStorageCreate");
  context_->cache_manager()->CreateCache(
      version_->scope().GetOrigin(),
      base::UTF16ToUTF8(cache_name),
      base::Bind(&ServiceWorkerCacheListener::OnCacheStorageCreateCallback,
                 weak_factory_.GetWeakPtr(),
                 request_id));
}

void ServiceWorkerCacheListener::OnCacheStorageDelete(
    int request_id,
    const base::string16& cache_name) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerCacheListener::OnCacheStorageDelete");
  context_->cache_manager()->DeleteCache(
      version_->scope().GetOrigin(),
      base::UTF16ToUTF8(cache_name),
      base::Bind(&ServiceWorkerCacheListener::OnCacheStorageDeleteCallback,
                 weak_factory_.GetWeakPtr(),
                 request_id));
}

void ServiceWorkerCacheListener::OnCacheStorageKeys(int request_id) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerCacheListener::OnCacheStorageKeys");
  context_->cache_manager()->EnumerateCaches(
      version_->scope().GetOrigin(),
      base::Bind(&ServiceWorkerCacheListener::OnCacheStorageKeysCallback,
                 weak_factory_.GetWeakPtr(),
                 request_id));
}

void ServiceWorkerCacheListener::Send(const IPC::Message& message) {
  version_->embedded_worker()->SendMessage(message);
}

void ServiceWorkerCacheListener::OnCacheStorageGetCallback(
    int request_id,
    const scoped_refptr<ServiceWorkerCache>& cache,
    ServiceWorkerCacheStorage::CacheStorageError error) {
  if (error != ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_NO_ERROR) {
    Send(ServiceWorkerMsg_CacheStorageGetError(
        request_id, ToWebServiceWorkerCacheError(error)));
    return;
  }

  CacheID cache_id = StoreCacheReference(cache);
  Send(ServiceWorkerMsg_CacheStorageGetSuccess(request_id, cache_id));
}

void ServiceWorkerCacheListener::OnCacheStorageHasCallback(
    int request_id,
    bool has_cache,
    ServiceWorkerCacheStorage::CacheStorageError error) {
  if (error != ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_NO_ERROR) {
    Send(ServiceWorkerMsg_CacheStorageHasError(
        request_id, ToWebServiceWorkerCacheError(error)));
    return;
  }
  if (!has_cache) {
    Send(ServiceWorkerMsg_CacheStorageHasError(
        request_id,
        blink::WebServiceWorkerCacheErrorNotFound));
    return;
  }
  Send(ServiceWorkerMsg_CacheStorageHasSuccess(request_id));
}

void ServiceWorkerCacheListener::OnCacheStorageCreateCallback(
    int request_id,
    const scoped_refptr<ServiceWorkerCache>& cache,
    ServiceWorkerCacheStorage::CacheStorageError error) {
  if (error != ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_NO_ERROR) {
    Send(ServiceWorkerMsg_CacheStorageCreateError(
        request_id, ToWebServiceWorkerCacheError(error)));
    return;
  }
  CacheID cache_id = StoreCacheReference(cache);
  Send(ServiceWorkerMsg_CacheStorageCreateSuccess(request_id, cache_id));
}

void ServiceWorkerCacheListener::OnCacheStorageDeleteCallback(
    int request_id,
    bool deleted,
    ServiceWorkerCacheStorage::CacheStorageError error) {
  if (!deleted ||
      error != ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_NO_ERROR) {
    Send(ServiceWorkerMsg_CacheStorageDeleteError(
        request_id, ToWebServiceWorkerCacheError(error)));
    return;
  }
  Send(ServiceWorkerMsg_CacheStorageDeleteSuccess(request_id));
}

void ServiceWorkerCacheListener::OnCacheStorageKeysCallback(
    int request_id,
    const std::vector<std::string>& strings,
    ServiceWorkerCacheStorage::CacheStorageError error) {
  if (error != ServiceWorkerCacheStorage::CACHE_STORAGE_ERROR_NO_ERROR) {
    Send(ServiceWorkerMsg_CacheStorageKeysError(
        request_id, ToWebServiceWorkerCacheError(error)));
    return;
  }

  std::vector<base::string16> string16s;
  for (size_t i = 0, max = strings.size(); i < max; ++i) {
    string16s.push_back(base::UTF8ToUTF16(strings[i]));
  }
  Send(ServiceWorkerMsg_CacheStorageKeysSuccess(request_id, string16s));
}

ServiceWorkerCacheListener::CacheID
ServiceWorkerCacheListener::StoreCacheReference(
    const scoped_refptr<ServiceWorkerCache>& cache) {
  CacheToIDMap::iterator it = cache_to_id_map_.find(cache.get());
  if (it == cache_to_id_map_.end()) {
    CacheID cache_id = next_cache_id_++;
    cache_to_id_map_.insert(std::make_pair(cache.get(), cache_id));
    id_to_cache_map_.insert(std::make_pair(cache_id, cache));
    return cache_id;
  }

  return it->second;
}

void ServiceWorkerCacheListener::DropCacheReference(CacheID cache_id) {
  IDToCacheMap::iterator it = id_to_cache_map_.find(cache_id);
  if (it != id_to_cache_map_.end())
    return;

  size_t deleted = cache_to_id_map_.erase(it->second.get());
  DCHECK(deleted == 1u);
  id_to_cache_map_.erase(it);
}

}  // namespace content
