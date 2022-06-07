/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "merge.h"

#include "board/book.h"

using namespace oobs;

void Merge::runTask()
{
    std::cout   << "Merging..." << std::endl;

//    if (paraRecord.inputPaths.empty()) {
//        std::cout << "Error: there is no path for input files" << std::endl;
//        return;
//    }
//
//    if (paraRecord.queries.empty()) {
//        std::cout << "Error: there is no FEN string" << std::endl;
//        return;
//    }
//
//    assert(paraRecord.task == ocgdb::Task::query);
//
////    ocgdb::printOut.init(true, "");
//
//    gameCnt = 0; errCnt = 0; succCount = 0;
//
//    for(auto && _query : paraRecord.queries) {
//        query = cleanQuery(_query);
//
//        if (query.empty()) {
//            continue;;
//        }
//
//        if (!parser.parse(query.c_str())) {
//            std::cerr << "Error: " << parser.getErrorString() << std::endl;
//            continue;;
//        }
//
//        std::cout << "Search with query " << query <<  "..." << std::endl;
//
//        // Query databases
//        for(auto && path : paraRecord.inputPaths) {
//            if (bslib::Funcs::endsWith(path, ".db3")) {
//                queryADb(path);
//            }
//        }
//    }
}


//void doProcessARecord(Query* instance, const std::map<std::string, std::string>& theMap)
//{
//    assert(instance);
//    instance->processARecord(theMap);
//}


void Merge::queryADb(const std::string& dbPath)
{
//    std::cout << "Querying a database: " << dbPath << std::endl;
//    assert(pool);
//
//    startTime = getNow();
//
//    if (!_openDB(dbPath)) {
//        return;
//    }
//
//    for(auto && m : threadMap) {
//        m.second.resetStats();
//    }
//
//    auto sqlString = "SELECT * FROM Book";
//    SQLite::Statement stmt(*mDb, sqlString);
//
//    for (gameCnt = 0; stmt.executeStep(); ++gameCnt) {
//        std::map<std::string, std::string> theMap;
//        for(int i = 0, cnt = stmt.getColumnCount(); i < cnt; ++i) {
//            auto c = stmt.getColumn(i);
//            std::string name = c.getName();
//            theMap[name] = c.getString();
//        }
//        pool->submit(doProcessARecord, this, theMap);
//
//        if (pool->get_tasks_total() > 1024 * 8) {
//            pool->wait_for_tasks();
//        }
//
//        if (gameCnt && !(gameCnt & 0xfffff)) {
//            printStats();
//        }
//    }
//
//    pool->wait_for_tasks();
//    printStats();
}

void Merge::printStats() const
{
//    int64_t resultCnt = 0;
//    for(auto && m : threadMap) {
//        resultCnt += m.second.resultCnt;
//    }
//    int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(getNow() - startTime).count() + 1;
//
//    std::lock_guard<std::mutex> dolock(printMutex);
//
//    std::cout << "#EPD: " << gameCnt
//              << ", elapsed: " << elapsed << "ms "
//              << bslib::Funcs::secondToClockString(static_cast<int>(elapsed / 1000), ":")
//              << ", speed: " << gameCnt * 1000ULL / elapsed
//              << " EPD/s, #results: " << resultCnt
//              << std::endl;
//    ;
}


void Merge::processARecord(const std::map<std::string, std::string>& theMap)
{
//    auto t = getThreadRecord(); assert(t);
//    t->init();
//
//    auto it = theMap.find("EPD");
//    if (it == theMap.end()) {
//        return;
//    }
//
//    t->board->newGame(it->second);
//
//    auto bitboardVec = t->board->posToBitboards();
//
//    if (!parser.evaluate(bitboardVec)) {
//        return;
//    }
//
//    t->resultCnt++;
//
//    std::string str;
//    for(auto && it : theMap) {
//        str += it.first + ":" + it.second + ", ";
//    }
//
//    ocgdb::printOut.printOut(str);
}

