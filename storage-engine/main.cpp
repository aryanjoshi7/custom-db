#include <iostream>
#include "db.h"
#include <cassert>

struct Person {
    int age;
    std::string location;

    std::vector<char> serialize() {
        char buffer[4 + 64];
        memset(&buffer, 0, 68);
        memcpy((char*)(&buffer), &age, sizeof(age));
        memcpy((char*)(&buffer) + 4, location.c_str(), location.size());
        std::vector<int> intbuf (buffer, buffer + 68);
        std::vector<char> vecbuffer (buffer, buffer + 68);
        return vecbuffer;
    }
    static Person deserialize(std::vector<char> vec) {
        Person p;
        if (!vec.size()) {
            p = {0,""};
            return p;
        }
        int* ageptr = (int*)&vec[0];
        p.age = *ageptr;
        char* locationptr = (char*)&vec[4];
        p.location = locationptr;
        return p;
    }
    bool operator==(Person& other) {
        return other.age == age && other.location == other.location;
    }
};

std::ostream& operator<<(std::ostream& out, Person p) {
    out << p.age << " " << p.location;
    return out;
}

int main() { 
    tuple tup {"../schema.txt"};
    Person bill = {5, "Kentucky"};
    Person empty{0,""};
    DB table{tup};
    Transaction* tx2 = table.startTx(TxType::READ);
    Transaction* tx1 = table.startTx(TxType::WRITE);

    table.insertOp(tx1, "Bill", bill.serialize());
    Person res = Person::deserialize(table.getOp(tx1, "Bill"));
    assert(bill == res);
    print("Assert passed");
    Person res2 = Person::deserialize(table.getOp(tx2, "Bill"));
    assert(empty == res2);
    print("Assert passed");
    table.commitTx(tx1);
    Person res3 = Person::deserialize(table.getOp(tx2, "Bill"));
    print(res3);
    assert(empty == res3);
    print("Assert passed");

    Transaction* tx3 = table.startTx(TxType::WRITE);
    Person res4 = Person::deserialize(table.getOp(tx3, "Bill"));
    assert(bill == res4);
    print("Assert passed");
    assert(table.updateOp(tx3, "Bill", "location", std::string{"virginia"}));
    assert(table.updateOp(tx3, "Bill", "age", 6));
    Person res5 = Person::deserialize(table.getOp(tx3, "Bill"));
    print(res5);
    assert(6 == res5.age);
    

}