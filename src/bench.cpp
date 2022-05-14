/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "bench.h"

#include "board/book.h"

using namespace oobs;

void Bench::runTask()
{
    std::cout   << "Benchmark..." << std::endl;

    if (paraRecord.inputPaths.empty()) {
        std::cout << "Error: there is no path for input files" << std::endl;
        return;
    }

    assert(paraRecord.task == ocgdb::Task::bench);

    gameCnt = 0; errCnt = 0; succCount = 0;
    
    // print out
    // paraRecord.optionFlag |= ocgdb::query_flag_print_all;

    std::vector<std::string> benchVec{
        "rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR b KQkq - 0 1",
        "r3kbnr/pp1b1ppp/1qn1p3/3pP3/1p1P4/P1P2N2/5PPP/RNBQKB1R w KQkq - 0 1",
        "r1bqkb1r/3n1ppp/p1p1p3/1p1nP3/3P4/2NQ1N2/PP3PPP/R1B1KB1R w KQkq - 0 1",
        "r1bqr1k1/ppp1bppp/3p1n2/8/2BnP3/2N4P/PPP2PP1/R1BQR1K1 w - - 0 1",
        "r2q1rk1/pp1bppbp/2np1np1/8/2P5/2N3P1/PPN1PPBP/R1BQ1RK1 w - - 0 1",
        "r2qk2r/pp3ppp/2n1bn2/1Bbp4/8/1PN1PN2/P4PPP/R1BQ1RK1 b kq - 0 1",
        "r1bq1rk1/4bppp/p4n2/np1p4/5B2/2NQ1N2/PPB2PPP/R3R1K1 b - - 0 1",
        "r1bq1k1r/pp4p1/1bn1ppp1/3pP3/1P1N4/P4NQ1/1BP2PPP/R3K2R b KQ - 0 1",
        "r2qkb1r/pppn1ppp/2b1pn2/6N1/3P4/3B1N2/PPP1QPPP/R1B1K2R b KQkq - 0 1",
        "r3k2r/pp1qppbp/2n3p1/2p5/3PP3/2P1BN2/P4PPP/R2QK2R w KQkq - 0 1",
        "r1bqk2r/ppp2pbp/2n3p1/3np3/8/2NP1NP1/PPP2PBP/R1BQ1RK1 b kq - 0 1",
        "r1bqk2r/1p2bppp/p1n1pn2/3p4/1PP5/P4NP1/1B2PPBP/RN1Q1RK1 b kq - 0 1",
        "r1b2rk1/ppq1ppbp/2np1np1/2p5/4PP2/2PP1NP1/PP2Q1BP/RNB2RK1 b - - 0 1",
        "r1bqkb1r/ppp2ppp/2np1n2/4p3/4P3/2PP1NP1/PP3P1P/RNBQKB1R b KQkq - 0 1",
        "r1bq1rk1/pp1nbppp/4p3/2p5/3PN2P/3B1N2/PPPQ1PP1/2KR3R b - h3 0 1",
        "r1b1kb1r/pp1n1ppp/4p3/n2pP3/3P1P2/PqN2N2/1P4PP/R1BQKB1R b KQkq - 0 1",
        "r3kb1r/ppqn1ppp/2p5/4nb2/P1N2B2/2N3P1/1P2PP1P/R2QKB1R w KQkq - 0 1",
        "r2q1rk1/1b1nbppp/pp1ppn2/8/2P5/1PN2NP1/PB1QPPBP/R2R2K1 b - - 0 1",
        "rnbqkb1r/pp2pppp/2p2n2/3p4/2PP4/4PN2/PP3PPP/RNBQKB1R b KQkq c3 0 1",
        "r1bqkbnr/pp1p1ppp/4p3/1Np5/3PP3/5N2/PPP2PPP/R1BQK2R b KQkq d3 0 1",
        "rnbq1rk1/pp3pbp/2pp1np1/4p3/2P5/2NPP1P1/PP2NPBP/R1BQK2R w KQ e6 0 1",
        "rnbqk2r/ppp2ppp/4pn2/8/2pP4/P4NP1/1P1BPP1P/R2QKB1R b KQkq - 0 1",
        "r1bqk2r/ppp1nppp/1bn5/3p4/1P1P4/P1N1B3/2P1NPPP/R2QKB1R w KQkq - 0 1",
        "rn1q1rk1/1p2bppp/2p1pn2/p2p4/2P3bN/1P1P2P1/PB2PPBP/RN1Q1RK1 w - - 0 1",
        "rnbqk2r/pp2b1pp/2p1pn2/3p1p2/2PP4/5NP1/PP2PPBP/RNBQ1RK1 w kq - 0 1",
        "r1bqk1nr/pp2bppp/2n1p3/2ppP3/5P2/2N2N2/PPPP2PP/R1BQKB1R w KQkq - 0 1",
        "3rkb1r/pppq1ppp/5n2/8/2Qn4/6P1/PP3PBP/RNB2RK1 w k - 0 1",
        "r1bqk1nr/ppp1ppbp/2np2p1/8/3PPB2/2N5/PPP2PPP/R2QKBNR w KQkq - 0 1",
        "r1b1k2r/pp3ppp/2n1pn2/q7/1b1P4/2NB1N2/PP3PPP/R1BQ1RK1 w kq - 0 1",
        "r1bqkb1r/pp3ppp/2p2n2/2p1p3/4P3/2NP1N2/PPP2PPP/R1BQK2R b KQkq - 0 1",
    };

    std::vector<std::string> fenVec;
    
    for(auto i = 0; i < 10; i++) {
        for(auto && query : benchVec) {
            auto s = bslib::Funcs::FEN2EPD(query);
            if (!s.empty()) {
                fenVec.push_back(s);
            }
        }
    }

    // Query databases
    for(auto && dbPath : paraRecord.inputPaths) {
        startTime = getNow();
        if (bslib::Funcs::endsWith(dbPath, ".db3")) {
            queryADb(dbPath, fenVec);
        } else if (bslib::Funcs::endsWith(dbPath, ".bin")) {
            queryAPolyglot(dbPath, fenVec);
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(getNow() - startTime).count() + 1;

        std::cout
                    << "Benchmark, #FENs: " << fenVec.size()
                    << ", elapsed: " << elapsed << " ms "
                    << bslib::Funcs::secondToClockString(static_cast<int>(elapsed / 1000), ":")
                    << ", speed: " << fenVec.size() * 1000 / elapsed << " nodes/s"
                    << std::endl
                    << std::endl;

    }
}

