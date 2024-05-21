#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <array>
#include "tuple.h"
#include <unordered_map>
#include "print.h"
#include <cassert>

constexpr int PAGESIZE = 1024;
constexpr int KEYSIZE = 64;

// struct Person {
//     int age;
//     std::string location;
    
// };

enum class TxType {
    READ, WRITE
};


struct Row {
    int tx_min; // just used to check how far back in time you can go with the undo links
    int tx_max; // used for the versioning - current version
    int valSize;
    int tid;
    char key[64];
};

struct Change {
    std::vector<bool> fieldsChanged;
    std::vector<char> oldData;
    bool isDeleted;
    int ts;
    Change* prev;
};

struct Transaction { // can read things that have lower txid and are commited
    bool isWrite;
    int tid_start;
    int tid_commit;
    int tid;
    bool isCommited;
    std::vector<Change*> changelog;
    std::vector<Row*> inserted;
};


struct Page {
    char* page;
    Page() {
        page = new char[PAGESIZE];
        memset(page, 0, PAGESIZE);
    }
};


struct DB {
    static constexpr int numPages = 2;
    std::array<Page*, numPages> data;
    std::atomic<int> currentPage = 0;
    std::atomic<int> nextOffset = 0;
    std::atomic<int> lastCommited = 1;
    std::atomic<int> nextTxId = 1;
    std::unordered_map<int, Transaction> tx_map;
    std::unordered_map<std::string, std::pair<int, int>> keytoRow;
    std::unordered_map<std::string, Change*> change_log;

    tuple tup;
    int rowSize;
    int valSize;

    DB (tuple& tup_) : tup(tup_) {
        for (int i = 0; i < numPages; ++i) {
            data[i] = new Page;
        }
        rowSize = sizeof(Row) - sizeof(void*) + tup.totalSize;
        valSize = tup.totalSize - 64;
    }
    //DB ()
    std::pair<Row *, char*> getSlot(int page, int offset) {
        
        void* orig = &(data[page]->page[offset*rowSize]);
        Row* res = (Row*)(&(data[page]->page[offset*rowSize]));
        void* valptr = (char*)(&res->key) + 64;
        std::pair<Row *, char*> retval = {res, (char*)valptr};
        return retval;
    }
    bool isRowVisible (Row& row, Transaction *tx) {
        if (row.tid == tx->tid) {
            return true;
        }
        // only want to read from transactions that commited before it started
        // start id is = the last commit stamp
        // the row's commit id must be less then or equal to tx's start
        // commit id of a row is assigned at commit time
        // if not commited it should be -1
        // each new commit increments the commit id
        // can you ahve more then one transaction committing at once
        bool ret = row.tx_max <= tx->tid_start && row.tx_max;
        if (ret) {
            assert((tx_map[row.tid].isCommited));
        }
        return ret;
    }

    Transaction* startTx (TxType type) {
        int txid = nextTxId.fetch_add(1);
        int start_id = lastCommited.load();
        assert(tx_map.count(txid) == 0);
        tx_map[txid] = Transaction{type == TxType::WRITE, start_id, 0, txid, false};
        return &tx_map[txid];
    }  

    bool commitTx (Transaction* tx) {
        int commitTime = lastCommited.fetch_add(1) + 1;
        tx_map[tx->tid].tid_commit = commitTime;
        tx_map[tx->tid].isCommited = true;
        // loop through all changes and mark their commit time
        for (Row* insertedRow: tx->inserted) {
            insertedRow->tx_max = commitTime;
        }
        return true;
    }   
    void printPage (char* str) {
        print("printing page");
        for (int i = 0 ; i < PAGESIZE; ++i) {
            if (str[i] == '\0') {
                std::cout << '-';
            } else {
                std::cout << str[i];
            }
            
        }
        std::cout << std::endl;

    }

    void insertOp(Transaction* tx, std::string key, std::vector<char> val) {
        /*
        requirements
        well if that key doesnt exist then go ahead;
        if it does exist then need to check some transactions
        so i think you need to point key to the first version of it and then iterate
        */
        if (keytoRow.count(key)) {
           return; 
        }

        int offset = nextOffset++;
        if (offset >= PAGESIZE) {
            currentPage+=1;
            offset = 0;
        }
        int page = currentPage;

        auto [row, valptr] = getSlot(page, offset);

        memset(valptr, 0, valSize);
        row->tx_min = tx->tid_start;
        row->valSize = valSize;
        row->tid = tx->tid;
        memcpy(&row->key, key.c_str(), key.size());
        memcpy(valptr, val.data(), val.size());
        tx->inserted.push_back(row);
        keytoRow[key] = {page, offset};
        change_log[key] = nullptr;
        
    }

    // char* getTupleAtTime(int page, int offset, int tx_start) {

    // }

    std::vector<char> getOp (Transaction* tx, std::string key) {
        if (!keytoRow.count(key)) {
            return {};
        }
        auto [page, offset] = keytoRow[key];
        auto [row, val] = getSlot(page, offset);
        //print(row, (void*)val);
        if (!isRowVisible(*row, tx)) {
            std::cout << "exit" <<std::endl;
            return {};
        }

        std::vector<char> retval (val, val + valSize);
        return retval;
    }

    


};

// template<typename T>
// struct MemTable {
    
//     void put(std::string key, T val) {
//         data.push_back({sizeof(key) + sizeof(val) + 2*sizeof(int), sizeof(key), key, val});
//     }

//     std::optional<T> get(std::string key) {
//         for (int i = 0; i < data.size(); ++i) {
//             if (data[i].key == key) {
//                 return data[i].val;
//             }
//         }
//         return {};
//     }

//     void del(std::string key) {
//         for (int i = 0; i < data.size(); ++i) {
//             if (data[i].key == key) {
//                 return data[i].val;
//             }
//         }
//         return {};
//     }
//     // right now ot add to the vector could cause a resize
//     //
// };


/*

flow
write transaction initiated
snapshotting?
string to string
read write
data format - size, size string string
can make durable later
ok so how would you design in memory
would just be a vector of rows
but should i have class or buffer
why not use hashmap, because of the disk format i believe

in memory kv store
add to disk, with WAL or not, so WALL adds to some log in disk in case memory is lost, but then how does the data eventually go to disk?, some timer, evictions?

can you have good concurrency without an index
locking and mvcc

locking, lock what data you read?
lock what data you write? rw locks?

mvcc? writes just append data
each transaction reads from the version of the data when its transaction started
isolation with locking or mvcc
well i dont 

how 

a put, would just modify the end of the structure
batched writes need to be atomic

what does it mean by level db does not support transactions



versioning
whenever a transaction ends the "next version canbe higher"



transactions  - cooler
    business logic

vs put / get operations - concurrent persistent hashmap


*/