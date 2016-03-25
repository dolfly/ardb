/*
 * rocksdb.hpp
 *
 *  Created on: 2015��8��21��
 *      Author: wangqiying
 */

#ifndef ROCKSDB_HPP_
#define ROCKSDB_HPP_

#include "common/common.hpp"
#include "thread/spin_rwlock.hpp"
#include "thread/thread_local.hpp"
#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/comparator.h"
#include "rocksdb/cache.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/statistics.h"
#include "rocksdb/merge_operator.h"
#include "engine.hpp"
#include <vector>
#include <sparsehash/dense_hash_map>

OP_NAMESPACE_BEGIN

    class RocksDBEngine;
    class RocksDBIterator:public Iterator
    {
        private:
            KeyObject m_key;
            ValueObject m_value;
            RocksDBEngine* m_engine;
            rocksdb::Iterator* m_iter;
            bool Valid();
            void Next();
            void Prev();
            void Jump(const KeyObject& next);
            KeyObject& Key();
            ValueObject& Value();
        public:
            RocksDBIterator(RocksDBEngine* engine, rocksdb::Iterator* iter) :
                m_engine(engine),m_iter(iter)
            {
            }
            ~RocksDBIterator();
    };

    class RocksDBEngine: public Engine
    {
        private:
            class RocksTransaction
            {
                private:
                    rocksdb::WriteBatch batch;
                    uint32_t ref;
                public:
                    RocksTransaction() :
                            ref(0)
                    {
                    }
                    rocksdb::WriteBatch& GetBatch()
                    {
                        return batch;
                    }
                    rocksdb::WriteBatch* Ref()
                    {
                        if (ref > 0)
                        {
                            return &batch;
                        }
                        return NULL;
                    }
                    uint32 AddRef()
                    {
                        ref++;
                        batch.SetSavePoint();
                        return ref;
                    }
                    uint32 ReleaseRef(bool rollback)
                    {
                        ref--;
                        if (rollback)
                        {
                            batch.RollbackToSavePoint();
                        }
                        return ref;
                    }
                    void Clear()
                    {
                        batch.Clear();
                        ref = 0;
                    }
            };
            struct RocksSnapshot
            {
                    const rocksdb::Snapshot* snapshot;
                    uint32_t ref;
                    RocksSnapshot() :
                            snapshot(NULL), ref(0)
                    {
                    }
            };

            typedef TreeMap<Data, rocksdb::ColumnFamilyHandle*>::Type ColumnFamilyHandleTable;
            rocksdb::DB* m_db;
            rocksdb::Options m_options;
            ColumnFamilyHandleTable m_handlers;
            SpinRWLock m_lock;
            ThreadLocal<RocksTransaction> m_transc;
            ThreadLocal<RocksSnapshot> m_snapshot;

            rocksdb::ColumnFamilyHandle* GetColumnFamilyHandle(Context& ctx, const Data& name);
            const rocksdb::Snapshot* GetSnpashot();
            const rocksdb::Snapshot* PeekSnpashot();
            void ReleaseSnpashot();
            friend class RocksDBIterator;
        public:
            RocksDBEngine();
            ~RocksDBEngine();
            int Init(const std::string& dir, const std::string& conf);
            int Put(Context& ctx, const KeyObject& key, const ValueObject& value);
            int Get(Context& ctx, const KeyObject& key, ValueObject& value);
            int MultiGet(Context& ctx, const KeyObjectArray& keys, ValueObjectArray& values, ErrCodeArray& errs);
            int Del(Context& ctx, const KeyObject& key);
            int Merge(Context& ctx, const KeyObject& key, uint16_t op, const DataArray& args);bool Exists(Context& ctx, const KeyObject& key);
            int BeginTransaction();
            int CommitTransaction();
            int DiscardTransaction();
            int Compact(Context& ctx, const KeyObject& start, const KeyObject& end);

            Iterator* Find(Context& ctx, const KeyObject& key);
    };

OP_NAMESPACE_END
#endif /* SRC_ROCKSDB_HPP_ */