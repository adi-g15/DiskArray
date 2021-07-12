#define DISKARRAY_DEBUG
#define DISKARRAY_KEEP_FILES
#define DISKARRAY_LOG_SAVES
// "Above defines not needed; Include order doesn;t matter"

#include <DiskArray/diskarray.hpp>
#include "bigobject.pb.h"

using namespace diskarray;

int main() {
    DiskArray<bigobject> arr;

    for (auto i = 0; i < 1000000; i++)
    {
        bigobject obj;
        obj.set_name("Aditya");
        obj.set_id(40);
        obj.add_data(std::rand());

        arr.push_back(obj);
    }
}
