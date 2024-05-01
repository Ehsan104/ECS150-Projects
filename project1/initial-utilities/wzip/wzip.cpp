#include <iostream> 

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>

#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {

    if (argc == 1){ //if there are no arguments
        cout << "wzip: file1 [file2 ...]" << endl;
        return 1;
    }

    int fd = 0; //same from wcat.cpp
    char buffer[4096];
    int BytesRead = 0;

    char prevChar; //needed to check if the character changes or not
    int run_length = 0;

    for (int i = 1; i < argc; i++){ //open each file, also from wcat.cpp
        fd = open(argv[i], O_RDONLY);
        if (fd == -1){
            cout << "wzip: cannot open file" << endl;
            return 1;
        }

//read from a file descriptor into a buffer
//then iterates over the buffer and for each character check if it's the same as the previous character
//if its the same is increment the run_length and if not just print it out 

        while ((BytesRead = read(fd, buffer, 4096)) > 0){ //read from file
            for (int j = 0; j < BytesRead; j++) {
                if (buffer[j] == prevChar) {
                    run_length++;
                } else { //if the character changes then enter this loop
                    if (run_length > 0) { //if there is a count, write it STDOUT and then the character
                        write(STDOUT_FILENO, &run_length, sizeof(run_length));
                        write(STDOUT_FILENO, &prevChar, sizeof(prevChar));
                    }
                    //had to put these two changes outside the loop test case 4, the program was running a number of files together
                    //the code interpreted it as separate files
                    prevChar = buffer[j]; //set new character
                    run_length = 1; //reset the run_length 
                }
            }
        }

        close(fd); //close the file 
    }

    if (run_length > 0) { //if there are any characters left, write their count and the character
        write(STDOUT_FILENO, &run_length, sizeof(run_length));
        write(STDOUT_FILENO, &prevChar, sizeof(prevChar));

    return 0; //return 0 if everything is successful
}
}