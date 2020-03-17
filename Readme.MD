# emeralddb 
> A Key-value Database System.
## Tools
 Boost/thread
 BSON

## Envirements
1. Install Boosts. See offcial document such as tutorial
2. Install bson. just copy bson-cxx to src //note: in this respority already copyed

## Install And Use
 1. git clone this repository
 2. run shell script `build.sh` to make.
 > And Then the executable file edb&emeralddb will be made in bin/
 3. run emralddb first
 4. run edb && input `connect hostname port`
 > note: in this program `connect localhost 12551` 
 5. input `insert {_id:1,key:"value"}`
 > the key-value will be storaged in data file.
 6. input `query {_id:1}`
 > the dataSystem will find the key in index and return the record
 7. input `delete {_id:1}`
 > the record and index of key will be erased in data file

------
More information about this system, please click [here--emeraldDB](https://github.com/cycmay/notes/blob/master/DataBase/emeraldDB-implements.md)

