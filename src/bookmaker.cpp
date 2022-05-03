/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2021 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021 Developers
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

#include "3rdparty/SQLiteCpp/VariadicBind.h"
#include "3rdparty/sqlite3/sqlite3.h"

#include "board/chess.h"
#include "board/book.h"

#include "bookmaker.h"

extern const bool two_tableMode;

// the standard uses only one table, thus this value must be set to false.
// use two tables for studying purposes only
const bool two_tableMode = false;

using namespace oobs;


void BookMaker::runTask()
{
    assert(sizeof(bslib::BookPolyglotItem) == 16);
    assert(sizeof(WinDrawLoss) == 12);
    assert(sizeof(MoveWDL) == 16);

    std::cout << "Convert PGN/database files into a database..." << std::endl;
    std::cout << "Mode: " << (two_tableMode ? "two tables" : "one table") << std::endl;

    startTime = getNow();

    // Prepare
    assert(!paraRecord.bookPaths.empty());
    auto bookPath = paraRecord.bookPaths.front();
    
    // remove old db file if existed
    std::remove(bookPath.c_str());
    
    create();
}

void BookMaker::create()
{
    std::cout << "Building opening trees from PGN files/databases..." << std::endl;
    // init
    {
        gameCnt = itemCnt = discardCnt = 0;
    }

    if (createBook()) {
        processPgnFiles(paraRecord.pgnPaths);

        for(auto && dbPath : paraRecord.dbPaths) {
            readADb(dbPath, "SELECT * FROM Games");
        }

        toBook();
        printStats();
    }

    // clean up
    if (polyglotMode) {
        polyglotFile.close();
    }
}

bool BookMaker::createBook()
{
    auto bookPath = paraRecord.bookPaths.front();
    
    if (bslib::Funcs::endsWith(bookPath, ".db3")) {
        polyglotMode = false;
        bookDb = createBookDb(bookPath, paraRecord.desc);
        
        // prepared statements
        if (bookDb) {
            if (two_tableMode) {
                const std::string fenSql = "INSERT INTO FEN (ID, FEN) VALUES (:ID, :FEN)";
                insertFENStatement = new SQLite::Statement(*bookDb, fenSql);

                const std::string sql = "INSERT INTO Book (FENID, Move, Win, Draw, Loss) "
                "VALUES (:FENID, :Move, :Win, :Draw, :Loss)";

                insertStatement = new SQLite::Statement(*bookDb, sql);
            } else {
                const std::string sql = "INSERT INTO Book (FEN, Move, Win, Draw, Loss) "
                "VALUES (:FEN, :Move, :Win, :Draw, :Loss)";
                insertStatement = new SQLite::Statement(*bookDb, sql);
            }

            return true;
        }
        return false;
    }
    
    // Polyglot
    polyglotMode = true;
    assert(bslib::Funcs::endsWith(bookPath, ".bin")); // Polyglot
    polyglotFile.open(bookPath, std::ofstream::binary| std::ofstream::trunc);
    auto r = polyglotFile.is_open();
    
    if (r) {
        std::cout << "Opened successfully to write Polyglot book " << bookPath << std::endl;
    } else {
        std::cerr << "Error: can't open to write Polyglot book " << bookPath << std::endl;

    }
    
    return r;
}

void BookMaker::updateInfoTable()
{
    assert(bookDb);
    try
    {
        std::string str = "DELETE FROM Info WHERE Name='Variant' OR Name='ItemCount'";
        bookDb->exec(str);

        str = std::string("INSERT INTO Info (Name, Value) VALUES ('Variant', '") + bslib::Funcs::chessVariant2String(chessVariant) + "')";
        bookDb->exec(str);

        str = std::string("INSERT INTO Info (Name, Value) VALUES ('ItemCount', '") + std::to_string(itemCnt) + "')";
        bookDb->exec(str);
    } catch (std::exception& e) {
        std::cout << "SQLite exception: " << e.what() << std::endl;
    }
}

