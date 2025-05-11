#include <iostream>

using namespace std ;
void operations ();
string a ;
int main() {

    cout << "please Enter the number :" ;
    cin >> a ;
  operations();

    return 0;
}
void operations () {
     int i = 0 ;
     int sum = 0 ;
    while (i < a.length()){
        sum += a[i]-'0' ;
        i++;
    }
    cout <<"Result : "<< sum << endl ;
}

