#include <iostream>
#include <limits>
using namespace std;

// Prototype

bool check(char V[], char n);
int main() {

    char V[5] = {'A', 'O', 'I', 'U', 'E'};
    char n;

    while (true) {
        cout << "Enter the letter (only 1 letter) Enter 0 to exit : " ;
        cin >> n ;

        n = toupper(n); // converting the lowercase letter to uppercase latter
        // to exit the program
        if (n == '0') {
            break;
        }
        // check the user input (number of letters)
        else if ( cin.peek () != '\n'){ // check if the end is '\n' or is there another letter
            cout << "only 1 letter pleas !" << endl ;
            // to ignore the largest number of letter to '\n'
            cin.ignore(numeric_limits<streamsize>::max() , '\n') ;
            continue;
        }

        else if (check(V, n)) {
            cout << "The letter is Vowel" << endl;
        } else {
            cout << "The letter is Consonant" << endl;
        }
    }

    return 0;
}
// function check the letter is vowel or consonant by using for loop
bool check(char V[], char n) {
    for (int i = 0; i < 5; i++) {
        if (n == V[i]) {
            return true;
        }
    }
    return false;
}
