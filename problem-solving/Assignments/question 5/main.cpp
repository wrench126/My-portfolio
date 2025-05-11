#include <iostream>
using namespace  std ;
double calc ( double a , double b , int choose ) ;
int main() {
    while(true) {
        int choose = 0;
        double a = 0 , b = 0;

        cout << " 1) ADD \n" << " 2) sub \n" << " 3) multi \n " << "4) divide \n "<< "5) EXIT \n "<< "Choose from (1 : 5 ) :  ";
        cin >> choose;

        if (choose == 5) {
            break;
        }

        if (choose >= 1 && choose <= 5) {
            cout << "Enter num1 : ";
            cin >> a;
            cout << " Enter num2 : ";
            cin >> b;
            calc(a, b, choose);
         if (choose == 4 && b == 0)
             cout << "Error!!! Division by zero is not allowed (Please study mathematics :) ! )" << endl << endl ;;
             continue;
         }else{
            cout << "please choose from ( 1 : 5 ) only" << endl;
        }
    }

    return 0;
}
double calc ( double a , double b ,  int choose ) {
    if (choose == 1) {
        cout << " Result : " << a + b << endl << endl ;
    } else if (choose == 2) {
        cout << " Result : " << a - b << endl << endl ;
    } else if (choose == 3) {
        cout << " Result : " << a * b << endl << endl ;
    } else if (choose == 4) {
        cout << " Result : " << a / b << endl << endl ;
    }
    return 0 ;
}
