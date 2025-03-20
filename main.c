# include <stdio.h>
# include <stdlib.h>
# include <stdbool.h>

//  this will be used to see if a number is already in the list
bool is_unique(int *uniqueList, int size, int num) {
    for (int i = 0; i < size; i++) {
        if (uniqueList[i] == num) {
            return false;
        }
    }
    return true;
}

// function to count unique elements in the multiplication table
int count_unique_elements(int N) {
    int *unique_list = (int *) malloc(N * N * sizeof(int)); // store unique elements
    int unique_count = 0; // count of unique elements

    // generate multiplication table
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            int product = i * j;
            if (is_unique(unique_list, unique_count, product)) {
                unique_list[unique_count++] = product;
            }
        }
    }

    free(unique_list);
    return unique_count;
}

int main(int arg_count, char *arg_values[]) {
    int N;
    printf("Enter the value of N: ");
    scanf("%d", &N);
    printf("The number of unique elements in the multiplication table of size %d is %d\n", N, count_unique_elements(N));
    return 0;
}