/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2021 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef BOOKRECORDS_H
#define BOOKRECORDS_H


#include <vector>
#include <unordered_map>

#include "3rdparty/SQLiteCpp/SQLiteCpp.h"
#include "3rdparty/threadpool/thread_pool.hpp"
#include "board/types.h"
#include "board/base.h"

namespace oobs {


class WinDrawLoss {
public:
    int win = 0, draw = 0, loss = 0;

    void addFrom(const WinDrawLoss& bnm) {
        win += bnm.win;
        draw += bnm.draw;
        loss += bnm.loss;
    }

    int total() const {
        return win + draw + loss;
    }
    
    bool isValid() const {
        return total() > 0;
    }
};

class MoveWDL : public WinDrawLoss {
public:
    static int move2Int(int8_t from, int8_t dest, int8_t promotion, int8_t castled);

    void moveFromInt(bslib::Move& move, int& castled) const;
    std::string toChessCoordinateString() const;

public:
    int m;

};

class BookNode {
public:
    std::string fen;
    
    // map move int to BookNodeMove
    std::unordered_map<int, WinDrawLoss> moveMap;

    bool isValid() const {
        if (fen.empty()) {
            return false;
        }
        
        for(auto && it : moveMap) {
            if (it.first == 0 || !it.second.isValid()) {
                return false;
            }
        }
        return true;
    }
    
    int hitCount() const {
        int cnt = 0;
        for(auto && it : moveMap) {
            cnt += it.second.total();
        }
        return cnt;
    }
    
    void addFrom(const BookNode& node) {
        assert(fen == node.fen);
        assert(node.isValid());

        for(auto && m : node.moveMap) {
            auto it = moveMap.find(m.first);
            if (it == moveMap.end()) {
                moveMap[m.first] = m.second;
            } else {
                moveMap[m.first].addFrom(m.second);
            }
        }
        
        assert(isValid());
    }
    
    bool isWhite() const {
        return fen.find(" w ") != std::string::npos;
    }
};



} // namespace oobs

#endif /* BOOKRECORDS_H */
