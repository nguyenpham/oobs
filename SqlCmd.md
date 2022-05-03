# Basic SQL command


## Create database and tables

    DROP TABLE IF EXISTS Info
    CREATE TABLE Info (Name TEXT UNIQUE NOT NULL, Value TEXT);
    INSERT INTO Info (Name, Value) VALUES ('Version', '0.1');
    INSERT INTO Info (Name, Value) VALUES ('Variant', 'standard');
    INSERT INTO Info (Name, Value) VALUES ('License', 'free');

    DROP TABLE IF EXISTS Book;
    CREATE TABLE Book (ID INTEGER PRIMARY KEY AUTOINCREMENT, FEN TEXT NOT NULL, Move TEXT, Active INTEGER DEFAULT 1, Win INTEGER, Draw INTEGER, Loss INTEGER)


## Example of insert commands
    INSERT INTO Info (Name, Value) VALUES ('Version', '0.1');
    INSERT INTO Info (Name, Value) VALUES ('Variant', 'standard');
    INSERT INTO Info (Name, Value) VALUES ('License', 'free');

    INSERT INTO Book (FEN, Move, Win, Draw, Loss) VALUES (?, ?, ?, ?, ?);
        
## Example of querying a FEN
    SELECT * FROM Book WHERE FEN = ?
