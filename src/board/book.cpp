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

#include "book.h"

using namespace bslib;


void Book::loadData()
{
    loadData(path);

    bookStatus = isEmpty() ? BookStatus::BookStatus_bad : BookStatus::BookStatus_ok;
}


/////////////////////
BookPolyglotItem::BookPolyglotItem()
{
}

BookPolyglotItem::BookPolyglotItem(uint64_t _key, int from, int dest, int promotion, int castled, uint16_t _weight)
{
    key = _key;
    weight = _weight;
    learn = 0;
    move = fromMove(from, dest, promotion, castled);
}

bslib::Move BookPolyglotItem::getMove(const bslib::BoardCore* board) const
{
    auto df = move & 0x7, dr = move >> 3 & 0x7;
    auto dest = (7 - dr) * 8 + df;

    auto f = move >> 6 & 0x7, r = move >> 9 & 0x7;
    auto from = (7 - r) * 8 + f;

    // castling moves
    if (board && board->variant == bslib::ChessVariant::standard
        && dr == r && (from == 4 || from == 60)
        && (df == 0 || df == 7)
        && board->getPiece(from).type == bslib::KING) {
        dest = dest == 0 ? 2 : dest == 7 ? 6 : dest == 56 ? 58 : 62;
    }

    auto p = move >> 12 & 0x7;  /// 3 bits
    /// none 0, knight 1, bishop 2, rook 3, queen 4
    auto promotion = p == 0 ? bslib::EMPTY : (6 - p);
    return bslib::Move(from, dest, promotion);
}

uint16_t BookPolyglotItem::fromMove(const bslib::Hist& hist)
{
    return fromMove(hist.move.from, hist.move.dest, hist.move.promotion, hist.castled);
}

uint16_t BookPolyglotItem::fromMove(int from, int dest, int promotion, int castled)
{
    assert(bslib::Move::isValid(from, dest));

    // Special case
    if (castled && (from == 4 || from == 60) && abs(from - dest) == 2) {
        dest = dest == 2 ? 0 : dest == 6 ? 7 : dest == 58 ? 56 : 63;
    }

    auto f = dest & 0x7, r = dest >> 3 & 0x7;
    auto pdest = (7 - r) * 8 + f;

    f = from & 0x7; r = from >> 3 & 0x7;
    auto pfrom = (7 - r) * 8 + f;

    assert(pdest != pfrom);
    auto p = promotion == bslib::EMPTY ? 0 : (6 - promotion);
    return pdest | pfrom << 6 | p << 12;
}

void BookPolyglotItem::convertLittleEndian()
{
    key = (key >> 56) |
    ((key<<40) & 0x00FF000000000000UL) |
    ((key<<24) & 0x0000FF0000000000UL) |
    ((key<<8) & 0x000000FF00000000UL) |
    ((key>>8) & 0x00000000FF000000UL) |
    ((key>>24) & 0x0000000000FF0000UL) |
    ((key>>40) & 0x000000000000FF00UL) |
    (key << 56);
    
    move = (move >> 8) | (move << 8);
    weight = (weight >> 8) | (weight << 8);
    
    learn = (learn >> 24) | ((learn<<8) & 0x00FF0000) | ((learn>>8) & 0x0000FF00) | (learn << 24);
    
    assert(key && move);
}
/////////////////////



BookPolyglot::~BookPolyglot()
{
    clear();
}

void BookPolyglot::clear()
{
    if (items) {
        free(items);
        items = nullptr;
    }
    itemCnt = 0;
}

bool BookPolyglot::isEmpty() const
{
    return items == nullptr || itemCnt == 0;
}

size_t BookPolyglot::size() const
{
    return size_t(itemCnt);
}

void BookPolyglot::loadData(const std::string& _path)
{
    if (path == _path && readData) return;
    path = _path;
    readData = true;
    
    clear();

    std::ifstream ifs(path, std::ios::binary| std::ios::ate);
    
    int64_t length = 0;
    if (ifs.is_open()) {
        length = std::max<int64_t>(0, static_cast<int64_t>(ifs.tellg()));
    }
    
    assert(sizeof(BookPolyglotItem) == 16);
    itemCnt = length / static_cast<int64_t>(sizeof(BookPolyglotItem));
    
    if (itemCnt == 0) {
        std::cerr << "Error: cannot load book " << path << std::endl;
        return;
    }
    
    memallocCnt = itemCnt + 512;
    items = (BookPolyglotItem *) malloc(memallocCnt * static_cast<int64_t>(sizeof(BookPolyglotItem)) + 64);
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)items, length);
    
    for(int64_t i = 0; i < itemCnt; i++) {
        items[i].convertLittleEndian();
    }
}

bool BookPolyglot::isValid() const
{
    if (!Book::isValid()) {
        return false;
    }
    
    if (items) {
        uint64_t preKey = 0;
        for(int64_t i = 0; i < itemCnt; i++) {
            if (preKey > items[i].key) {
                return false;
            }
            preKey = items[i].key;
        }
    }

    return true;
}

