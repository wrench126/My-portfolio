#include <iostream>
using namespace std ;

void patterns ();
int choose = 0 ;

int main() {
     while (true){
         cout << "1) R-triangle  \n" << "2) Square \n" << "3) Triangle \n" << "4) Exit \n" << "Enter your choice :" << endl ;
         cin >> choose ;
         if (choose == 4){
             break;
         }
         if (choose >= 1 && choose <= 4){
             patterns() ;
         }
     }
    return 0;
}

void patterns (){
    if (choose == 1){
        for (int i = 0 ; i <= 4 ; i++){
            for (int j = 0 ; j <= i ; j++) {
                cout << "*";
            }cout << endl ;
        }
    }else if (choose == 2) {
        for (int i = 0; i < 7; i++) {
            for (int j = 0; j < 11; j++) {
                if (i == 0 || i == 6 || j == 0 || j == 10) {
                    cout << "*";
                } else {
                    cout << " ";
                }
            }cout << endl ;

            }
        }else if (choose == 3){
            for (int i = 0 ; i < 7 ; i++){
                for (int j = 0 ; j < 6-i ; j++){
                    cout << " " ;
                 }
                for (int k = 0 ; k < ( 2*i + 1 ) ; k++){
                    cout << "*" ;
                }
                 cout << endl;
             }

           }
        }





