/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OOBS_EXPORT_H
#define OOBS_EXPORT_H

#include "bookmaker.h"

namespace oobs {


class Export : public BookMaker
{
public:
    void processARecord(const std::map<std::string, std::string>& theMap);

protected:
    void processOBS(const std::string& dbPath);
    virtual void runTask() override;

private:
    void threadProcessARecord(const std::map<std::string, std::string>& theMap);
//    void printStats() const override;
    

};

} // namespace oobs

#endif /* OOBS_EXPORT_H */
