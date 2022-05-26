/**
 * This file is part of Open Chess Game Database Standard.
 *
 * Copyright (c) 2021-2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021-2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OCGDB_CORE_H
#define OCGDB_CORE_H

#include <stdio.h>
#include <unordered_map>
#include <atomic>

#include "../3rdparty/threadpool/thread_pool.hpp"

#include "records.h"
#include "report.h"

namespace ocgdb {


class Core
{
public:
    Core();
    virtual ~Core();

    virtual void run(const ocgdb::ParaRecord&);
    ThreadRecord* getThreadRecord();
    static std::chrono::steady_clock::time_point getNow();
    static int64_t getElapse(std::chrono::steady_clock::time_point start);

protected:
    virtual void runTask() = 0;

    void createPool();
    virtual void printStats() const;

protected:
    bslib::ChessVariant chessVariant = bslib::ChessVariant::standard;

    ParaRecord paraRecord;
    
    mutable std::mutex threadMapMutex;
    mutable std::mutex printMutex;
    
    static thread_pool* pool;
    std::unordered_map<std::thread::id, ThreadRecord> threadMap;
    
    std::atomic<IDInteger> gameCnt;

    /// For stats
    std::chrono::steady_clock::time_point startTime;
    int64_t blockCnt, processedPgnSz, errCnt, succCount;
};

} // namespace ocdb

#endif /* OCGDB_CORE_H */
