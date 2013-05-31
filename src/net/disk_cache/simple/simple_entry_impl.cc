// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_entry_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/message_loop_proxy.h"
#include "base/threading/worker_pool.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/simple/simple_index.h"
#include "net/disk_cache/simple/simple_synchronous_entry.h"

namespace {

typedef disk_cache::Entry::CompletionCallback CompletionCallback;
typedef disk_cache::SimpleSynchronousEntry::SynchronousCreationCallback
    SynchronousCreationCallback;
typedef disk_cache::SimpleSynchronousEntry::SynchronousOperationCallback
    SynchronousOperationCallback;

}  // namespace

namespace disk_cache {

using base::FilePath;
using base::MessageLoopProxy;
using base::Time;
using base::WeakPtr;
using base::WorkerPool;

// static
int SimpleEntryImpl::OpenEntry(WeakPtr<SimpleIndex> index,
                               const FilePath& path,
                               const std::string& key,
                               Entry** entry,
                               const CompletionCallback& callback) {
  // TODO(gavinp): More closely unify the last_used_ in the
  // SimpleSynchronousEntry  and the SimpleIndex.
  if (!index || index->UseIfExists(key)) {
    SynchronousCreationCallback sync_creation_callback =
        base::Bind(&SimpleEntryImpl::CreationOperationComplete,
                   index, callback, key, entry);
    WorkerPool::PostTask(FROM_HERE,
                         base::Bind(&SimpleSynchronousEntry::OpenEntry, path,
                                    key, MessageLoopProxy::current(),
                                    sync_creation_callback),
                         true);
    return net::ERR_IO_PENDING;
  }
  return net::ERR_FAILED;
}

// static
int SimpleEntryImpl::CreateEntry(WeakPtr<SimpleIndex> index,
                                 const FilePath& path,
                                 const std::string& key,
                                 Entry** entry,
                                 const CompletionCallback& callback) {
  SynchronousCreationCallback sync_creation_callback =
      base::Bind(&SimpleEntryImpl::CreationOperationComplete,
                 index, callback, key, entry);
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::CreateEntry, path,
                                  key, MessageLoopProxy::current(),
                                  sync_creation_callback),
                       true);
  return net::ERR_IO_PENDING;
}

// static
int SimpleEntryImpl::DoomEntry(WeakPtr<SimpleIndex> index,
                               const FilePath& path,
                               const std::string& key,
                               const CompletionCallback& callback) {
  if (index)
    index->Remove(key);
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::DoomEntry, path, key,
                                  MessageLoopProxy::current(), callback),
                       true);
  return net::ERR_IO_PENDING;
}

void SimpleEntryImpl::Doom() {
  DCHECK(io_thread_checker_.CalledOnValidThread());
#if defined(OS_POSIX)
  // This call to static SimpleEntryImpl::DoomEntry() will just erase the
  // underlying files. On POSIX, this is fine; the files are still open on the
  // SimpleSynchronousEntry, and operations can even happen on them. The files
  // will be removed from the filesystem when they are closed.
  DoomEntry(index_, path_, key_, CompletionCallback());
#else
  NOTIMPLEMENTED();
#endif
}

void SimpleEntryImpl::Close() {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  if (!synchronous_entry_in_use_by_worker_) {
    WorkerPool::PostTask(FROM_HERE,
                         base::Bind(&SimpleSynchronousEntry::Close,
                                    base::Unretained(synchronous_entry_)),
                         true);
  }
  // Entry::Close() is expected to release this entry. See disk_cache.h for
  // details.
  delete this;
}

std::string SimpleEntryImpl::GetKey() const {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  return key_;
}

Time SimpleEntryImpl::GetLastUsed() const {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  return last_used_;
}

Time SimpleEntryImpl::GetLastModified() const {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  return last_modified_;
}

int32 SimpleEntryImpl::GetDataSize(int index) const {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  return data_size_[index];
}

int SimpleEntryImpl::ReadData(int index,
                              int offset,
                              net::IOBuffer* buf,
                              int buf_len,
                              const CompletionCallback& callback) {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  // TODO(gavinp): Add support for overlapping reads. The net::HttpCache does
  // make overlapping read requests when multiple transactions access the same
  // entry as read only. This might make calling SimpleSynchronousEntry::Close()
  // correctly more tricky (see SimpleEntryImpl::EntryOperationComplete).
  if (synchronous_entry_in_use_by_worker_) {
    NOTIMPLEMENTED();
    CHECK(false);
  }
  synchronous_entry_in_use_by_worker_ = true;
  index_->UseIfExists(key_);
  SynchronousOperationCallback sync_operation_callback =
      base::Bind(&SimpleEntryImpl::EntryOperationComplete,
                 index_, callback, weak_ptr_factory_.GetWeakPtr(),
                 synchronous_entry_);
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::ReadData,
                                  base::Unretained(synchronous_entry_),
                                  index, offset, make_scoped_refptr(buf),
                                  buf_len, sync_operation_callback),
                       true);
  return net::ERR_IO_PENDING;
}

