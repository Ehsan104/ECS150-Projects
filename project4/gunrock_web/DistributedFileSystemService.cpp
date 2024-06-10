#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>

#include "DistributedFileSystemService.h"
#include "ClientError.h"
#include "ufs.h"
#include "WwwFormEncodedDict.h"

using namespace std;

//There are four types of errors that your distributed file system can return. 
//First, use ClientError::notFound() for any API calls that try to access a resouce that does not exist. 
//Second, use ClientError::insufficientStorage() for operations that modify the file system and need to allocate new blocks but the disk does not have enough storage to satisfy them. 
//Third, use ClientError::conflict() if an API call tries to create a directory in a location where a file already exists. 
//Fourth, use ClientError::badRequest() for all other errors.

//Error checking here:

void setNotFound(HTTPResponse *response) {
  response->setStatus(ClientError::notFound().status_code);
  response->setBody(ClientError::notFound().what());
}

void setInsufficientStorage(HTTPResponse *response) {
  response->setStatus(ClientError::insufficientStorage().status_code);
  response->setBody(ClientError::insufficientStorage().what());
}

void setConflict(HTTPResponse *response) {
  response->setStatus(ClientError::conflict().status_code);
  response->setBody(ClientError::conflict().what());
}

void setBadRequest(HTTPResponse *response) {
  response->setStatus(ClientError::badRequest().status_code);
  response->setBody(ClientError::badRequest().what());
}

//////////////END of the error checking 


DistributedFileSystemService::DistributedFileSystemService(string diskFile) : HttpService("/ds3/") {
  this->fileSystem = new LocalFileSystem(new Disk(diskFile, UFS_BLOCK_SIZE));
}  

void DistributedFileSystemService::get(HTTPRequest *request, HTTPResponse *response) {
    response->setBody("");
}

void DistributedFileSystemService::put(HTTPRequest *request, HTTPResponse *response) {
    response->setBody("");

}

void DistributedFileSystemService::del(HTTPRequest *request, HTTPResponse *response) {
    response->setBody("");
  
}
