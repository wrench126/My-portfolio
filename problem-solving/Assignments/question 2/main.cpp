#include <iostream>
using namespace std ;
// prototype
void numbers (int num);
int main() {
    while (true) {
        int num;
        cout << "Enter the number : ";
        cin >> num;
        numbers(num);

    }
    return 0;
}
// check the number is negative or positive or zero
void numbers (int num){
    if (num > 0){
        cout << "the number is positive" << endl ;

    }else if ( num < 0){
        cout << "the number is negative" << endl ;

    }else {
        cout << "the number is zero " << endl ;
    }
}
