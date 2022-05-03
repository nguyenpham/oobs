/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2021 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OOBS_MEM_H
#define OOBS_MEM_H

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>

namespace oobs {


class Mem
{
public:
    Mem(int addingSz, int thresholdSz)
    : addingSz(addingSz), thresholdSz(thresholdSz) { }
    Mem() { }

    virtual ~Mem() {
        if (buf) {
            free(buf);
            buf = nullptr;
        }
    }

    
protected:
    bool checkAndExpand() {
        if (error || allocatedSz - usedSz > thresholdSz) {
            return false;
        }
        
        auto newSz = allocatedSz + addingSz;
        auto newBuf = (char*)malloc(newSz);
        if (newBuf == nullptr) {
            error = true;
            return false;
        }
        
        memcpy(newBuf, buf, usedSz);
        free(buf);
        buf = newBuf;
        
        allocatedSz = newSz;
        return true;
    }

    
public:
    mutable std::mutex memMutex;

    bool error = false;
    char* buf = nullptr;
    
    int allocatedSz = 0, usedSz = 0;
    int thresholdSz = 4 * 1024;
    int addingSz = 4 * 1024 * 1024;

private:
    
};


class StringMng : public Mem
{
public:
    StringMng() : Mem(32 * 1024 * 1024, 128 * 1024) {}
    
    const char* get_byOrder(int ord) const {
        if (ord < itemCnt) {
            auto cnt = 0;
            for(char *p = buf, *end = p + usedSz; p < end; ++p) {
                if (!*p) {
                    cnt ++;
                    if (cnt == ord) {
                        return p;
                    }
                }
            }
        }
        return nullptr;
    }
    
    void add(const char* s) {
        checkAndExpand();
        auto len = strlen(s);
        strcpy(buf + usedSz, s);
        
        usedSz += len + 1;
        itemCnt++;
    }

    
    int itemCnt = 0;
};

class HashIdxItem
{
public:
    HashIdxItem() {}
    HashIdxItem(uint64_t hash, int idx) {
        set(hash, idx);
    }

    int32_t _hash[2];
    int32_t _idx;
    
    uint64_t getHash() const {
        return *((uint64_t*)_hash);
    }
    void set(uint64_t hash, int idx) {
        _idx = idx;
        *((uint64_t*)_hash) = hash;
    }
};


class HashIdx : public Mem
{
public:
    HashIdx() : Mem(32 * 1024 * 1024, 128 * 1024) {}
    
    
    HashIdxItem get_byHash(uint64_t hash) const {
        auto ord = binarySearch(hash);
        return get_byOrder(ord);
    }

    HashIdxItem get_byOrder(int ord) const {
        if (ord < itemCnt) {
            auto items = (HashIdxItem*) buf;
            return items[ord];
        }
        return HashIdxItem(0, -1);
    }

    HashIdxItem getadd_byHash(uint64_t hash) {
        auto items = (HashIdxItem*) buf;
        int64_t first = 0, last = itemCnt - 1, middle = -1;
    
        while (first <= last) {
            middle = (first + last) / 2;
            if (items[middle].getHash() == hash) {
                return items[middle];
            }
    
            if (items[middle].getHash() > hash) {
                last = middle - 1;
            } else {
                first = middle + 1;
            }
        }
    

        middle = std::max<int64_t>(0, std::min<int64_t>(middle, itemCnt));
        assert(sizeof(HashIdxItem) == 12);
        
        if (checkAndExpand()) {
            items = (HashIdxItem*) buf;
        }
        
        auto itemsz = sizeof(HashIdxItem);
        auto endSz = itemsz * (itemCnt - middle);
        auto point = middle * itemsz;

        char *q = (char*)items + point;
        memmove(q + itemsz, q, endSz);
        items[middle] = HashIdxItem(hash, itemCnt);
        itemCnt++;
            
        return items[middle];
    }

    uint itemCnt = 0;
    
private:
    int64_t binarySearch(uint64_t key) const
    {
        int64_t first = 0, last = itemCnt - 1;
        auto items = (HashIdxItem*) buf;

        while (first <= last) {
            auto middle = (first + last) / 2;
            if (items[middle].getHash() == key) {
                //for(; middle > 0 && items[middle - 1].key == key; middle--) {}
                return middle;
            }
            
            if (items[middle].getHash() > key)  {
                last = middle - 1;
            } else {
                first = middle + 1;
            }
        }
        return -1;
    }
};

} // namespace ocdb

#endif /* OOBS_MEM_H */
