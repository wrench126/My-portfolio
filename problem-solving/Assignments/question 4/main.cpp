#include <iostream>
using namespace std ;
int fact (int num , int sum ) ;

int num = 0 ;
int sum = 1 ;

int main() {
    while (true) {
        cout << "Enter the number (Positive number only ) Enter 99 to exit !! :";
        cin >> num;
        if (num == 99){
            break;
        }else if ( num == 0){
            cout << num << "! = " <<  1 << endl ;
        }else if (num < 0) {
            cout << "positive number only !!" << endl ;
        } else {
            cout <<  num << "! = " << fact(num, sum) << endl;
        }
    }
    return 0;
}
int fact (int num , int sum){
    for(int i = 1 ; i <= num ; i++){
        sum = sum * i ;
    }
    return sum ;
}
