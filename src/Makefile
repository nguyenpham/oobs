# This file is part of Open Opening Book Standard.
#
# Copyright (c) 2022 Nguyen Pham (github@nguyenpham)
# Copyright (c) 2022 developers
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT)

all:
	g++ -std=c++17 -DNDEBUG -O3 -c *.cpp board/*.cpp ocgdb/*.cpp 3rdparty/SQLiteCpp/*.cpp
	gcc -DNDEBUG -O3 -c 3rdparty/sqlite3/*.c
	g++ -o oobs *.o -lpthread -ldl

clean:
	rm -f oobs *.o ./board/*.o ./ocgdb/*.o ./3rdparty/SQLiteCpp/*.o ./3rdparty/sqlite3/*.o
