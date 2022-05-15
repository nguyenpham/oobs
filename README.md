# Open Opening Book Standard (OOBS)


## Brief of main ideas/techniques
- Use SQL/SQLite as the backbone/framework for storing data and querying information
- All information is clear, meaningful, in full, understandable for both humans and machine


## Why OOBS? Features/Highlights
- Open databases: users could easily understand data structures, modify, convert to, or from other database formats
- It is based on SQL - the strongest query language for querying information. Users can work without using chess specific programs
- It could be used for all opening book purposes, from tournaments to normal games 
- All information is clear, full meaning, and understandable. It doesn't have "magic" numbers such as hash key numbers (which are no meaning for humans or even for computers when they stand-alone). It uses EPD (Extended Position Description) strings instead
- MIT license: you may use it for any applications/purposes unlimitedly without worrying about license conditions


## Overview
Almost all chess opening books are in text or binary formats. 

### Text

The most popular are PGN and EPD. Their strong points are human-readable, simple, editable with any text editor without the need of using specific software, and workable with almost all chess GUIs and tools. Their common problems are slow to process, need more computing/power to retrieve information, large sizes, contain errors, heavy redundancy, and missing vital information for book tasks. 

Basically, they aren’t designed and don’t have any specific structures/information for opening book purposes at all such as weights for moves. As the consequence, they are used mostly as small opening books with some simple uses such as for tournaments (when each match just needs a given position to start from). They almost can’t be used to play move by move because of lacking weights and being slow matching.

### Binary
They have some advantages such as being fast, compact, and typically having full opening book features. However, they also have some drawbacks:
 
- Hard to understand: each format requires a long list of descriptions and explanations why this but not that
- Hard to change structures: adding, removing, changing data types... are quite rare because of being hard and headache tasks, require change seriously both data and code
- Hard to support by other apps: other programs may get many problems to support since they may use different programming languages and data structures… In the case of using hash keys, it is not easy for an app to create hash keys for other ones
- Hard to support by web apps: processing large binary files is not a strong point of scripting languages/web apps
- License compatibility: code of some binary formats are from GPL to more restricted, thus some programs may get hard to adapt or even can't support 

Take a look at the Polyglot format closely as an example. It is quite popular, being de-facto standard format for chess opening books. However, it contains some drawbacks as below:
* Hash key: For each position, the book stored a hash key as a representation. Hash keys are actually "magic" numbers. They look random (actually they are first generated as random numbers) and have no meaning when standing alone. We almost can’t convert it back into a chess position. It is also a big challenge for other programs to create hash keys which are matched exactly the ones of the original Polyglot program. Furthermore, even though we have accepted Polyglot hash keys as a de factor standard ones for ordinary chess, we may get the chaos to apply that format to other chess variants (such as Xiangqi with board of 90 cells) since they don’t have any standard for their hash keys.
* Weight/score: when creating books, Polyglot calculates a weight for each move from numbers involving win/draw/loss games (typically the formula is weight = 2 wins + draw). All weights were then scaled globally down to fit numbers of 16 bits. Those numbers are good for comparison (to find out which moves are better than other ones). However, there are some problems: 1) the calculation is fixed. The default formula ignores completely losing numbers. Some users may love to select moves by win/loss rates instead 2) one way: we can’t get back numbers of win-draw-loss which are very useful for many purposes 3) inconsistent for merging, add games to an existing book: all weights of a book may be scaled automatically. However, the scale number is not stored anywhere. Thus new scores from merging, adding games can’t be scaled similar to the original ones, thus the resulting book may work inconsistently or even wrongly

The advantages of being binary formats were very important in the past when computers were weak and Internet speed was slow. However, they become less important nowadays since computers become much stronger and Internet is much faster. That has been making other formats become more attractive.

Thus we think we need a new format. Drawing requirements:
- Easier to understand data structures. It is best if everything  could describe themselves 
- Easy to support by different languages, apps, web apps
- Fast enough to use directly
- Easy to support other chess variants
- Almost free for all (MIT/or less restriction license)


## SQL/SQLite

We have picked up SQL/SQLite as the main tools/direction to develop the new opening book format. We have known it has some reason drawbacks such as slow and large on size.

However, it has some advantages:
- Data could be displayed in text forms, readable for both humans and machines
- Easy to alternate structures. Adding, removing, changing fields are so simple
- Easy to understand structures. They all are almost described themselves 
- Easy to write tools, converters
- Users can query directly
- Supported by a lot of tools
- SQL: is a very strong and flexible way to make queries
- Come with many strong, mature SQL engines and libraries


