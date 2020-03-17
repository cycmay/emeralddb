# emeralddb 
> A Sey-value Database System.
## Tools
 Boost/thread
 BSON

 ## Install And Use
 1. git clone this repository
 2. cd build/
 3. cmake ..
 4. make
 // And Then the executable file edb&emeralddb will be made in bin/
 5. run emralddb first
 6. run edb && input `connect hostname port` // note: in this program `connect localhost 12551` 
 7. input `insert {_id:1,key:"value"}`, the key-value will storage in data file.
 8. input `query {_id:1}`, the dataSystem will find the key in index and return the record
 9. input `delete {_id:1}`, the record and index of key will be erased in data file

