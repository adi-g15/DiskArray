#define DISKARRAY_DEBUG
#define DISKARRAY_KEEP_FILES
#define DISKARRAY_LOG_SAVES
// "Above defines not needed; Include order doesn;t matter"

#include <DiskArray/diskarray.hpp>
#include "bigobject.pb.h"

using namespace diskarray;

int main() {
    DiskArray<bigobject> arr;

    for (auto i = 0; i < 3; i++)
    {
        bigobject obj;
        obj.set_name("Aditya");
        obj.set_id(40);
        obj.add_data(std::rand());

        arr.push_back(obj);
    }

    bigobject fourth_object;

    fourth_object.set_name("Hidden King");
    fourth_object.set_id(15035);
    fourth_object.add_data(1);
    fourth_object.add_data(5);
    // fourth_object.add_data(4);   // works fine
    fourth_object.add_data(0);      // zeroes out all next elements too (Read comment at main.cpp:55)
    fourth_object.add_data(3);
    fourth_object.add_data(5);

    auto encoded = fourth_object.SerializeAsString();
    std::cout << "Encoded length is: " << encoded.size() << "\n";

    arr.push_back(fourth_object);

    for (auto i = 0; i < 999996; i++)
    {
        bigobject obj;
        obj.set_name("Aditya");
        obj.set_id(40);
        obj.add_data(std::rand());

        arr.push_back(obj);
    }

    using std::cout, std::endl;

    auto parsed_obj = arr.at( 3 );
    bigobject& reborn_fourth = parsed_obj;

    cout << reborn_fourth.name() << endl;
    cout << reborn_fourth.id() << endl;

    /*
     * A problem here:
     * It seems the data was restored, reborn_fourth.data() also shows 5 elements, but the elements after the 0 are all zero, ie. 3, 5
     * */
    for (const auto& i: reborn_fourth.data()) {
        cout << i << ", ";
    }
    cout << endl;
}
