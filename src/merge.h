/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OOBS_MERGE_H
#define OOBS_MERGE_H

#include "bookmaker.h"

#include "ocgdb/dbread.h"
#include "ocgdb/pgnread.h"
#include "ocgdb/parser.h"

namespace oobs {


class Merge : public ocgdb::DbRead
{
public:
    void processARecord(const std::map<std::string, std::string>& theMap);

protected:
    void queryADb(const std::string& dbPath);
    virtual void runTask() override;

private:
    void threadProcessARecord(const std::map<std::string, std::string>& theMap);
    void printStats() const override;

private:
    std::string query;
    ocgdb::Parser parser;
};

} // namespace oobs

#endif /* OOBS_MERGE_H */
