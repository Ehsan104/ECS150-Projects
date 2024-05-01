#include <iostream> 

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>

#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {

    if (argc < 2) { //from wcat.cpp 
        cout << "wgrep: searchterm [file ...]" << endl;
        return 1;
    }

    const char* search_term = argv[1]; //the first argument is the search term
    int fd = 0; //same from wcat.cpp
    char buffer[4096];
    int bytesRead;

    if (argc == 2){ //here it was needed for test case 4
        fd = STDIN_FILENO;
    } 

    for (int i = 2; i < argc; i++) { //open the files, also from wcat.cpp
        if (argc != 2) {
            fd = open(argv[i], O_RDONLY);
            if (fd == -1) {
                cout << "wgrep: cannot open file" << endl;
            return 1;
            }
        }
    }

    string line = ""; //create a string to store the line
    //here we are searching line by line 
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int j = 0; j < bytesRead; j++) {
            if (buffer[j] == '\n') { //means that the end of the line has been reached and it it has then we do the searching 
                if (line.find(search_term) != string::npos) {
                    cout << line << endl; //print the line if the search term is found
                }
                line = ""; //reset the line
            } else {
                line += buffer[j]; //keep adding onto the line if there isnt a '\n
            }
        }
    }

    close(fd);

    return 0;
}
