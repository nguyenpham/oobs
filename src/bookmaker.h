/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2021 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021 Developers
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
#include "dbread.h"
#include "pgnread.h"

namespace oobs {


class BookMaker : public ocgdb::DbRead, public ocgdb::PGNRead
{
public:
    virtual void runTask() override;

    void create();

private:
    static SQLite::Database* createBookDb(const std::string& path, const std::string& dbDescription);

    virtual void processPGNGameWithAThread(ocgdb::ThreadRecord*, const std::unordered_map<char*, char*>&, const char *) override;

    virtual void processAGameWithAThread(ocgdb::ThreadRecord*, const bslib::PgnRecord& record, const std::vector<int8_t>& moveVec) override;

    void printStats() const override;
    void updateInfoTable();

private:
    bool createBook();

    void toBook();
    void threadAddGame(const std::unordered_map<char*, char*>& itemMap, const char* moveText);

    void add(int& fenID, uint64_t key, const BookNode&);
    void add2Db(int& fenID, const std::string& fenString, const std::vector<MoveWDL>&);
    void add2Polyglot(uint64_t hashKey, std::vector<MoveWDL>&, bool isWhite);
    int scoreForPolyglot(const WinDrawLoss& a, bool isWhite) const;

private:
    const int Transaction2Comit = 512 * 1024;
    SQLite::Database* bookDb = nullptr;

    std::mutex nodeMapMutex;
    std::map<uint64_t, oobs::BookNode> nodeMap;

    // Prepared statements
    SQLite::Statement *insertStatement = nullptr, *insertFENStatement = nullptr;

    uint64_t itemCnt, discardCnt;

    bool polyglotMode = false;

    std::ofstream polyglotFile;
};


} // namespace oobs
#endif // OOBS_BOOKMAKER_H