void BookMaker::printStats() const
{
    int64_t nodeCnt = 0;
    for(auto && m : threadMap) {
        nodeCnt += m.second.nodeCnt;
    }
    DbCore::printStats();
    std::cout << ", nodeCnt: " << nodeCnt << std::endl;
}

SQLite::Database* BookMaker::createBookDb(const std::string& path, const std::string& dbDescription)
{
    assert(!path.empty());

    try
    {
        // Open a database file in create/write mode
        auto db = new SQLite::Database(path, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
        std::cout << "SQLite database file '" << db->getFilename() << "' opened successfully\n";

        db->exec("DROP TABLE IF EXISTS Info");
        db->exec("CREATE TABLE Info (Name TEXT UNIQUE NOT NULL, Value TEXT)");
        db->exec("INSERT INTO Info (Name, Value) VALUES ('Data Structure Version', '" + ocgdb::VersionDatabaseString + "')");

        // User Data version
        db->exec("INSERT INTO Info (Name, Value) VALUES ('Version', '" + ocgdb::VersionUserDatabaseString + "')");
        db->exec("INSERT INTO Info (Name, Value) VALUES ('License', 'free')");
        db->exec("INSERT INTO Info (Name, Value) VALUES ('Description', '" + dbDescription + "')");

        db->exec("DROP TABLE IF EXISTS Book");

        if (two_tableMode) {
            db->exec("DROP TABLE IF EXISTS FEN");
            db->exec("CREATE TABLE FEN (ID INTEGER PRIMARY KEY AUTOINCREMENT, FEN TEXT NOT NULL)");

            db->exec("CREATE TABLE Book (ID INTEGER PRIMARY KEY AUTOINCREMENT, FENID INTEGER NOT NULL, Move TEXT, Active INTEGER DEFAULT 1, Win INTEGER, Draw INTEGER, Loss INTEGER, FOREIGN KEY(FENID) REFERENCES FEN)");
        } else {
            db->exec("CREATE TABLE Book (ID INTEGER PRIMARY KEY AUTOINCREMENT, FEN TEXT NOT NULL, Move TEXT, Active INTEGER DEFAULT 1, Win INTEGER, Draw INTEGER, Loss INTEGER, Comment TEXT)");
        }

        db->exec("PRAGMA journal_mode=OFF");
        return db;
    } catch (std::exception& e) {
        std::cout << "SQLite exception: " << e.what() << std::endl;
    }

    return nullptr;
}

// Process a game from a PGN
void BookMaker::processPGNGameWithAThread(ocgdb::ThreadRecord* t, const std::unordered_map<char*, char*>& itemMap, const char* moveText)
{
    assert(t);
    t->init();
    assert(t->board);
    t->gameCnt++;
    gameCnt++;

    if (itemMap.empty()) {
        t->errCnt++;
        return;
    }
    
    std::unordered_map<std::string, const char*> stringMap;
    std::unordered_map<std::string, int> intMap;

    auto eloCnt = 0;
    std::string startFenString, resultString;

    for(auto && it : itemMap) {
        if (strcmp(it.first, "Variant") == 0) {
            auto variant = bslib::Funcs::string2ChessVariant(it.second);
            // at this moment, support only the standard variant
            assert(variant != bslib::ChessVariant::standard);

            if (variant != bslib::ChessVariant::standard) {
                t->errCnt++;
                return;
            }
            continue;
        }
        if (strcmp(it.first, "SetUp") == 0) {
            continue;
        }

        if (strcmp(it.first, "FEN") == 0) {
            if (paraRecord.optionFlag & ocgdb::create_flag_discard_fen) {
                return;
            }
            startFenString = it.second;
            continue;
        }

        if (strcmp(it.first, "Result") == 0) {
            resultString = it.second;
            continue;
        }

        if (strcmp(it.first, "PlyCount") == 0) {
            auto plyCount = std::atoi(it.second);
            if (plyCount < paraRecord.limitLen) {
                discardCnt++;
                return;
            }
        }
        if (strcmp(it.first, "WhiteElo") == 0 || strcmp(it.first, "BlackElo") == 0) {
            auto elo = std::atoi(it.second);
            if (elo < paraRecord.limitElo) {
                discardCnt++;
                return;
            }
            eloCnt++;
        }
    }

    if ((paraRecord.optionFlag & ocgdb::create_flag_discard_no_elo) && eloCnt != 2) {
        return;
    }

    assert(t);
    t->init();

    t->board->newGame(startFenString);
    
    int flag = bslib::BoardCore::ParseMoveListFlag_quick_check | bslib::BoardCore::ParseMoveListFlag_discardComment;

    bslib::PgnRecord record;
    record.moveText = moveText;
    record.gameID = static_cast<int>(gameCnt);

    t->board->fromMoveList(&record, bslib::Notation::san, flag);

    {
        std::lock_guard<std::mutex> dolock(nodeMapMutex);
        t->boardToNodes(nodeMap, paraRecord, resultString);
    }
}


// Process a game from a database
void BookMaker::processAGameWithAThread(ocgdb::ThreadRecord* t, const bslib::PgnRecord& record, const std::vector<int8_t>& moveVec)
{
    assert(t);
    t->init();
    assert(t->board);
    t->gameCnt++;

    t->board->newGame(record.fenText);
    
    int flag = bslib::BoardCore::ParseMoveListFlag_create_san;
    if (searchField == SearchField::moves) { // there is a text move only
        flag |= bslib::BoardCore::ParseMoveListFlag_quick_check;
        t->board->fromMoveList(&record, bslib::Notation::san, flag, checkToStop);
    } else {
        if (searchField == SearchField::moves1) {
            flag |= bslib::BoardCore::ParseMoveListFlag_move_size_1_byte;
        }
        
        t->board->fromMoveList(&record, moveVec, flag, checkToStop);
    }

    {
        std::lock_guard<std::mutex> dolock(nodeMapMutex);
        t->boardToNodes(nodeMap, paraRecord, record.result);
    }
}

void BookMaker::toBook()
{
    std::cout << "Exporting to book..." << std::endl;

    auto startTime2 = getNow();

    auto fenID = 1, transactionCnt = 0;
    for(auto && it : nodeMap) {
        if (it.second.hitCount() >= paraRecord.gamepernode) {
            if (!polyglotMode) {
                if (transactionCnt >= Transaction2Comit) {
                    sendTransaction(bookDb, false);
                    transactionCnt = 0;
                }
                if (transactionCnt == 0) {
                    sendTransaction(bookDb, true);
                }
                transactionCnt++;
            }

            add(fenID, it.first, it.second);
        }
    }

    int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(getNow() - startTime2).count() + 1;

    std::cout   << "Total tree nodes: " << nodeMap.size()
                << ", stored moves: " << itemCnt
                << ", elapsed: " << elapsed << " ms "
                << bslib::Funcs::secondToClockString(static_cast<int>(elapsed / 1000), ":")
                << std::endl;

    if (!polyglotMode) {
        updateInfoTable();

        if (transactionCnt > 0) {
            sendTransaction(bookDb, false);
        }
        
        if (insertStatement) {
            delete insertStatement;
            insertStatement = nullptr;
        }
        
        if (insertFENStatement) {
            delete insertFENStatement;
            insertFENStatement = nullptr;
        }
    }

    auto nodes = nodeMap.size();
    nodeMap.clear();
    
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(getNow() - startTime2).count() + 1;

    std::cout
                << "Exported, #games: " << gameCnt
                << ", #nodes: " << nodes
                << ", #items: " << itemCnt
                << ", elapsed: " << elapsed << " ms "
                << bslib::Funcs::secondToClockString(static_cast<int>(elapsed / 1000), ":")
                << ", speed: " << itemCnt * 1000 / elapsed << " nodes/s"
                << std::endl
                << std::endl;
}

void BookMaker::add(int& fenID, uint64_t key, const BookNode& node)
{
    std::vector<MoveWDL> moveWDLVec;
    for(auto && it : node.moveMap) {
        auto wdl = it.second;
        if (wdl.total() >= paraRecord.gamepernode) {
            MoveWDL moveWDL;
            moveWDL.m = it.first;
            moveWDL.win = wdl.win;
            moveWDL.draw = wdl.draw;
            moveWDL.loss = wdl.loss;
            moveWDLVec.push_back(moveWDL);
        }
    }
    
    if (moveWDLVec.empty()) {
        return;
    }
        
    if (polyglotMode) {
        add2Polyglot(key, moveWDLVec, node.isWhite());
    } else {
        add2Db(fenID, node.fen, moveWDLVec);
    }

    fenID++;
}

void BookMaker::add2Db(int& fenID, const std::string& fenString, const std::vector<MoveWDL>& moveWDLVec)
{
    if (two_tableMode) {
        insertFENStatement->reset();
        insertFENStatement->bind(":ID", fenID);
        insertFENStatement->bind(":FEN", fenString);
        if (insertFENStatement->exec() != 1) {
            return; // something wrong
        }
    }
    
    for(auto && mwdl : moveWDLVec) {
        insertStatement->reset();

        if (two_tableMode) {
            insertStatement->bind(":FENID", fenID);
        } else {
            insertStatement->bind(":FEN", fenString);
        }

        insertStatement->bind(":Move", mwdl.toChessCoordinateString());
        insertStatement->bind(":Win", mwdl.win);
        insertStatement->bind(":Draw", mwdl.draw);
        insertStatement->bind(":Loss", mwdl.loss);

        if (insertStatement->exec() != 1) {
            std::cout << "Error: " << insertStatement->getErrorMsg() << std::endl;
            return; // something wrong
        }
        itemCnt++;
    }
}

int BookMaker::scoreForPolyglot(const WinDrawLoss& a, bool isWhite) const
{
    auto win = a.win, loss = a.loss;
    if (!isWhite) {
        std::swap(win, loss);
    }
    auto score = win * paraRecord.winfactor + a.draw * paraRecord.drawfactor + loss * paraRecord.lossfactor;

    return score;
}

void BookMaker::add2Polyglot(uint64_t hashKey, std::vector<MoveWDL>& moveWDLVec, bool isWhite)
{
    assert(hashKey && !moveWDLVec.empty());
    
    // sort by values
    std::sort(moveWDLVec.begin(), moveWDLVec.end(),
          [=](const WinDrawLoss& a, const WinDrawLoss& b) -> bool {
        auto av = scoreForPolyglot(a, isWhite);
        auto bv = scoreForPolyglot(b, isWhite);
        return av == bv ? a.win > b.win : av > bv;
    });
    
    // calculate global scale to scale to 16-bit number
    auto maxScore = 0;
    for(auto && a : moveWDLVec) {
        // write down hashkey, score, learn
        auto score = scoreForPolyglot(a, isWhite);
        maxScore = std::max(maxScore, score);
    }
    
    double f = 1.0;
    if (maxScore > 0xffff) {
        f = 0xffff / static_cast<double>(maxScore);
    }

    assert(f <= 1);
    assert(polyglotFile.is_open());
    
    std::vector<bslib::BookPolyglotItem> vec;
    for(auto && a : moveWDLVec) {
        // write down hashkey, score, learn
        auto score = scoreForPolyglot(a, isWhite);
        if (score <= 0) {
            continue;
        }
        score = std::max(1, static_cast<int>(score * f));
        assert(score <= 0xffff);
        
        bslib::Move move;
        int castled;
        a.moveFromInt(move, castled);
        bslib::BookPolyglotItem item(hashKey, move.from, move.dest, move.promotion, castled, score);
        assert(item.key == hashKey && item.move);
        item.convertLittleEndian();
        vec.push_back(item);
    }
    polyglotFile.write((char*)vec.data(), sizeof(bslib::BookPolyglotItem) * vec.size());
}
