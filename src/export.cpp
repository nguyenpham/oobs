/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "export.h"

#include "board/book.h"

using namespace oobs;


void Export::runTask()
{
    std::cout   << "Exporting..." << std::endl;

    if (paraRecord.inputPaths.empty() || paraRecord.outputPaths.size() != 1) {
        std::cout << "Error: there is no path for input files or empty output" << std::endl;
        return;
    }
    
    
    assert(paraRecord.task == ocgdb::Task::export_);

    gameCnt = 0; errCnt = 0; succCount = 0;
    
    std::cout << "Building the opening tree from input files..." << std::endl;

    for(auto && path : paraRecord.inputPaths) {
        if (bslib::Funcs::endsWith(path, ".db3")) {
            processOBS(path);
        }
    }
    
    if (nodeMap.empty()) {
        std::cerr << "The tree is empty. Stop!" << std::endl;
        return;
    }

    if (createBookFile()) {
        toBook();
        printStats();
    }

    // clean up
    if (bookType == bslib::BookType::polyglot) {
        textBookFile.close();
    }
}


void doProcessARecord(Export* instance, const std::map<std::string, std::string>& theMap)
{
    assert(instance);
    instance->processARecord(theMap);
}


void Export::processOBS(const std::string& dbPath)
{
    std::cout << "Processing a OBS book: " << dbPath << std::endl;

    startTime = getNow();

    if (!_openDB(dbPath)) {
        return;
    }

    for(auto && m : threadMap) {
        m.second.resetStats();
    }

    auto sqlString = "SELECT * FROM Book";
    SQLite::Statement stmt(*mDb, sqlString);
    
    for (gameCnt = 0; stmt.executeStep(); ++gameCnt) {
        std::map<std::string, std::string> theMap;
        for(int i = 0, cnt = stmt.getColumnCount(); i < cnt; ++i) {
            auto c = stmt.getColumn(i);
            std::string name = c.getName();
            theMap[name] = c.getString();
            pool->submit(doProcessARecord, this, theMap);
        }

        if (pool->get_tasks_total() > 1024 * 8) {
            pool->wait_for_tasks();
        }
        
        if (gameCnt && !(gameCnt & 0xfffff)) {
            printStats();
        }
    }

    pool->wait_for_tasks();
    printStats();
}

//void Export::printStats() const
//{
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
//}


void Export::processARecord(const std::map<std::string, std::string>& theMap)
{
    auto t = getThreadRecord(); assert(t);
    t->init();
    
    auto it = theMap.find("EPD");
    if (it == theMap.end()) {
        return;
    }
    
    auto epdString = it->second;

    
    it = theMap.find("Move");

    if (it == theMap.end()) {
        return;
    }
    auto moveString = it->second;

    int win = 0, draw = 0, loss = 0;

    it = theMap.find("Win");
    if (it != theMap.end()) {
        win = std::stoi(it->second);
    }
    it = theMap.find("Draw");
    if (it != theMap.end()) {
        draw = std::stoi(it->second);
    }
    it = theMap.find("Loss");
    if (it != theMap.end()) {
        loss = std::stoi(it->second);
    }
    
    if (win + draw + loss == 0 || moveString.empty() || moveString.empty()) {
        return;
    }

    t->board->newGame(epdString);
    auto hashKey = t->board->key();
    
    auto move = t->board->moveFromString_coordinate(moveString);
    
    if (!t->board->_quickCheckMake(move.from, move.dest, move.promotion, false)) {
        return;
    }

    auto hist = t->board->getHistAt(0);
    auto moveInt = oobs::MoveWDL::move2Int(hist.move.from, hist.move.dest, hist.move.promotion, hist.castled);


    {
        std::lock_guard<std::mutex> dolock(nodeMapMutex);
        
        auto node = &nodeMap[hashKey];
        if (node->epd.empty()) {
            t->nodeCnt++;
            node->epd = epdString;
        }
        auto wdl = &node->moveMap[moveInt];
        wdl->win += win;
        wdl->draw += draw;
        wdl->loss += loss;
    }
}


