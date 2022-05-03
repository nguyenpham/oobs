/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2021 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OOBS_SEARCH_H
#define OOBS_SEARCH_H

#include "dbread.h"
#include "pgnread.h"

namespace oobs {


class Search : public ocgdb::DbRead
{
private:
    void queryADb(const std::string& dbPath, const std::vector<std::string>& fenVec);
    void queryAPolyglot(const std::string& dbPath, const std::vector<std::string>& fenVec);

    virtual void runTask() override;
};

} // namespace ocdb

#endif /* OOBS_SEARCH_H */
