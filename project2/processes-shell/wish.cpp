#include <iostream>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <sstream>
using namespace std;

// Global Path Variable having a local one was messing up my code for some reason this seems to work better
vector<string> path = {"/bin"};



//what we need to do:
    //the shell can either be in interactive mode or batch mode
        //interactive mode: the shell prints a prompt and waits for the user to enter a command
        //batch mode: the shell reads the commands from a file and executes them, shouldnt read from STDIN anymore

    //should be "wish> " space after the greater than sign parses the input executes the command, waits for command to finish
    
    //should repeat till the user enters exit
    
    //creates a process for each new command, exception of:
    
    //parse a command and run the program corresponding to the command: "ls -la /tmp" ->
       //shell should run the program /bin/ls with the arguments -la and /tmp
    
    //SHELL RUNS IN A WHILE LOOP REPEATEDLY ASKING FOR INPUT TO TELL IT WHAT COMMAND TO EXECUTE
       //if the user types built-in command "exit" the shell should exit

    //use getline() for readins lines of input
       //allows to obtain arbitarily long input lines

    //to parse the input lines into constituent pieces use stl::string class

    //to execute commands look into fork(), exec(), and wait()/waitpid()
        //use execv() dont use system() library function to run command
    
    //PATH variable, describe the et of directories to search for executables

    //access() to check if a file exists in a directory and is executable
        //finds executable in one of directories specified by PATH, creates nre process to run them
        //initial shell path should contain one directory: /bin
        //no need to worry about absolute and relative paths

    //BUILT IN COMMANDS: exit, cd, path
        //exit: exits the shell, user types 0, call exit system call with 0 as a prameter, error to pass arguments to exit.
        //cd: changes the current directory of the shell, takes one argument (0 or > 1 args should be signaled as an error), to change directory use chdir() system call, if chdir() fails print an error message.
        //path: changes the PATH variable of the shell, takes 0 or more arguments, each needs to be separated by a whitespace


