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

int MoveWDL::move2Int(int8_t from, int8_t dest, int8_t promotion, int8_t castled)
{
    return static_cast<int>(from) | static_cast<int>(dest) << 8 | static_cast<int>(promotion) << 16 | static_cast<int>(castled) << 24;
}

void MoveWDL::moveFromInt(bslib::Move& move, int& castled) const
{
    move.from = static_cast<int>(m & 0xff);
    move.dest = static_cast<int>((m >> 8) & 0xff);
    move.promotion = static_cast<int>((m >> 16) & 0xff);
    castled = static_cast<int>((m >> 24) & 0xff);
}

std::string MoveWDL::toChessCoordinateString() const
{
    bslib::Move move;
    int castled;
    moveFromInt(move, castled);
    return bslib::Move::toChessCoordinateString(move.from, move.dest, move.promotion);
}


