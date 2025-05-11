#include <iostream>
#include <limits>
using namespace std;
bool alpha (char letter );
int main() {
    while (true) {
        char letter;
        cout << "Enter the character (only 1 character)enter 0 to exit :";
        cin >> letter;

        letter = toupper(letter) ;

        if (cin.peek() != '\n') {
            cout << "Enter 1 letter only !" << endl ;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }else if (alpha (letter)) {
            cout << "the letter it is alphabet" << endl;
        }else if (letter == '0'){
            break;
        }else{
            cout << "the letter it is not alphabet" << endl ;
        }
    }

    return 0;
}
bool alpha (char letter ){
    for (char  alpha = 'A' ; alpha <= 'Z'; alpha ++) {
        if (letter == alpha) {
            return true;
        }
    }
        return false ;
}


