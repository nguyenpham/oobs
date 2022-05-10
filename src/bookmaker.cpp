/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
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
    assert(!paraRecord.outputPaths.empty());
    auto bookPath = paraRecord.outputPaths.front();
    // remove old db file if existed
    std::remove(bookPath.c_str());
    
    createBook();
}

void BookMaker::createBook()
{
    // init
    {
        std::cout << "Building opening trees from PGN files/databases..." << std::endl;
        gameCnt = itemCnt = discardCnt = 0;
        lastEpdSet.clear();
        
        // auto set flag to discard games with FENs
        paraRecord.optionFlag |= ocgdb::create_flag_discard_fen;
    }

    if (createBookFile()) {
        for(auto && path : paraRecord.inputPaths) {
            if (bslib::Funcs::endsWith(path, ".db3")) {
                readADb(path, "SELECT * FROM Games");
            } else {
                processPgnFile(path);
            }
        }

        toBook();
        printStats();
    }

    // clean up
    if (bookType == CreateBookType::polyglot) {
        textBookFile.close();
    }
}

bool BookMaker::createBookFile()
{
    auto bookPath = paraRecord.outputPaths.front();
    
    bookType = CreateBookType::none;

    if (bslib::Funcs::endsWith(bookPath, ".db3")) {
        bookType = CreateBookType::obs;
    } else if (bslib::Funcs::endsWith(bookPath, ".bin")) {
        bookType = CreateBookType::polyglot;
    } else if (bslib::Funcs::endsWith(bookPath, ".pgn")) {
        bookType = CreateBookType::pgn;
    } else if (bslib::Funcs::endsWith(bookPath, ".epd")) {
        bookType = CreateBookType::epd;
    } else {
        std::cerr << "Error: can't recognize the book extension. Stop!" << std::endl;
        return false;
    }
    
    switch(bookType) {
    case CreateBookType::obs:
    {
        paraRecord.ply_delta = 0;
        bookDb = createBookDb(bookPath, paraRecord.desc);
        
        // prepared statements
        if (bookDb) {
            if (two_tableMode) {
                const std::string fenSql = "INSERT INTO EPD (ID, EPD) VALUES (:ID, :EPD)";
                insertEPDStatement = new SQLite::Statement(*bookDb, fenSql);

                const std::string sql = "INSERT INTO Book (EPDID, Move, Win, Draw, Loss) "
                "VALUES (:EPDID, :Move, :Win, :Draw, :Loss)";

                insertStatement = new SQLite::Statement(*bookDb, sql);
            } else {
                const std::string sql = "INSERT INTO Book (EPD, Move, Win, Draw, Loss) "
                "VALUES (:EPD, :Move, :Win, :Draw, :Loss)";
                insertStatement = new SQLite::Statement(*bookDb, sql);
            }

            return true;
        }
        return false;
    }
    case CreateBookType::polyglot:
        paraRecord.ply_delta = 0;

    case CreateBookType::pgn:
    case CreateBookType::epd:
    {
        std::ios_base::openmode mode = std::ofstream::trunc | std::ofstream::out;
        if (bookType == CreateBookType::polyglot) {
            mode |= std::ofstream::binary;
        }

        textBookFile.open(bookPath, mode);
        if (textBookFile.is_open()) {
            std::cout << "Opened successfully to write the text book " << bookPath << std::endl;
            return true;
        }
        
        std::cerr << "Error: can't open to write the text book " << bookPath << std::endl;
        break;
    }
    default:
            assert(false);
        break;
    }

    return false;
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
        db->exec("INSERT INTO Info (Name, Value) VALUES ('Data Structure Version', '" + oobs::VersionDatabaseString + "')");

        // User Data version
        db->exec("INSERT INTO Info (Name, Value) VALUES ('Version', '" + ocgdb::VersionUserDatabaseString + "')");
        db->exec("INSERT INTO Info (Name, Value) VALUES ('License', 'free')");
        db->exec("INSERT INTO Info (Name, Value) VALUES ('Description', '" + dbDescription + "')");

        db->exec("DROP TABLE IF EXISTS Book");

        if (two_tableMode) {
            db->exec("DROP TABLE IF EXISTS EPD");
            db->exec("CREATE TABLE EPD (ID INTEGER PRIMARY KEY AUTOINCREMENT, EPD TEXT NOT NULL)");

            db->exec("CREATE TABLE Book (ID INTEGER PRIMARY KEY AUTOINCREMENT, EPDID INTEGER NOT NULL, Move TEXT, Active INTEGER DEFAULT 1, Win INTEGER, Draw INTEGER, Loss INTEGER, FOREIGN KEY(EPDID) REFERENCES EPD)");
        } else {
            db->exec("CREATE TABLE Book (ID INTEGER PRIMARY KEY AUTOINCREMENT, EPD TEXT NOT NULL, Move TEXT, Active INTEGER DEFAULT 1, Win INTEGER, Draw INTEGER, Loss INTEGER, Comment TEXT)");
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
    
    if (resultString.empty()) {
        if (paraRecord.optionFlag & ocgdb::flag_noresult_win) {
            resultString = "1-0";
        } else
        if (paraRecord.optionFlag & ocgdb::flag_noresult_draw) {
            resultString = "0.5-0.5";
        } else
        if (paraRecord.optionFlag & ocgdb::flag_noresult_loss) {
            resultString = "0-1";
        } else {
            return;
        }
    }

    assert(t);
    t->init();

    t->board->newGame(startFenString);
    
    int flag = bslib::BoardCore::ParseMoveListFlag_quick_check | bslib::BoardCore::ParseMoveListFlag_discardComment;

    bslib::PgnRecord record;
    record.moveText = moveText;
    record.result = resultString;
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
    
    if (bookType == CreateBookType::pgn || bookType == CreateBookType::epd) {
        setupRandonSavingPly();
        auto board = bslib::Funcs::createBoard(chessVariant);
        board->newGame("");
        std::set<uint64_t> vistedSet;
        toBookPgnEpd(board, vistedSet);
        delete board;
    } else {
        
        // Polyglot: calculate global scale to scale to 16-bit number
        polyglotScaleFactor = 1.0;
        if (bookType == CreateBookType::polyglot) {
            auto maxScore = 0;
            for(auto && it : nodeMap) {
                if (it.second.hitCount() >= paraRecord.gamepernode) {
                    for(auto && wdl : it.second.moveMap) {
                        auto score = scoreForPolyglot(wdl.second, it.second.isWhite());
                        maxScore = std::max(maxScore, score);
                    }
                }
            }
            
            if (maxScore > 0xffff) {
                polyglotScaleFactor = 0xffff / static_cast<double>(maxScore);
            }
        }
        
        for(auto && it : nodeMap) {
            if (it.second.hitCount() >= paraRecord.gamepernode) {
                if (bookType == CreateBookType::obs) {
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
    }

    if (bookType == CreateBookType::obs) {
        updateInfoTable();

        if (transactionCnt > 0) {
            sendTransaction(bookDb, false);
        }
        
        if (insertStatement) {
            delete insertStatement;
            insertStatement = nullptr;
        }
        
        if (insertEPDStatement) {
            delete insertEPDStatement;
            insertEPDStatement = nullptr;
        }
    } else if (textBookFile.is_open()) {
        textBookFile.close();
    }

    int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(getNow() - startTime2).count() + 1;

    std::cout   << "Total tree nodes: " << nodeMap.size()
                << ", stored moves: " << itemCnt
                << ", elapsed: " << elapsed << " ms "
                << bslib::Funcs::secondToClockString(static_cast<int>(elapsed / 1000), ":")
                << std::endl;

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

void BookMaker::toBookPgnEpd(bslib::BoardCore* board, std::set<uint64_t>& vistedSet)
{
    assert(board);

    // check if the position is in the tree and the hit number is accepted
    auto it = nodeMap.find(board->hashKey);
    if (    it == nodeMap.end()
            || it->second.hitCount() < paraRecord.gamepernode
            || vistedSet.find(board->hashKey) != vistedSet.end()) {
        return;
    }

    auto n = board->getHistListSize();
    if (n == randomSavingPly) {
        savePgnEpd(board);
        return;
    }

    // to avoid repeating forever
    vistedSet.insert(board->hashKey);

    auto legalCnt = 0;
    auto theSide = board->side;
    std::vector<bslib::MoveFull> moveList;
    board->_gen(moveList, theSide);

    for (auto && move : moveList) {
        if (board->_quickCheckMake(move.from, move.dest, move.promotion, bookType == CreateBookType::pgn)) {
            legalCnt++;
            toBookPgnEpd(board, vistedSet);
            board->_takeBack();
        }
    }

    if (!legalCnt && n >= std::max(1, paraRecord.ply_take - paraRecord.ply_delta)) {
        savePgnEpd(board);
    }

    vistedSet.erase(board->hashKey);
}

// If ply_delta = 0, every games may have the same length and side to move,
// set it a value to add a random factor to the length/side to move
void BookMaker::setupRandonSavingPly()
{
    randomSavingPly = paraRecord.ply_take;
    if (paraRecord.ply_delta > 0) {
        auto n = arc4random() % (2 * paraRecord.ply_delta + 1);
        randomSavingPly += paraRecord.ply_delta - n;
    }
}

void BookMaker::savePgnEpd(bslib::BoardCore* board)
{
    assert(board && board->getHistListSize() > 0);
    setupRandonSavingPly();

    std::string str;
    if (bookType == CreateBookType::pgn) {
        auto ok = true;
        if (paraRecord.optionFlag & ocgdb::flag_pgn_unique_lastpos) {
            auto epd = board->getEPD(false);
            ok = lastEpdSet.find(epd) == lastEpdSet.end();
            if (!ok) {
                lastEpdSet.insert(epd);
            }
        }

        if (ok) {
            str = "\n[Event \"?\"]\n[Site \"?\"]\n[Date \"????.??.??\"]\n[Round \"?\"]\n[White \"?\"]\n[Black \"?\"]\n[Result \"*\"]\n";

            auto eco = board->getLastFullEcoString();
            if (!eco.empty()) {
                auto vec = bslib::Funcs::splitString(eco, ';');
                if (vec.size() > 1) {
                    str += "[ECO \"" + vec.front() + "\"]\n[Opening \"" + vec.back() + "\"]\n";
                }
            }

            str += "\n"
                   + board->toMoveListString(bslib::Notation::san, 10, true, bslib::CommentComputerInfoType::none, true)
                   + " *\n\n";
        }
    } else if (bookType == CreateBookType::epd) {
        auto epd = board->getEPD(false);
        if (lastEpdSet.find(epd) == lastEpdSet.end()) {
            str = epd + "\n";
            lastEpdSet.insert(epd);
        }
    }


    if (!str.empty()) {
        assert(textBookFile.is_open());
        textBookFile.write(str.c_str(), str.length());
        itemCnt++;
    }
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
        
    switch (bookType) {
        case CreateBookType::obs:
            add2Db(fenID, node.epd, moveWDLVec);
            break;
        case CreateBookType::polyglot:
            add2Polyglot(key, moveWDLVec, node.isWhite());
            break;

        default:
            std::cerr << "Error: The book is not supported yet" << std::endl;
            return;
    }
    fenID++;
}

void BookMaker::add2Db(int& epdID, const std::string& epdString, const std::vector<MoveWDL>& moveWDLVec)
{
    if (two_tableMode) {
        insertEPDStatement->reset();
        insertEPDStatement->bind(":ID", epdID);
        insertEPDStatement->bind(":EPD", epdString);
        if (insertEPDStatement->exec() != 1) {
            return; // something wrong
        }
    }
    
    for(auto && mwdl : moveWDLVec) {
        insertStatement->reset();

        if (two_tableMode) {
            insertStatement->bind(":EPDID", epdID);
        } else {
            insertStatement->bind(":EPD", epdString);
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
    
    assert(textBookFile.is_open());
    
    std::vector<bslib::BookPolyglotItem> vec;
    for(auto && a : moveWDLVec) {
        // write down hashkey, score, learn
        auto score = scoreForPolyglot(a, isWhite);
        if (score <= 0) {
            continue;
        }
        score = std::max(1, static_cast<int>(score * polyglotScaleFactor));
        assert(score <= 0xffff);
        
        bslib::Move move;
        int castled;
        a.moveFromInt(move, castled);
        bslib::BookPolyglotItem item(hashKey, move.from, move.dest, move.promotion, castled, score);
        assert(item.key == hashKey && item.move);
        item.convertLittleEndian();
        vec.push_back(item);
    }
    textBookFile.write((char*)vec.data(), sizeof(bslib::BookPolyglotItem) * vec.size());
    itemCnt += moveWDLVec.size();
}
