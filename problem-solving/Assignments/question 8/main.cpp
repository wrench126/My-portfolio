#include <iostream>
#include <cmath>
using namespace std;

int num = 0;

void prime();

int main() {
    cout << "Enter the number: ";
    cin >> num;
    prime();
    return 0;
}

void prime() {
    for (int i = 2; i <= num; i++) {
        bool isPrime = true;


        for (int j = 2; j <= sqrt(i); j++) {
            if (i % j == 0) {
                isPrime = false;
                break;
            }
        }


        if (isPrime) {
            cout << i << " " ;
        }
    }
}

