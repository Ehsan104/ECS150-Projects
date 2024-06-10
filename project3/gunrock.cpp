#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <fcntl.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <deque>

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HttpService.h"
#include "HttpUtils.h"
#include "FileService.h"
#include "MySocket.h"
#include "MyServerSocket.h"
#include "dthread.h"

using namespace std;

int PORT = 8080;
int THREAD_POOL_SIZE = 1;
int BUFFER_SIZE = 1;
string BASEDIR = "static";
string SCHEDALG = "FIFO";
string LOGFILE = "/dev/null";

vector<HttpService *> services;

HttpService *find_service(HTTPRequest *request) {
   // find a service that is registered for this path prefix
  for (unsigned int idx = 0; idx < services.size(); idx++) {
    if (request->getPath().find(services[idx]->pathPrefix()) == 0) {
      return services[idx];
    }
  }

  return NULL;
}


void invoke_service_method(HttpService *service, HTTPRequest *request, HTTPResponse *response) {
  stringstream payload;

  // invoke the service if we found one
  if (service == NULL) {
    // not found status
    response->setStatus(404);
  } else if (request->isHead()) {
    service->head(request, response);
  } else if (request->isGet()) {
    service->get(request, response); 
    
    //put in the rest of the requests after this get one which are, put, post, delete
  /*
  } else if (request->isPut()){ 
    service -> put(request, response);
  } else if (request->isPost()){
    service -> post(request, response);
  } else if (request->isDelete()){
    service -> del(request, response);
*/ 
  } else {
    // The server doesn't know about this method
    response->setStatus(501);
  }
}

void handle_request(MySocket *client) {
  HTTPRequest *request = new HTTPRequest(client, PORT);
  HTTPResponse *response = new HTTPResponse();
  stringstream payload;
  
  // read in the request
  bool readResult = false;
  try {
    payload << "client: " << (void *) client;
    sync_print("read_request_enter", payload.str());
    readResult = request->readRequest();
    sync_print("read_request_return", payload.str());
  } catch (...) {
    // swallow it
  }    
    
  if (!readResult) {
    // there was a problem reading in the request, bail
    delete response;
    delete request;
    sync_print("read_request_error", payload.str());
    return;
  }
  
  HttpService *service = find_service(request);
  invoke_service_method(service, request, response);

  // send data back to the client and clean up
  payload.str(""); payload.clear();
  payload << " RESPONSE " << response->getStatus() << " client: " << (void *) client;
  sync_print("write_response", payload.str());
  cout << payload.str() << endl;
  client->write(response->response());
    
  delete response;
  delete request;

  payload.str(""); payload.clear();
  payload << " client: " << (void *) client;
  sync_print("close_connection", payload.str());
  client->close();
  delete client;
}

//trying to make it a producer consumer implementation, like the coke machine
//only using pthread to initialize the variables 
//the producer creates the threads and the consumer takes those and uses the implemented handle_request function to handle the request
//Use: conditional variable reading and producer/consumer lecture 
deque<MySocket *> clients; //create a queue of clients 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; //initialize the mutex
pthread_cond_t buffer_available = PTHREAD_COND_INITIALIZER; //initialize the condition variable
pthread_cond_t client_available = PTHREAD_COND_INITIALIZER; //initialize the condition variable

void *consumer(void *arg) {
  MySocket *client; 

  while (true) {
    dthread_mutex_lock(&lock); //lock the mutex

    while (clients.empty()) { //if buffer is empty, wait for the client to be available
      dthread_cond_wait(&client_available, &lock);
    }

    client = clients.front(); //get the client from the front of the buffer
    clients.pop_front(); //this is to implement the FIFO 
    dthread_cond_signal(&buffer_available); //signal that the buffer is available
    dthread_mutex_unlock(&lock); //unlock the mutex
    handle_request(client); //handle the request 
  }
}

void *producer() {
  pthread_t thread[THREAD_POOL_SIZE]; //create the threads for the thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) { //create the threads for each of the new requests 
    dthread_create(&thread[i], NULL, consumer, NULL);
    dthread_detach(thread[i]); 
  }

  MyServerSocket *server = new MyServerSocket(PORT); //create the server socket
  MySocket *client; //create the client socket

  while (true) { //while there is a new request, keep accepting the new requests
    sync_print("waiting_to_accept", ""); 
    client = server->accept();
    sync_print("client_accepted", "");

    dthread_mutex_lock(&lock); 

    while ((int)clients.size() == BUFFER_SIZE) { //if the buffer is full, wait for the buffer to be available
      dthread_cond_wait(&buffer_available, &lock); 
    }

    clients.push_back(client); //push the client into the buffer
    dthread_cond_signal(&client_available); //signal that the client is available
    dthread_mutex_unlock(&lock); //unlock the mutex
  }
}

int main(int argc, char *argv[]) {

  signal(SIGPIPE, SIG_IGN);
  int option;

  while ((option = getopt(argc, argv, "d:p:t:b:s:l:")) != -1) {
    switch (option) {
    case 'd':
      BASEDIR = string(optarg);
      break;
    case 'p':
      PORT = atoi(optarg);
      break;
    case 't':
      THREAD_POOL_SIZE = atoi(optarg);
      break;
    case 'b':
      BUFFER_SIZE = atoi(optarg);
      break;
    case 's':
      SCHEDALG = string(optarg);
      break;
    case 'l':
      LOGFILE = string(optarg);
      break;
    default:
      cerr<< "usage: " << argv[0] << " [-p port] [-t threads] [-b buffers]" << endl;
      exit(1);
    }
  }

  set_log_file(LOGFILE);

  sync_print("init", "");
  //MyServerSocket *server = new MyServerSocket(PORT);
  //MySocket *client;

  // The order that you push services dictates the search order
  // for path prefix matching
  services.push_back(new FileService(BASEDIR));

  producer(); //call the producer function to start the whole process 

}