## EPD strings
Instead of using "magic" keys such as hash keys, we use EPD strings to represent/store chess positions. An EPD is a short form of a FEN when the last two numbers halfmove clock and fullmove are tripped. Those EPD should not have any operations.

### Legal en-passant
We count a “legal” en-passant only if the opponent has its Pawns in right positions to capture the advance Pawn, otherwise, store the string “-“ as there is no en-passant. It differs from popular EPD/FEN  strings when en-passant is always counted. The reason is that we support Polyglot format when en-passant is stored as above.


## Win-Draw-Loss numbers
We store directly those numbers (win-draw-loss) with the position. Whenever a program wants a score (say, to compare) it can compute on the fly from those numbers. Storing those numbers can prevent losing information. That is very useful for studying as well as updating and merging later.


## Comments
Comments by users


## Other fields
Polyglot has a field called “learn”. It is very rare to be used even in every position must-have. Thus we don’t mention it or any other field. They are not compulsory. However, users are totally free to add or remove them. Because of using SQL database, adding, removing, and having extra fields are simple and won’t affect other fields. We don’t have to worry about compatibility between them.


## File name extension
It should keep the normal extension of a database. For example, the one for SQLite should have the extension .db3. It should have upper/extra extension .obs to distinguish it from other databases.

For examples of file names:

```
  book-mb345.obs.db3
  book-lichess-elite.obs.db3
```


## One or two tables?
Basically, we need to store EPD strings and their belonging information such as moves and the win rate (Win-Draw-Loss) of those moves. That is the one-to-many relationship between EPD and move-win rate. To save some space as well as have a "perfect" design, those EPD strings should be stored in a separate table, say EPD table, while the move-win rate should be stored in another one, say, the MoveRate table and they will have a relationship (via their ID as FOREIGN KEY).

However, after studying and trying, we design to store all information in one table (name Book) only. The database is a bit larger but not too much (5-10%) compared with having two tables. The gain is that the design is simpler. More important, users can study, and edit data much easier.


## Book Table
It is the main table. It should have some fields: ID, EPD, Move, Active, Win, Draw, Loss

### ID
Just a normal ID number

### EPD field
A EPD string of each chess position could be stored in that field. The string should contain only the basic part and have not any extra item (operations).

### Move
Move which could be made from the board of the EPD string. The move is in the coordinate format.

### Active
This branch/move may be used only if it is active. Basically, we will use only 1 bit of that number. Other bits may be reserved for later use

### Win, Draw, Loss
Store numbers of won (1-0), draw (0.5-0.5), loss (0-1) games which were used to create the book


## Info Table
Store some extra information:
### Variant
Store chess variant information.

### ItemCount
Store number of records in Book table. It is useful for quickly retracting that number since querying by the statement "SELECT COUNT" could take a while for a huge database


## Sample databases
There is a sample opening book in the samples folder:
- book-mb3.45.obs.db3. It is created by games from the MillionBase database (by Ed Schröder) of 3.45 million games. Games are limited with at least 40-ply length, Elo >= 2000, repeat at least twice, take to ply 30, hit from 3 games. The book contains about 30 million positions.

You may open it with any SQLite browsers/tools and make some queries to understand its structures, speed, advantages, and disadvantages.


## Query

Typically we query all information of a given position with its EPD string:

```
SELECT * FROM Book WHERE EPD="rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR b KQkq -"
```

We may ask SQL engine to calculate scores from WDL and sort the result:

```
SELECT *, (2*Win + Draw) Score FROM Book WHERE EPD="rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR b KQkq -" ORDER BY Score DESC
```


## Speed and indexes
Since the main searching field is EPD - a text field and its data stored without sorting, the search may be slow for large databases. The sample database could be searched with the speed of about 10 positions per second on my computer. That speed may be good enough for normal use. However, it is slow for other purposes such as bullet games. Users could easily speed up the search by adding indexes for the field “EPD”. It is purely SQL statement:

```CREATE INDEX EPDIndex ON Book (EPD)```

With that index, the search speed could reach over 1600 positions per second, fast enough for any task/game/purpose.


## Compile
Run ```make``` in the folder src.


## Usage
Run program without any argument. It will display a short help how to use.


## License
MIT: Almost totally free (requires just fair use). All documents, codes, and data samples in this project are ready and free to integrate into other apps without worrying about license compatibility.


