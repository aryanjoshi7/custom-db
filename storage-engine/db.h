#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <array>
#include "tuple.h"
#include <unordered_map>
#include "print.h"

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
    int tx_min;
    int tx_max;
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
    std::atomic<int> lastCommited = 0;
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
        std::cout << res  <<  std::endl;
        std::cout << valptr << std::endl;
        std::pair<Row *, char*> retval = {res, (char*)valptr};
        std::cout << "getslot return" << std::endl;
        return retval;
    }
    bool isRowVisible (Row& row, Transaction *tx) {
        // tx_min must be less than tid and commited
        // tx_max must be more than tid;
        print(row.tx_min,  tx->tid_start,  row.tx_max,  tx->tid_start, tx_map[row.tid].isCommited,  row.tid,  tx->tid);
        return row.tx_min <= tx->tid_start && row.tx_max >= tx->tid_start && (tx_map[row.tid].isCommited || row.tid == tx->tid);
    }

    Transaction* startTx (TxType type) {
        int txid = nextTxId.fetch_add(1);
        int start_id = lastCommited.load();
        assert(tx_map.count(txid) == 0);
        tx_map[txid] = Transaction{type == TxType::WRITE, start_id, 0, txid, false};
        return &tx_map[txid];
    }  

    bool commitTx (Transaction* tx) {
        int commitTime = nextTxId.fetch_add(1) + 1;
        tx_map[tx->tid_start].tid_commit = commitTime;
        tx_map[tx->tid_start].isCommited = true;
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
        // if (fwrite(str, 1, 1024, stdout) != 1024) {
        //     print("error printing");
        // }

    }

    void insertOp(Transaction* tx, std::string key, std::vector<char> val) {
        /*
        requirements
        well if that key doesnt exist then go ahead;
        if it does exist then need to check some transactions
        so i think you need to point key to the first version of it and then iterate
        */
        printPage(data[0]->page);
        if (keytoRow.count(key)) {
           return; 
        }

        int offset = nextOffset++;
        if (offset >= PAGESIZE) {
            currentPage+=1;
            offset = 0;
        }
        int page = currentPage;

        std::pair<Row*, char*> slot = getSlot(page, offset);
        Row* row = slot.first;
        char* valptr = slot.second;
        memset(valptr, 0, valSize);
        row->tx_min = tx->tid_start;
        row->valSize = valSize;
        row->tid = tx->tid;
        print("tid is ", row->tid);
        print("valsize is ", tup.totalSize);

        printPage(data[0]->page);
        memcpy(&row->key, key.c_str(), key.size());
        printPage(data[0]->page);
        print(val);
        memcpy(valptr + 64, val.data(), val.size());
        printPage(data[0]->page);

        keytoRow[key] = {page, offset};
        change_log[key] = nullptr;
        
    }

    // char* getTupleAtTime(int page, int offset, int tx_start) {

    // }

    std::vector<char> getOp (Transaction* tx, std::string key) {
        printPage(data[0]->page);
        if (!keytoRow.count(key)) {
            return {};
        }
        auto [page, offset] = keytoRow[key];
        std::cout << page << " " << offset << std::endl;
        auto [row, val] = getSlot(page, offset);
        std::cout << "jhee" <<std::endl;
        
        if (!isRowVisible(*row, tx)) {
            std::cout << "exit" <<std::endl;
            return {};
        }
        print("val size ", row->valSize);

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