int64_t BookPolyglot::binarySearch(uint64_t key) const
{
    int64_t first = 0, last = itemCnt - 1;
    
    while (first <= last) {
        auto middle = (first + last) / 2;
        if (items[middle].key == key) {
            for(; middle > 0 && items[middle - 1].key == key; middle--) {}
            return middle;
        }
        
        if (items[middle].key > key)  {
            last = middle - 1;
        } else {
            first = middle + 1;
        }
    }
    return -1;
}

// sort if the list is not in correct order
std::vector<BookPolyglotItem> BookPolyglot::search(uint64_t key) const
{
    std::vector<BookPolyglotItem> vec;
    
    auto k = binarySearch(key);
    if (k >= 0) {
        uint16_t weight = 0xffff;

        auto goodOrder = true;
        for(auto i = 0; k < int64_t(itemCnt) && items[k].key == key; k++, i++) {
            auto item = items[k];
            auto ok = weight >= item.weight;
            if (ok) {
                vec.push_back(item);
                weight = item.weight;
            } else {
                assert(i > 0);
                for(auto j = i - 1; j >= 0; j--) {
                    if (j == 0 || vec.at(j - 1).weight > item.weight) {
                        vec.insert(vec.begin() + j, item);
                        break;
                    }
                }
                weight = vec.back().weight;
                goodOrder = false;
            }
        }
#ifndef NDEBUG
        if (!goodOrder) {
            std::cout << "Warning: BookPolyglot::search weight order not good - re-sorted" << std::endl;
        }

//        weight = 0xffff;
//        for(auto && v : vec) {
//            assert(weight >= v.weight);
//            weight = v.weight;
//        }
#endif
    }

    return vec;
}


//bool BookPolyglot::merge(const Book* otherBook, BookMergeRule method, int fixValue)
//{
//    if (!otherBook || otherBook->type != BookType::polyglot) {
//        return false;
//    }
//
//    if (otherBook->isEmpty()) {
//        return true;
//    }
//
//    auto book = static_cast<const  BookPolyglot*>(otherBook);
//
//    auto sz = size() + book->size();
//    auto tmpItems = (BookPolyglotItem*) malloc(sz * sizeof(BookPolyglotItem));
//    auto q = tmpItems;
//
//    if (memallocCnt - itemCnt < book->size()) {
//        auto need = std::min<int>(book->size(), 1024);
//        extendDataBuffer(need);
//    }
//
//    auto p0 = items, end0 = p0 + itemCnt, p1 = book->items, end1 = p1 + book->itemCnt;
//
//    while(p0 < end0 && p1 < end1) {
//        if (p0->key != p1->key) {
//            if (p0->key < p1->key) {
//                for(auto key = p0->key; p0 < end0 && key == p0->key; ++p0) {
//                    *q = *p0; ++q;
//                }
//            } else {
//                for(auto key = p1->key; p1 < end1 && key == p1->key; ++p1) {
//                    *q = *p1; ++q;
//                }
//            }
//            continue;
//        }
//
//        std::map<u16, BookPolyglotItem> theMap;
//        auto key = p0->key;
//        for(; p0 < end0 && key == p0->key; ++p0) {
//            theMap[p0->move] = *p0;
//        }
//        for(; p1 < end1 && key == p1->key; ++p1) {
//            if (method == BookMergeRule::priority_book2 || theMap.find(p1->move) == theMap.end()) {
//                if (method == BookMergeRule::priority_book1)
//                    continue;
//                theMap[p1->move] = *p1;
//            } else {
//                //assert(method == BookMergeRule::add);
//                auto item = theMap[p1->move];
//                if (method == BookMergeRule::resetToFixValue) {
//                    item.weight += fixValue;
//                    item.learn = 0;
//                } else
//                if (method == BookMergeRule::sumup) {
//                    item.weight += p1->weight;
//                    item.learn += p1->learn;
//                } else if (method == BookMergeRule::best && item.weight < p1->weight) {
//                    item.weight = p1->weight;
//                    item.learn = p1->learn;
//                }
//                theMap[p1->move] = item;
//            }
//        }
//
//        for(auto && m : theMap) {
//            *q = m.second; ++q;
//        }
//    }
//
//
//    // Re-store
//    memallocCnt = sz;
//    itemCnt = q - tmpItems;
//    free(items);
//    items = tmpItems;
//
//    //assert(isValid());
//    return true;
//}


bool BookPolyglot::save(const std::string& path)
{
    if (!items) {
        return false;
    }

    // Write down
    int64_t length = itemCnt * static_cast<int64_t>(sizeof(BookPolyglotItem));

    auto tmpItems = (BookPolyglotItem *) malloc(length + 64);
    memcpy(tmpItems, items, length);
    for(int64_t i = 0; i < itemCnt; i++) {
        tmpItems[i].convertLittleEndian();
    }

    std::ofstream outFile(path, std::ofstream::binary| std::ofstream::trunc);
    if (outFile.is_open()) {
        outFile.write((char*)tmpItems, length);
    }
    outFile.close();
    free(tmpItems);

    return true;
}

