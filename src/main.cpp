/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2021 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <iostream>
#include "bookmaker.h"
#include "search.h"

#include "board/chess.h"

#include "records.h"
#include "report.h"

void print_usage();
extern bool debugMode;

void runTask(ocgdb::ParaRecord& param)
{
    ocgdb::DbCore* dbCore = nullptr;

    switch (param.task) {
        case ocgdb::Task::create:
        {
            dbCore = new oobs::BookMaker;
            break;
        }
        case ocgdb::Task::query:
        {
            dbCore = new oobs::Search;
            break;
        }

        default:
            break;
    }
    
    if (dbCore) {
        dbCore->run(param);
        delete dbCore;
    }
}

void printConflictedTasks(ocgdb::Task task0, ocgdb::Task task1)
{
    std::cerr << "Error: multi/conflicted tasks: " << ocgdb::ParaRecord::toString(task0) << " vs "  << ocgdb::ParaRecord::toString(task1) << std::endl;
}

int main(int argc, const char * argv[]) {
    std::cout << "Open Opening Book Standard - Book maker (C) 2022\n";
    
    if (argc < 2) {
        print_usage();
        return 0;
    }

    // init
    {
        bslib::ChessBoard::staticInit();
    }

    auto errCnt = 0;
    ocgdb::ParaRecord paraRecord;
    
    for(auto i = 1; i < argc; i++) {
        auto oldTask = paraRecord.task;
        auto str = std::string(argv[i]);
        if (str == "-bench") {
            paraRecord.task = ocgdb::Task::bench;
            continue;
        }

        if (str == "-debug") {
            debugMode = true;
            continue;
        }

        if (str == "-create" || str == "-merge" || str == "-export") {
            if (str == "-create") {
                paraRecord.task = ocgdb::Task::create;
            } else if (str == "-merge") {
                paraRecord.task = ocgdb::Task::merge;
            } else if (str == "-export") {
                paraRecord.task = ocgdb::Task::export_;
            }
            if (oldTask != ocgdb::Task::none) {
                errCnt++;
                printConflictedTasks(oldTask, paraRecord.task);
            }
            continue;
        }

        if (i + 1 >= argc) continue;;

        if (str == "-cpu") {
            paraRecord.cpuNumber = std::atoi(argv[++i]);
            continue;
        }
        if (str == "-pgn") {
            paraRecord.pgnPaths.push_back(std::string(argv[++i]));
            continue;
        }
        if (str == "-db") {
            paraRecord.dbPaths.push_back(std::string(argv[++i]));
            continue;
        }
        if (str == "-book") {
            paraRecord.bookPaths.push_back(std::string(argv[++i]));
            continue;
        }

        if (str == "-elo") {
            paraRecord.limitElo = std::atoi(argv[++i]);
            continue;
        }
        if (str == "-wdl") {
            auto string = std::string(argv[++i]);
            paraRecord.setWDLFactors(string);
            continue;
        }
        if (str == "-o") {
            auto string = std::string(argv[++i]);
            paraRecord.setupOptions(string);
            continue;
        }
        if (str == "-plycount") {
            paraRecord.limitLen = std::atoi(argv[++i]);
            continue;
        }
        if (str == "-plytake") {
            paraRecord.ply_take = std::atoi(argv[++i]);
            continue;
        }
        if (str == "-gamepernode") {
            paraRecord.gamepernode = std::atoi(argv[++i]);
            continue;
        }
        if (str == "-desc") {
            paraRecord.desc = std::string(argv[++i]);
            continue;
        }
        if (str == "-q") {
            paraRecord.task = ocgdb::Task::query;
            auto query = std::string(argv[++i]);
            paraRecord.queries.push_back(query);
            continue;
        }

        errCnt++;
        std::cerr << "Error: unknown parameter: " << str << "\n" << std::endl;
        break;
    }
    
    if (errCnt == 0) {
        if (debugMode) {
            std::cout << "All parameters:\n" << paraRecord.toString() << std::endl;
        }
        
        if (paraRecord.isValid()) {
            runTask(paraRecord);
            return 0;
        }
        
        auto errorString = paraRecord.getErrorString();
        if (!errorString.empty()) {
            std::cerr << "Error: " << errorString << "\n" << std::endl;
        }
    }

    print_usage();
    return 1;
}


void print_usage()
{
    const std::string str =
    "Usage:\n" \
    " oobs [<parameters>]\n" \
    "\n" \
    " -create               create a new database from multi PGN files, works with -book, -db, -pgn\n" \
    " -book <file>          opening book file, repeat to add multi files. Extensions should be .obs.db3 for SQLite format, .bin for Polyglot\n" \
    " -pgn <file>           PGN game database file, repeat to add multi files\n" \
    " -db <file>            database file in OCGDB format, extension should be .ocgdb.db3, repeat to add multi files\n" \
    " -elo <n>              discard games with Elo under n (for creating)\n" \
    " -plycount <n>         discard games with ply-count under n (for creating)\n" \
    " -plytake <n>          use first n moves to build openings\n" \
    " -gamepernode <n>      count node with having at least n games\n" \
    " -wdl <w,d,l>          win, draw, loss factors for Polyglot books. Default is 2,1,0\n" \
    " -cpu <n>              number of threads, should <= total physical cores, omit it for using all cores\n" \
    " -desc \"<string>\"      a description to write to the table Info when creating a new database\n" \
    "\n" \
    "Examples:\n" \
    " oobs -create -pgn big.png -book big.obs.db3 -cpu 4\n" \
    " oobs -create -pgn big.png -book big.bin -cpu 4 -wdlfactors 3,1,0\n" \
    " oobs -create -pgn big1.png -pgn big2.png -db big3.ocgdb.db3 -book allbig.obs.db3 -elo 2100\n" \
    " oobs -book big.obs.db3 -q \"K7/N7/k7/8/3p4/8/N7/8 w - - 0 1\"\n" \
    "\n" \
    "Main functions/features:\n" \
    "1. create an opening book in formats OBS (.obs.db3), Polyglot (.bin) from multi PGN/database (.ocgdb.db3) files\n" \
    "2. get/display all information for given FEN strings from an OBS book\n"
    ;

//    " -o [<options>,]       options, separated by commas\n" \
//    "    discardnoelo       discard games without player Elos (for creating)\n" \
//    "    discardfen         discard games with FENs (not started from origin; for creating)\n" \

    std::cerr << str << std::endl;
}
