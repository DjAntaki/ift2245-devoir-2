#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include "common.h"

//POSIX library for threads
#include <pthread.h>
#include <unistd.h>

class Client {
public:
    /**
     * Identifiant unique du client.
     */
    int id;

    /**
     * Thread identifier
     */
    pthread_t pt_tid;

    /**
     * Resources acquises par le client.
     * 
     * Ceci est de taille numResources.
     */
    int acquired[];

    static int port;
    static int numClients;
    static int numResources;
    static int numRequests;

    Client();
    ~Client();

    /**
     * Envoit une requête au serveur.
     * 
     * @param requestID
     * @param socketFD
     * @return la réponse du serveur
     */
    int send(int request_id, int sock);

    /**
     * Lance un thread client
     * 
     * @param param
     * @return 
     */
    static void *run(void * param);

    static void printAndSaveResults(const char* fileName);
    static int readConfigurationFile(const char *fileName);
    static void readMaxFromFile();

private:

    /**
     * Nombre maximale de resources que les clients peuvent acquérir.
     */
    static int **Max;

    // Results variables
    static int countAccepted; // Result counter for total acepted requests
    static int countOnWait; // Result counter for total request denied and put to wait
    static int countInvalid; // Result counter for total invalid requests
    static int countClientsDispatched; // Result counter for total clients correctly finished

    /**
     * Vérouille la connection et l'écriture sur une stream.
     */
    static pthread_mutex_t connect_lock;

    /**
     * Limite le nombre de threads qui peuvent ouvrir le socket.
     */
    static sem_t open_limit;

    static int count; // Common counter of created ClienThread to asign an ID
};

#endif // CLIENTTHREAD_H
