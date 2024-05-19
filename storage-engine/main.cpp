#include <iostream>
#include "db.h"


struct Person {
    int age;
    std::string location;

    std::vector<char> serialize() {
        char buffer[4 + 64];
        memset(&buffer, 0, 68);
        memcpy((char*)(&buffer), &age, sizeof(age));
        std::cout << *(int*)(&buffer) << std::endl;
        memcpy((char*)(&buffer) + 4, location.c_str(), location.size());
        std::vector<char> vecbuffer (buffer, buffer + 68);
        return vecbuffer;
    }
    static Person deserialize(std::vector<char> vec) {
        Person p;
        int* ageptr = (int*)&vec[0];
        p.age = *ageptr;
        char* locationptr = (char*)&vec[4];
        p.location = *locationptr;
        return p;
    }
};
std::ostream& operator<<(std::ostream& out, Person p) {
    out << p.age << " " << p.location << std::endl;
    return out;
}
int main() { 
    tuple tup {"../schema.txt"};
    Person bill = {5, "Kentucky"};
    DB table{tup};
    Transaction* tx1 = table.startTx(TxType::WRITE);
    table.insertOp(tx1, "Bill", bill.serialize());
    std::cout << "Running1" << std::endl;
    std::vector<char> res = table.getOp(tx1, "Bill");
    if (res.size()) {
        std::cout << res.size()<<std::endl;
        Person getValue = bill.deserialize(res);
        std::cout << getValue.age <<std::endl;
    } else {
        std::cout << "not found" <<std::endl;
    }

}