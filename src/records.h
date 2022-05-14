/**
 * This file is part of Open Chess Game Database Standard.
 *
 * Copyright (c) 2021-2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021-2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OOBS_RECORDS_H
#define OOBS_RECORDS_H

#include <vector>
#include <unordered_map>
#include <fstream>

#include "3rdparty/SQLiteCpp/SQLiteCpp.h"
#include "3rdparty/threadpool/thread_pool.hpp"

#include "board/types.h"
#include "board/base.h"

#include "bookrecords.h"

namespace ocgdb {

// Developer may change those string
const std::string VersionUserDatabaseString = "0.1";

const std::string VersionString = "Beta 8";
const std::string VersionDatabaseString = "0.6";

// Current limit is about 4 billion, we can change later by changing this define
#define IDInteger uint32_t

enum class Task
{
    create,
    export_,
    merge,
    query,
    bench,
    none,
};

enum {
    TagIdx_ID,
    TagIdx_FEN,
    TagIdx_Move,
    TagIdx_Win, TagIdx_Draw, TagIdx_Loss,
    TagIdx_Max
};

extern const std::vector<std::string> knownPgnTagVec;

enum {
    create_flag_discard_no_elo          = 1 << 6,
    create_flag_discard_fen             = 1 << 7,

    query_flag_print_all                = 1 << 10,
    query_flag_print_fen                = 1 << 11,
    query_flag_print_pgn                = 1 << 12,

    lichess_flag_nobot                  = 1 << 20,
    lichess_flag_bot                    = 1 << 21,

    flag_noresult_win                   = 1 << 25,
    flag_noresult_draw                  = 1 << 26,
    flag_noresult_loss                  = 1 << 27,

    flag_pgn_unique_lastpos             = 1 << 29,
};

class ParaRecord
{
public:
    std::vector<std::string> inputPaths, outputPaths, queries;
    std::string reportPath, desc;

    int optionFlag = 0;

    Task task = Task::none;
    int cpuNumber = -1, limitElo = 0, limitLen = 0, ply_take = 20, ply_delta = 1, gamepernode = 1;
    
    int winfactor = 2, drawfactor = 1, lossfactor = 0;
    
    mutable std::string errorString;
    
    std::string getErrorString() const {
        return errorString;
    }

    static std::string toString(Task task);
    std::string toString() const;
    bool isValid() const;
    
    void setupOptions(const std::string&);
    void setWDLFactors(const std::string&);
};

class ThreadRecord
{
public:
    ~ThreadRecord();
    void init();

    void resetStats();
    void boardToNodes(std::map<uint64_t, oobs::BookNode>&, const ParaRecord&, const std::string&);

public:
    bslib::BoardCore *board = nullptr, *board2 = nullptr;

    int64_t errCnt = 0, gameCnt = 0, nodeCnt = 0;
};

};


enum class SearchField
{
    none,
    moves,          // text only
    moves1,         // 1.5 byte per move
    moves2,         // 2 bytes per move
};

class QueryGameRecord {
public:
    QueryGameRecord(SQLite::Database& db, SearchField searchField);

    ~QueryGameRecord() {
        if (queryGameByID) delete queryGameByID;
        if (queryComments) delete queryComments;
        if (board) delete board;
    }
    
    std::string queryAndCreatePGNByGameID(bslib::PgnRecord& record);
    
public:

    SQLite::Statement* queryGameByID = nullptr, *queryComments = nullptr;
    bslib::BoardCore* board = nullptr;
    SearchField searchField;
    
private:
    std::mutex queryMutex;
};

#endif /* OOBS_RECORDS_H */
