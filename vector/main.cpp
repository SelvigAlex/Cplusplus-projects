#include <iostream>
#include <string>
#include "Vector.hpp"

using namespace std;

int main() {
    Vector<std::string> array(5);
    for (size_t i = 0; i < array.size(); i++) {
        cin >> array[i];
    }

    for (size_t i = 0; i < array.size(); i++) {
        cout << array[i] << ' ';
    }
    cout << '\n';

    array.push_back("6");

    for (size_t i = 0; i < array.size(); i++) {
        cout << array[i] << ' ';
    }
    cout << '\n';

    array.pop_back();

    for (size_t i = 0; i < array.size(); i++) {
        cout << array[i] << ' ';
    }
    cout << '\n';

    return 0;
}