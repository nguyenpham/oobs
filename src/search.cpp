/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2021 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "search.h"

#include "board/book.h"

using namespace oobs;

void Search::runTask()
{
    std::cout   << "Finding a FEN..." << std::endl;

    if (paraRecord.bookPaths.empty()) {
        std::cout << "Error: there is no path for book files" << std::endl;
        return;
    }
    
    if (paraRecord.queries.empty()) {
        std::cout << "Error: there is no FEN string" << std::endl;
        return;
    }
    
    assert(paraRecord.task == ocgdb::Task::query);

    gameCnt = 0; errCnt = 0; succCount = 0;

    std::vector<std::string> fenVec;
    for(auto && query : paraRecord.queries) {
        auto s = bslib::Funcs::standardFENString(query);
        if (!s.empty()) {
            fenVec.push_back(s);
        }
    }

    // Query databases
    if (!paraRecord.bookPaths.empty()) {
        for(auto && dbPath : paraRecord.bookPaths) {
            if (bslib::Funcs::endsWith(dbPath, ".db3")) {
                queryADb(dbPath, fenVec);
            }
            else if (bslib::Funcs::endsWith(dbPath, ".bin")) {
                queryAPolyglot(dbPath, fenVec);
            }
        }
    }
}


void Search::queryADb(const std::string& dbPath, const std::vector<std::string>& fenVec)
{
    std::cout << "Processing a database: '" << dbPath << "'" << std::endl;

    if (!_openDB(dbPath)) {
        return;
    }

    for(auto && fenString : fenVec) {
        auto sqlString = "SELECT * FROM Book WHERE FEN='" + fenString + "'";
        
        SQLite::Statement stmt(*mDb, sqlString);
        
        for (gameCnt = 0; stmt.executeStep(); ++gameCnt) {
            for(int i = 0, cnt = stmt.getColumnCount(); i < cnt; ++i) {
                auto c = stmt.getColumn(i);
                std::string name = c.getName();
                std::cout << name << ": " << c.getString() << ", ";
            }
            
            std::cout << std::endl;
        }
    }
}

void Search::queryAPolyglot(const std::string& dbPath, const std::vector<std::string>& fenVec)
{
    std::cout << "Processing a Polyglot: '" << dbPath << "'" << std::endl;

    bslib::BookPolyglot book;
    
    book.loadData(dbPath);
    if (book.isEmpty()) {
        return;
    }

    auto board = bslib::Funcs::createBoard(bslib::ChessVariant::standard);
    
    for(auto && fenString : fenVec) {
        std::cout << "FEN: " << fenString << std::endl;

        board->newGame(fenString);
        
        auto vec = book.search(board->key());
        if (vec.empty()) continue;
        
        for(auto && item : vec) {
            auto move = item.getMove(board);
            std::cout << "key: " << item.key
            << ", move: " << bslib::Move::toChessCoordinateString(move.from, move.dest, move.promotion)
            << ", weight: " << item.weight
            << ", learn: " << item.learn
            << std::endl;

        }
   }
}