int SimpleEntryImpl::WriteData(int index,
                               int offset,
                               net::IOBuffer* buf,
                               int buf_len,
                               const CompletionCallback& callback,
                               bool truncate) {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  if (synchronous_entry_in_use_by_worker_) {
    NOTIMPLEMENTED();
    CHECK(false);
  }
  synchronous_entry_in_use_by_worker_ = true;
  index_->UseIfExists(key_);
  SynchronousOperationCallback sync_operation_callback =
      base::Bind(&SimpleEntryImpl::EntryOperationComplete,
                 index_, callback, weak_ptr_factory_.GetWeakPtr(),
                 synchronous_entry_);
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::WriteData,
                                  base::Unretained(synchronous_entry_),
                                  index, offset, make_scoped_refptr(buf),
                                  buf_len, sync_operation_callback, truncate),
                       true);
  return net::ERR_IO_PENDING;
}

int SimpleEntryImpl::ReadSparseData(int64 offset,
                                    net::IOBuffer* buf,
                                    int buf_len,
                                    const CompletionCallback& callback) {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
  return net::ERR_FAILED;
}

int SimpleEntryImpl::WriteSparseData(int64 offset,
                                     net::IOBuffer* buf,
                                     int buf_len,
                                     const CompletionCallback& callback) {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
  return net::ERR_FAILED;
}

int SimpleEntryImpl::GetAvailableRange(int64 offset,
                                       int len,
                                       int64* start,
                                       const CompletionCallback& callback) {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
  return net::ERR_FAILED;
}

bool SimpleEntryImpl::CouldBeSparse() const {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  return false;
}

void SimpleEntryImpl::CancelSparseIO() {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
}

int SimpleEntryImpl::ReadyForSparseIO(const CompletionCallback& callback) {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
  return net::ERR_FAILED;
}

SimpleEntryImpl::SimpleEntryImpl(
    SimpleSynchronousEntry* synchronous_entry,
    WeakPtr<SimpleIndex> index)
    : ALLOW_THIS_IN_INITIALIZER_LIST(weak_ptr_factory_(this)),
      path_(synchronous_entry->path()),
      key_(synchronous_entry->key()),
      synchronous_entry_(synchronous_entry),
      synchronous_entry_in_use_by_worker_(false),
      index_(index) {
  DCHECK(synchronous_entry);
  SetSynchronousData();
}

SimpleEntryImpl::~SimpleEntryImpl() {
  DCHECK(io_thread_checker_.CalledOnValidThread());
}

// static
void SimpleEntryImpl::CreationOperationComplete(
    WeakPtr<SimpleIndex> index,
    const CompletionCallback& completion_callback,
    const std::string& key,
    Entry** out_entry,
    SimpleSynchronousEntry* sync_entry) {
  if (!sync_entry) {
    completion_callback.Run(net::ERR_FAILED);
    // If OpenEntry failed, we must remove it from our index.
    if (index)
      index->Remove(key);
    return;
  }
  if (index)
    index->Insert(sync_entry->key());
  *out_entry = new SimpleEntryImpl(sync_entry, index);
  completion_callback.Run(net::OK);
}

// static
void SimpleEntryImpl::EntryOperationComplete(
    base::WeakPtr<SimpleIndex> index,
    const CompletionCallback& completion_callback,
    base::WeakPtr<SimpleEntryImpl> entry,
    SimpleSynchronousEntry* sync_entry,
    int result) {
  DCHECK(sync_entry);
  if (index)
    index->UpdateEntrySize(sync_entry->key(), sync_entry->GetFileSize());

  if (entry) {
    DCHECK(entry->synchronous_entry_in_use_by_worker_);
    entry->synchronous_entry_in_use_by_worker_ = false;
    entry->SetSynchronousData();
  } else {
    // |entry| must have had Close() called while this operation was in flight.
    // Since the simple cache now only supports one pending entry operation in
    // flight at a time, it's safe to now call Close() on |sync_entry|.
    WorkerPool::PostTask(FROM_HERE,
                         base::Bind(&SimpleSynchronousEntry::Close,
                                    base::Unretained(sync_entry)),
                         true);
  }
  completion_callback.Run(result);
}

void SimpleEntryImpl::SetSynchronousData() {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  DCHECK(!synchronous_entry_in_use_by_worker_);
  // TODO(felipeg): These copies to avoid data races are not optimal. While
  // adding an IO thread index (for fast misses etc...), we can store this data
  // in that structure. This also solves problems with last_used() on ext4
  // filesystems not being accurate.
  last_used_ = synchronous_entry_->last_used();
  last_modified_ = synchronous_entry_->last_modified();
  for (int i = 0; i < kSimpleEntryFileCount; ++i)
    data_size_[i] = synchronous_entry_->data_size(i);
}

}  // namespace disk_cache
