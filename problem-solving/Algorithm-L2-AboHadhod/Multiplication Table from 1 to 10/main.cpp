#include <iostream>
using namespace std;

void Dashboard (){

    cout <<"\n\t\t\t Multiplication Table from 1 to 10 \n\n";

    cout << "\t" ;

    for (int i = 1 ; i <= 10 ; i++) {
        cout << i << "\t" ;
    }
    cout << "\n___________________________________________________________________________________\n";

}
string colum (int i){

    if (i < 10)
        return "  |";
    else
        return " |" ;
}

void Multiplication () {

    Dashboard();

    for (int i = 1 ; i <= 10 ; i++){
        cout << " " << i << colum(i) << "\t";
        for (int j = 1 ; j <= 10 ; j++ ){
            cout << i * j << "\t" ;
        }
        cout << endl ;
    }
}

int main() {
    Multiplication();
    system("pause");

    return 0;
}
