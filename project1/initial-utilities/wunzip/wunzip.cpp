#include <iostream> 

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>

#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {

    if (argc == 1){ //similar to wcat, this time we have different conditions
        cout << "wunzip: file1 [file2 ...]" << endl;
        return 1;
    }

    for (int i = 1; i < argc; i++){  //copied over from the wcat.cpp file because we need to open and close files for this too
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1){
            cerr << "wunzip: cannot open file" << endl;
            return 1;
        }

        char buffer[5]; //buffer is 5 because we are reading 4 bytes for the run length and 1 byte for the character
        int run_length; 
        char ch;
        
        while (read(fd, buffer, 5) == 5) { // Read 5 bytes at a time
            // Extract run length and character from buffer
            run_length = *(int *)buffer; //used pointers here 
            ch = buffer[4];

            for (int j = 0; j < run_length; j++){ //iterate through the run length and print the character
                write(STDOUT_FILENO, &ch, 1); 
            }
        }


        close(fd); //similar to wcat, close the file descriptor after we are done
    }
    return 0; //return 0 if everything is successful
}