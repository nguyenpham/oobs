/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OOBS_SEARCH_H
#define OOBS_SEARCH_H

#include "ocgdb/dbread.h"
#include "ocgdb/pgnread.h"

namespace oobs {


class Search : public ocgdb::DbRead
{
protected:
    void queryADb(const std::string& dbPath, const std::vector<std::string>& fenVec);
    void queryAPolyglot(const std::string& dbPath, const std::vector<std::string>& fenVec);

    virtual void runTask() override;
};

} // namespace oobs

#endif /* OOBS_SEARCH_H */
