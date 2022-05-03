/**
 * This file is part of Open Chess Game Database Standard.
 *
 * Copyright (c) 2021-2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021-2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <set>
#include <fstream>

#include "board/chess.h"
#include "board/funcs.h"
#include "dbread.h"

#include "records.h"

bool debugMode = false;


using namespace ocgdb;


////////////////////////////////////////////////////////////////////////
bool ParaRecord::isValid() const
{
    auto pgnCnt = 0;
    for(auto && s : pgnPaths) {
        if (!s.empty()) {
            pgnCnt++;
        }
    }

    auto dbCnt = 0;
    for(auto && s : dbPaths) {
        if (!s.empty()) {
            dbCnt++;
        }
    }

    auto bookCnt = 0;
    for(auto && s : bookPaths) {
        if (bslib::Funcs::endsWith(s, ".obs.db3") || bslib::Funcs::endsWith(s, ".bin")) {
            bookCnt++;
        } else {
            errorString = "Supported only Open Book Standard (.obs.db3), Polyglot (.bin). The book path must have those extensions.";
            return false;
        }
    }
    

    errorString.clear();
    auto ok = false;
    switch (task) {
        case Task::none:
        {
            errorString = "Must set a task. Mising or wrong parameter such as -create, -merge, -export, -q, -bench, -g, -dup";
            break;
        }
        case Task::create:
        {
            if (!pgnCnt && !dbCnt) {
                errorString = "Must have at least one PGN/database path. Mising or wrong parameter -db -pgn";
                break;
            }
            if (bookCnt != 1) {
                errorString = "Must have one opening book path. Mising or wrong parameter -book";
                break;
            }

            ok = true;
            break;
        }

        case Task::query:
            if (!bookCnt) {
                errorString = "Must have at least one book (.obs.db3/.bin)  path. Mising or wrong parameter -book";
                return false;
            }
            ok = true;
            break;
            

        default:
            break;
    }
    return ok;
}


static const std::map<std::string, int> optionNameMap = {
    // creating
    {"discardnoelo", 6},
    {"discardfen", 7},

    // query
    {"printall", 10},
    {"printfen", 11},
    {"printpgn", 12},

    {"nobot", 20},
    {"bot", 21},
};

std::string ParaRecord::toString(Task task)
{
    const std::string taskNames[] = {
        "create SQL database",
        "export",
        "merge",
        "query",
        "bench",
        "none"
    };
        
    return taskNames[static_cast<int>(task)];
}

std::string ParaRecord::toString() const
{
    std::string s;
    
    s = "\tTask: " + toString(task) + "\n";
    
    s += "\tBook paths:\n";
    for(auto && path : bookPaths) {
        s += "\t\t" + path + "\n";
    }

    s += "\tPGN paths:\n";
    for(auto && path : pgnPaths) {
        s += "\t\t" + path + "\n";
    }
    
    s += "\tDatabase paths:\n";
    for(auto && path : dbPaths) {
        s += "\t\t" + path + "\n";
    }

    s += "\tQueries:\n";
    for(auto && query : queries) {
        s += "\t\t" + query + "\n";
    }

    s += "\tOptions: ";
    
    for(auto && it : optionNameMap) {
        if (optionFlag & (1 << it.second)) {
            s += it.first + ",";
        }
    }
    
    s += "\n";

    s += "\n";
    s += "\tply_take: " + std::to_string(ply_take) + ", gamepernode: " + std::to_string(gamepernode) + "\n"
        + "\tcpu: " + std::to_string(cpuNumber)
        + ", min Elo: " + std::to_string(limitElo)
        + ", min game length: " + std::to_string(limitLen)
        + "\n";

    return s;
}


void ParaRecord::setupOptions(const std::string& string)
{
    auto vec = bslib::Funcs::splitString(string, ',');
    
    for(auto && s : vec) {
        auto it = optionNameMap.find(s);
        if (it == optionNameMap.end()) {
            std::cerr << "Error: Don't know option string: " << s << std::endl;
        } else {
            optionFlag |= 1 << it->second;
            
            if (s == "printpgn" || s == "printfen") {
                auto it2 = optionNameMap.find("printall");
                assert(it2 != optionNameMap.end());
                optionFlag |= 1 << it2->second;
            }
        }
    }
}

void ParaRecord::setWDLFactors(const std::string& string)
{
    auto vec = bslib::Funcs::splitString(string, ',');
    
    if (vec.size() < 3) return;
    winfactor = std::stoi(vec.at(0));
    drawfactor = std::stoi(vec.at(1));
    lossfactor = std::stoi(vec.at(2));
}

void ThreadRecord::init()
{
    if (board) return;
    
    errCnt = 0;
    
    board = bslib::Funcs::createBoard(bslib::ChessVariant::standard);
    board2 = bslib::Funcs::createBoard(bslib::ChessVariant::standard);
}


ThreadRecord::~ThreadRecord()
{
    if (board) delete board;
    if (board2) delete board2;    
}


void ThreadRecord::resetStats()
{
    errCnt = gameCnt = nodeCnt = 0;
}

void ThreadRecord::boardToNodes(std::map<uint64_t, oobs::BookNode>& nodeMap, const ParaRecord& paraRecord, const std::string& resultString)
{
    auto n = board->getHistListSize();
    if (n < paraRecord.limitLen) {
        return;
    }

    auto result = bslib::ResultType::noresult;

    if (resultString == "1-0") result = bslib::ResultType::win;
    else if (resultString == "0-1") result = bslib::ResultType::loss;
    else if (resultString == "1/2-1/2" || resultString == "0.5-0.5") result = bslib::ResultType::draw;


    board2->newGame(board->getStartingFen());
    for(auto i = 0, e = std::min(paraRecord.ply_take, n); i < e; i++) {
        auto hashKey = board2->hashKey;
        auto fenString = board2->getFen(0, 1);

        auto hist = board->getHistAt(i);
        
        assert(hist.move.isValid());

        auto moveString = hist.move.toCoordinateString(board2->variant);
        
        auto node = &nodeMap[hashKey];
        if (node->fen.empty()) {
            nodeCnt++;
            node->fen = fenString;
        } else {
            assert(node->fen == fenString);
        }
        
        auto moveInt = oobs::MoveWDL::move2Int(hist.move.from, hist.move.dest, hist.move.promotion, hist.castled);

        auto wdl = &node->moveMap[moveInt];

        // hits
        if (result == bslib::ResultType::draw) {
            wdl->draw++;
        } else if ((result == bslib::ResultType::win && board2->side == bslib::Side::white) || (result == bslib::ResultType::loss && board2->side == bslib::Side::black)) {
            wdl->win++;
        } else {
            wdl->loss++;
        }
        assert(wdl->isValid());

        board2->_make(hist.move);
    }
}


//////////////////////////////////




QueryGameRecord::QueryGameRecord(SQLite::Database& db, SearchField searchField)
: searchField(searchField)
{
    std::string str = DbRead::fullGameQueryString + " WHERE g.ID = ?";
    queryGameByID = new SQLite::Statement(db, str);
    queryComments = new SQLite::Statement(db, "SELECT * FROM Comments WHERE GameID = ?");
    
    board = bslib::Funcs::createBoard(bslib::ChessVariant::standard);
}


std::string QueryGameRecord::queryAndCreatePGNByGameID(bslib::PgnRecord& record)
{
    std::lock_guard<std::mutex> dolock(queryMutex);

    assert(queryGameByID);

    queryGameByID->reset();
    queryGameByID->bind(1, record.gameID);
    
    std::string str;
    
    if (queryGameByID->executeStep()) {
        DbRead::queryForABoard(record, searchField, queryGameByID, queryComments, board);
        str = board->toPgn(&record);
    }
    return str;
}
