/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
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
    paraRecord.optionFlag |= ocgdb::query_flag_print_all;

    gameCnt = 0; errCnt = 0; succCount = 0;

    std::vector<std::string> fenVec;
    for(auto && query : paraRecord.queries) {
        auto s = bslib::Funcs::FEN2EPD(query);
        if (!s.empty()) {
            fenVec.push_back(s);
        }
    }

    // Query databases
    for(auto && dbPath : paraRecord.bookPaths) {
        if (bslib::Funcs::endsWith(dbPath, ".db3")) {
            queryADb(dbPath, fenVec);
        }
        else if (bslib::Funcs::endsWith(dbPath, ".bin")) {
            queryAPolyglot(dbPath, fenVec);
        }
    }
}


void Search::queryADb(const std::string& dbPath, const std::vector<std::string>& epdVec)
{
    std::cout << "Querying a database: '" << dbPath << "'" << std::endl;

    if (!_openDB(dbPath)) {
        return;
    }

    for(auto && epdString : epdVec) {
        auto sqlString = "SELECT * FROM Book WHERE EPD='" + epdString + "'";
        
        SQLite::Statement stmt(*mDb, sqlString);
        
        for (gameCnt = 0; stmt.executeStep(); ++gameCnt) {
            std::string str;
            for(int i = 0, cnt = stmt.getColumnCount(); i < cnt; ++i) {
                auto c = stmt.getColumn(i);
                std::string name = c.getName();
                str += name + ": " + c.getString() + ", ";
            }
            if (paraRecord.optionFlag & ocgdb::query_flag_print_all) {
                std::cout << str << std::endl;
            }
        }
    }
}

void Search::queryAPolyglot(const std::string& dbPath, const std::vector<std::string>& epdVec)
{
    std::cout << "Querying a Polyglot: '" << dbPath << "'" << std::endl;

    bslib::BookPolyglot book;
    
    book.loadData(dbPath);
    if (book.isEmpty()) {
        return;
    }

    auto board = bslib::Funcs::createBoard(bslib::ChessVariant::standard);
    
    for(auto && epdString : epdVec) {
        std::string str = "EPD: " + epdString + "\n";

        board->newGame(epdString);
        
        auto vec = book.search(board->key());
        if (vec.empty()) continue;
        
        for(auto && item : vec) {
            auto move = item.getMove(board);
            str += "key: " + std::to_string(item.key)
                + ", move: " + bslib::Move::toChessCoordinateString(move.from, move.dest, move.promotion)
                + ", weight: " + std::to_string(item.weight)
                + ", learn: " + std::to_string(item.learn);
            
            if (paraRecord.optionFlag & ocgdb::query_flag_print_all) {
                std::cout << str << std::endl;
            }
        }
   }
}
