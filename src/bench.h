/**
 * This file is part of Open Opening Book Standard.
 *
 * Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2022 Developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef OOBS_BENCH_H
#define OOBS_BENCH_H

#include "search.h"

namespace oobs {


class Bench : public Search
{
private:
    virtual void runTask() override;
};

} // namespace oobs

#endif /* OOBS_BENCH_H */
