/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2021 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OOBS_BOOK_H
#define OOBS_BOOK_H

#include <stdio.h>

#include "chess.h"

namespace bslib {

enum class BookType {
    polyglot, obs, none
};

enum class BookStatus {
    BookStatus_unknow, BookStatus_ok, BookStatus_bad
};

class Book
{
public:
    Book(BookType t) : type(t) { variant = bslib::ChessVariant::standard; }
    virtual ~Book() {}
    
    virtual bool isEmpty() const = 0;
    virtual size_t size() const = 0;

    virtual bool isValid() const {
        return !name.empty() && !path.empty();
    }
    
    bool isStatusOk() const {
        return bookStatus == BookStatus::BookStatus_ok;
    }

    virtual bool save() { return save(path); }
    virtual bool save(const std::string&) { return false; }
//    virtual bool save(const BookMove*, ChessVariant) { return false; }

public:
    virtual void loadData();
    virtual void loadData(const std::string& path) = 0;
    BookType type;
    
    std::string name, path;
    bslib::ChessVariant variant;
    BookStatus bookStatus = BookStatus::BookStatus_unknow;
    bool mode, white, black, readData = false;
};

class BookPolyglotItem {
public:
    BookPolyglotItem();
    BookPolyglotItem(uint64_t, int from, int dest, int promotion, int castled, uint16_t);

    bslib::Move getMove(const bslib::BoardCore* board) const;
    static uint16_t fromMove(const bslib::Hist& hist);
    static uint16_t fromMove(int from, int dest, int promotion, int castled);

    void convertLittleEndian();

public:
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};


class BookPolyglot : public Book
{
public:
    BookPolyglot() : Book(BookType::polyglot) {}

    virtual ~BookPolyglot() override;
    
    void clear();

    virtual bool isValid() const override;

    bool isEmpty() const override;
    size_t size() const override;

    void loadData(const std::string& path) override;

    std::vector<BookPolyglotItem> search(uint64_t key) const;

    int64_t binarySearch(uint64_t key) const;

        // for saving
    virtual bool save(const std::string&) override;

    int64_t itemCnt = 0, memallocCnt = 0;
    BookPolyglotItem* items = nullptr;
};


} // namespace ocdb

#endif /* OOBS_BOOK_H */
