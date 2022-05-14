/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <iostream>
#include "bookmaker.h"
#include "search.h"
#include "bench.h"

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
        case ocgdb::Task::bench:
        {
            dbCore = new oobs::Bench;
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

void study()
{
    std::cout << "Generating..." << std::endl;
    auto startTime = ocgdb::Core::getNow();
    std::map<uint64_t, oobs::BookNode> nodeMap;
    for(int i = 0; i < 65 * 1024 * 1024; ++i) {
        auto hk = std::rand();
        
        oobs::BookNode node;
        nodeMap[hk] = node;
    }
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(ocgdb::Core::getNow() - startTime).count() + 1;

    std::cout << "#nodes: " << nodeMap.size()
              << ", elapsed: " << elapsed << " ms "
              << bslib::Funcs::secondToClockString(static_cast<int>(elapsed / 1000), ":")
              << std::endl;

    startTime = ocgdb::Core::getNow();
    std::cout << "Deleting..." << std::endl;

    nodeMap.clear();
    
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(ocgdb::Core::getNow() - startTime).count() + 1;

    std::cout << "Deleted: #nodes: " << nodeMap.size()
              << ", elapsed: " << elapsed << " ms "
              << bslib::Funcs::secondToClockString(static_cast<int>(elapsed / 1000), ":")
              << std::endl;
}

int main(int argc, const char * argv[])
{
//    study();
//    return 0;
    
    std::cout   << "Open Opening Book Standard - Book maker (C) 2022, ver "
                << oobs::VersionString
                << ", by Nguyen Pham\n"
                << std::endl;
    
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
        if (str == "-in") {
            paraRecord.inputPaths.push_back(std::string(argv[++i]));
            continue;
        }
        if (str == "-out") {
            paraRecord.outputPaths.push_back(std::string(argv[++i]));
            continue;
        }

        if (str == "-elo") {
            paraRecord.limitElo = std::atoi(argv[++i]);
            continue;
        }
//        if (str == "-wdl") {
//            auto string = std::string(argv[++i]);
//            paraRecord.setWDLFactors(string);
//            continue;
//        }
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
        if (str == "-plydelta") {
            paraRecord.ply_delta = std::atoi(argv[++i]);
            continue;
        }
        if (str == "-hit") {
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
    " -create               task: create a new book from multi PGN/OCGDB files, works with -in, -out\n" \
    " -q <EPD>              task: query info of EPDs for OOBS (.obs.db3), repeat to add multi files, works with -in\n" \
    " -bench                task: benchmark for OOBS (.obs.db3), works with -in\n" \
    " -in <path>            input path, repeat to add multi files. For task:\n" \
    "                       -create: must be OCGDB (.ocgdb.db3) or PGN (.pgn) file\n" \
    "                       -q: must be OOBS (.obs.db3) file\n" \
    "                       -bench: should be OOBS (.obs.db3) or Polyglot (.bin) file\n" \
    " -out <path>           opening book file. Extensions must be:\n" \
    "                         OOBS: .obs.db3\n" \
    "                         Polyglot: .bin\n" \
    "                         PGN: .pgn\n" \
    "                         EPD: .epd (Extended Position Description)\n" \
    " -elo <n>              discard games with Elo under n (for creating), default is 0\n" \
    " -plycount <n>         discard games with ply-count under n (for creating), default is 0\n" \
    " -plytake <n>          use first n moves to build openings, default is 20\n" \
    " -plydelta <n>         random range for plytake, use for PGN/EPD books, default is 1\n" \
    " -hit <n>              ignore nodes with number of games/hit under n, default is 1\n" \
    " -cpu <n>              number of threads, should <= total physical cores, omit it for using all cores\n" \
    " -desc \"<string>\"      a description to write to the table Info when creating a new database\n" \
    " -o [<options>,]       options, separated by commas\n" \
    "    discardnoelo       discard games without player Elos (for creating)\n" \
    "    win=<n>            set win factor for Polyglot, default is 2\n" \
    "    draw=<n>           set draw factor for Polyglot, default is 1\n" \
    "    loss=<n>           set loss factor for Polyglot, default is 0\n" \
    "    noresult=(discard|win|draw|loss)  how to treat games without results, default is discard\n" \
    "    uniquelastpos      PGN books: not allow same last positions\n" \
    "\n" \
    "Examples:\n" \
    " oobs -create -in big.png -out big.obs.db3\n" \
    " oobs -create -in big.png -out big.bin -cpu 4 -o win=3,draw=1,noresult=win\n" \
    " oobs -create -in big1.png -in big2.ocgdb.db3 -out allbig.obs.db3 -elo 2100 -hit 5\n" \
    " oobs -in big.obs.db3 -q \"K7/N7/k7/8/3p4/8/N7/8 w - - 0 1\"\n" \
    "\n" \
    "Main functions/features:\n" \
    "1. create opening books in formats OBS (.obs.db3), Polyglot (.bin), PGN (.pgn), EPD (.epd)\n" \
    "   from multi PGN/database (.ocgdb.db3) files\n" \
    "2. get/display all information for given EPD (Extended Position Description) strings from an OOBS (.obs) book\n"
    "3. benchmark\n"
    ;

    std::cerr << str << std::endl;
}
