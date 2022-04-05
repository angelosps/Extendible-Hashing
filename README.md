# 🗃️ Extendible Hashing 

A block level implementation of [Extendible Hashing](https://en.wikipedia.org/wiki/Extendible_hashing) for both _primary_ and _secondary_ indexing in database systems.

## A tiny database example 

The next images show the hashing of the following records using both primary and secondary indexing.

ID | NAME      | SURNAME       | CITY
-- | --------- | ------------- | ----
0  | Dimitrios | Hardy         | Los Angeles
1  | Gabriel   | Kyriakopoulos | Philadelphia
2  | Angelos   | Sheeran       | New Work
3  | Will      | Jolie         | Milwaukee  
4  | Angelina  | Antetokounmpo | London
5  | Giannis   | Antetokounmpo | Athens
6  | Gabriel   | Rontogiannis  | Halifax
7  | Dimitrios | Smith         | Athens  
8  | Dimitrios | Smith         | Los Angeles

## Primary Indexing Extendible Hashing
<img src="./images/primary_eh.png" width = 600>

## Secondary Indexing Extendible Hashing
**Note:** The secondary hashing is performed on the ID and Surname keys, as shown in the following screenshot.
<img src="./images/secondary_eh.png">

## Usage
A block level library named `BlockFile` is given for managing the memory.  
**Warning!** The library is _only_ compatible with _linux_ machines, so you must be connected to a linux machine to run this repository.
In order to run primary and/or secondary extendible hashing indexing just open a terminal in the current directory and do the following:

``` 
$ make primary    # Compiles primary indexing
$ make secondary  # Compiles secondary indexing
$ make runp       # Runs primary
$ make runs       # Runs secondary

$ make clean 
```

## Authors
* [Angelos Poulis](https://github.com/angelosps)
* [Dimitrios Kyriakopoulos](https://github.com/dimitrskpl)
* [Dimitrios Rontogiannis](https://github.com/rondojim)