//lets make this a function so we can just call it whenever we need to print an error message
void print_error_message(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

//function to execute the commands this includes the built ins like exit, cd, and path
void execute_command(const vector<string>& words, bool isParallel = false) {
    
    const string& cmd = words[0];
    
    if (cmd == "exit"){ //this is for the exit command, if the user types exit, the shell
        if (words.size() > 1) { //should only be one argument
            print_error_message();
        } else {
            exit(0);
        }
    }
    
    else if (cmd == "cd") { //this is for the cd command, if the user types cd, the shell should change the current directory
        
        if (words.size() != 2) { //should only have 2 arguments, the command and the directory, otherwise it is an error
            print_error_message();
        }
        
        else {
            if (chdir(words[1].c_str()) != 0) {  //if chdir fails, print an error message
                print_error_message();
                
            }
        }
    }
    
    else if (cmd == "path") { //this is for the path command, if the user types path, the shell should change the PATH variable
        path.clear();
        for (size_t i = 1; i < words.size(); i++){
            path.push_back(words[i]);
        }
    }
    
    else { //this is for the rest of the commands, the shell should run the program corresponding to the command
        
        int redirectIndex = -1; //if its negative 1 then that means there is no redirection, positive means redirection
        vector<string> commandArgs;
        string outputFile;
        string tempOutputFile;

        for (size_t i = 0; i < words.size(); i++) {    //find where the redirection is if there is any
            if (words[i] == ">") { //thats the redirection symbol
                if (i + 1 == words.size()) {   //checks if there's an output file after redirection
                    print_error_message(); return;
                }
                else {
                    redirectIndex = i; //tells use where redirection is
                    break;
                }
        }}
        
        if (redirectIndex != -1) {  //writes to output file if there's redirection
            commandArgs.assign(words.begin(), words.begin() + redirectIndex);
            outputFile = tempOutputFile = words[redirectIndex + 1];
            
            if (isParallel) {   //if we're doing parallelism, separate the temporary output file
                tempOutputFile += to_string(getpid());
            }
            if (words.size() > static_cast<size_t>(redirectIndex + 2)) { //if there isnt exactly 1 argument after the redirect symbol, should be an error
                print_error_message();
                return;
            }
        }
        else {
            commandArgs = words;
        }
    
        
        pid_t pid = fork();
        
        if (pid == 0) { //This is the Child process
            if (redirectIndex != -1) {
                int fd = open(tempOutputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); //write only, create
                if (fd == -1) {
                    print_error_message();
                    exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    print_error_message();
                    exit(1);
                }
                close(fd);
            }
            
            vector<char*> args;
            for (const string& arg : commandArgs) { //convert to char* for the execv function
                args.push_back(const_cast<char*>(arg.c_str()));
            }
            args.push_back(nullptr);
            for (const string& p : path) {
                string fullPath = p + "/" + commandArgs[0]; //this is the whole difference between absolute and relative path issue, messed up a lot of test cases here
                execv(fullPath.c_str(), args.data());
            }
            
            print_error_message(); exit(1); //only executs if execv returns (failed execution)
            
        }
        
        else if (pid > 0) { //parent process
            int status;
            waitpid(pid, &status, 0); // wait(NULL) seems to fail test case 11, so I tried waitpid instead
            if (isParallel && WIFEXITED(status)) { //if its a parallel command then we want the output of the first command to go to output file
                ifstream temp(tempOutputFile); // opening an input file stream for temp output file
                ofstream out(outputFile, ios::app | ios::out); // opening an output file stream for output file
                out << temp.rdbuf(); // reading data from the temp file to the original output file
                temp.close(); // closing the file
                remove(tempOutputFile.c_str());
            }
        }
        
        else {
            print_error_message();
            exit(1);
        }
        return;
        
    }
}

//this function takes the user input and parses it to comething executable
void process_command(const string &input) {
    
    vector<string> commands;
    string command = "";
    
    for (size_t i = 0; i < input.size(); i++) { //we're checking for multiple commands in one line
        if (input[i] == '&') { //separate multiple commands
            commands.push_back(command);
            command.clear();
            if (input[i + 1] == ' ') i++;
        }
        else {
            command += input[i];
        }
    }
    commands.push_back(command);
    
    for (size_t i = 0; i < commands.size(); i++) { //using a for loop so we can do each command independetly
        
        string& cmd = commands[i];
        vector<string> words;
        string word;
        
        for (size_t i = 0; i < cmd.size(); i++) { //check if there even is any redirection by looping through each character in command
            if (cmd[i] == '>') { //if there is redirection
                if (!word.empty()) { //we dont wanna put any empty words in the words vector
                    words.push_back(word);
                    word.clear();
                }
                if (words.empty()) { //if the vector is empty then print an error, meaning no command before redirection symbol
                    print_error_message();
                    break;
                }
                words.push_back(string(1, cmd[i]));
            }
            else if (cmd[i] == ' ') { //space means end of the argument, we can add that to the words vector
                if (!word.empty()) {
                    words.push_back(word); word.clear();
                }
            }
            else { word += cmd[i]; }
        }
        
        if (!word.empty()) words.push_back(word); //sort of an edge-case like if there is an overflow word then add that to the vector too
        
        if (words.empty()) return; //this means we have an empty command
        
        execute_command(words, !i); //call execute to this
    }
}

//this is the main function to put everything together
int main(int argc, char* argv[]) {
    
    if (argc == 1) { // Interactive mode
        string input;
        while (true) {
            cout << "wish> ";
            if (!getline(cin, input)) break; // If there's an input failure, exit to the loop
            if (input.empty()) continue;  // If the command is empty, prompt again
            process_command(input); // Process and execute teh command
        }
    }
    
    else if (argc == 2) {   // Batch mode
        ifstream BatchFile(argv[1]); // Open the batch file
        if (!BatchFile) { // If the file cannot be opend, print an error message and exit
            print_error_message();
            exit(1);
        }
        string input;
        while (getline(BatchFile, input)) { // Read the file line by line
            if (input.empty()) continue; // Skip empty lines
            process_command(input); // Parse the command
        }
        BatchFile.close(); // Properly close the file
    }
    
    else {
        print_error_message(); // More than one argument not allowed, print an error message
        exit(1);
    }
    
    return 0; // Proper exit after finishing all commands
}
