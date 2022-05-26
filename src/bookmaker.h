/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OOBS_BOOKMAKER_H
#define OOBS_BOOKMAKER_H

#include <vector>
#include <unordered_map>

#include "board/types.h"
#include "board/base.h"

#include "bookrecords.h"
#include "ocgdb/dbread.h"
#include "ocgdb/pgnread.h"

namespace oobs {

enum class CreateBookType {
    obs, polyglot, pgn, epd, none
};


class BookMaker : public ocgdb::DbRead, public ocgdb::PGNRead
{
public:
    virtual void runTask() override;

    void createBook();
    
private:
    static SQLite::Database* createBookDb(const std::string& path, const std::string& dbDescription);

    virtual void processPGNGameWithAThread(ocgdb::ThreadRecord*, const std::unordered_map<char*, char*>&, const char *) override;

    virtual void processAGameWithAThread(ocgdb::ThreadRecord*, const bslib::PgnRecord& record, const std::vector<int8_t>& moveVec) override;

    void printStats() const override;
    void updateInfoTable();

private:
    bool createBookFile();

    void toBook();
    void toBookPgnEpd(bslib::BoardCore* board, std::set<uint64_t>& vistedSet);
    void savePgnEpd(bslib::BoardCore* board);

    void threadAddGame(const std::unordered_map<char*, char*>& itemMap, const char* moveText);

    void add(int& fenID, uint64_t key, const BookNode&);
    void add2Db(int& fenID, const std::string& fenString, const std::vector<MoveWDL>&);
    void add2Polyglot(uint64_t hashKey, std::vector<MoveWDL>&, bool isWhite);
    int scoreForPolyglot(const WinDrawLoss& a, bool isWhite) const;
    void setupRandonSavingPly();

private:
    const int Transaction2Comit = 512 * 1024;
    SQLite::Database* bookDb = nullptr;

    std::mutex nodeMapMutex;
    std::map<uint64_t, oobs::BookNode> nodeMap;
    std::set<std::string> lastEpdSet;
    int randomSavingPly;

    // Prepared statements
    SQLite::Statement *insertStatement = nullptr, *insertEPDStatement = nullptr;

    uint64_t itemCnt, discardCnt;
    double polyglotScaleFactor = 1.0;

    CreateBookType bookType = CreateBookType::obs;

    std::ofstream textBookFile;
};


} // namespace oobs
#endif // OOBS_BOOKMAKER_H
