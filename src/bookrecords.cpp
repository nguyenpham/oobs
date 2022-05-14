/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <sstream>
#include <fstream>

#include "bookrecords.h"


using namespace oobs;

int MoveWDL::move2Int(int from, int dest, int promotion, int castled)
{
    return from | dest << 8 | promotion << 16 | castled << 24;
}

void MoveWDL::moveFromInt(bslib::Move& move, int& castled) const
{
    move.from = m & 0xff;
    move.dest = (m >> 8) & 0xff;
    move.promotion = (m >> 16) & 0xff;
    castled = (m >> 24) & 0xff;
}

std::string MoveWDL::toChessCoordinateString() const
{
    bslib::Move move;
    int castled;
    moveFromInt(move, castled);
    return bslib::Move::toChessCoordinateString(move.from, move.dest, move.promotion);
}


