#include <iostream> 

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>

#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {

    if (argc == 1){ //if there are no arguments
        return 0;
    }

    //from lecture slides
    int fd = 0; //file descriptor, im not exactly sure if this should be the STDIN_FILENO or not it doesnt seem to make much of a difference
    char buffer[4096]; 
    int BytesRead = 0;

    for (int i = 1; i < argc; i++){ //opening the files
        fd = open(argv[i], O_RDONLY);
        if (fd == -1){
            cout << "wcat: cannot open file" << endl;
            return 1;
        }

        while ((BytesRead = read(fd, buffer, 4096))){ //reading and writing the files
            write(STDOUT_FILENO, buffer, BytesRead); 
        }

        close(fd); //closing the file 
    }

    return 0; //returning 0 if everything is successful
}