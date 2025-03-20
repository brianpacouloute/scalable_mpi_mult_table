# include <stdio.h>
# include <stdlib.h>
# include <stdbool.h>

//  this will be used to see if a number is already in the list
bool isUnique(int *uniqueList, int size, int num) {
    for (int i = 0; i < size; i++) {
        if (uniqueList[i] == num) {
            return false;
        }
    }
    return true;
}

int main() {
    printf("Hello, World!\n");
    return 0;
}