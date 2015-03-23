#include "client.h"

#include "common.h"

/**
 * Envoi d'une requête du client sur le socket.
 * 
 * @param requestID
 * @param socket
 */
void Client::send(int requestID, int socket) {
    cout << "Client " << this->id << " is sending its " << requestID << " request" << endl;

    /*
     * 1 int            client id
     * numResources int les resources
     */
    int request[1 + Client::numResources];

    // le premier entier correspond au ID du client
    request[0] = this->id;

    // on remplit la requête à partir de demandes et de libération aléatoires
    for (int i = 0; i < Client::numResources; i++) {
        if (rand() % 2) {
            // allocation de ressources
            request[i + 1] = -(rand() % (Client::Max[this->id][i] - this->acquired[i]));
        } else {
            // libération de ressources
            request[i + 1] = rand() % this->acquired[i]; // ...?
        }
    }

    cout << "client " << this->id << " request: " << requestID << " ";

    // impression de la requête
    for (int i = 2; i < sizeof (request); i++)
        cout << " " << request[i];

    cout << endl;

    ssize_t written = write(socket, request, sizeof (request));

    if (written < sizeof (request)) {
        error("couldn't write bytes in the socket (written %d out of %d)");
    }

    // lecture de la réponse du serveur
    int response;

    do {
        read(socket, &response, sizeof (response));

        if (response > 0)
            countOnWait++;

        // attente jusqu'à réponse du serveur
        if (response > 0)
            usleep(response * 1000);

    } while (response > 0);

    // accepted or invalid
    if (response == ACCEPTED) {
        // application de la transaction sur les données acquises
        for (int i = 0; i < Client::numResources; i++) {
            this->acquired[i] += request[i + 1];
        }
        countAccepted++;
    }

    if (response == INVALID) {
        cout << "client " << this->id << ": request " << id << ": INVALID";
        countInvalid++;
    }
}

/**
 * Code du thread client.
 * 
 * Chaque client ouvre une connection sur le socket et transmet sa requête au 
 * serveur.
 * 
 * La communication se fait sur un socket TCP/IP à l'adresse "localhost".
 * 
 * @param param
 * @return 
 */
void *Client::run(void * param) {
    Client* client = (Client*) param;

    cout << "client " << client->id << " running!" << endl;

    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    sockaddr_in addr;

    if (sock < 0)
        error("couldn't open socket");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    // transmit clientID
    for (int request_id = 0; request_id < numRequests; request_id++) {
        //Start creating conextions to send the request

        // connect on the socket
        while (connect(sock, (sockaddr*) & addr, sizeof (addr)) < 0)
            usleep(300); // attendre 300 µs avant la prochaine connexion

        cout << "client " << client->id << ": connexion établie avec le serveur" << endl;

        /**
         * Envoi de la requête.
         * 
         * @param param
         * @return 
         */
        client->send(request_id, sock);
    }

    close(sock);

    pthread_exit(NULL);
}

/// You can modify this function to print other values at
/// the end of the client application excecution. But do not
/// modify the result file output, as it is neccesary for evaluation

void Client::printAndSaveResults(const char* fileName) {
    cout << endl << "------Client Results-----" << endl;
    cout << "Requests accepted:\t\t" << countAccepted << endl;
    cout << "Requests sent to wait:\t\t" << countOnWait << endl;
    cout << "Invalid requests:\t\t" << countInvalid << endl;
    cout << "Clients dispatched:\t\t" << countClientsDispatched << endl;

    ///DO NOT MODIFY THIS PART
    // Save the counted values for evaluation
    if (fileExists(fileName)) remove(fileName);
    ofstream fs(fileName);
    fs << countAccepted << " " << countOnWait << " " << countInvalid << " " << countClientsDispatched << endl;
    fs.close();
}

/**
 * You can modify this function if you want to add other fields
 * to the configuration fiel, but normally you will not need to do it
 * @param fileName
 * @return
 */
int Client::readConfigurationFile(const char *fileName) {
    if (!fileExists(fileName)) {
        cout << "No configuration file " << fileName << " found" << endl;
        exit(1);
    }

    libconfig::Config cfg;
    cfg.readFile(fileName);
    cfg.lookupValue("portNumber", port);
    cfg.lookupValue("numClients", numClients);
    cfg.lookupValue("numResources", numResources);
    cfg.lookupValue("numRequestsPerClient", numRequests);

    Max = new int*[numClients];
    for (int i = 0; i < numClients; i++)
        Max[i] = new int[numResources];

    readMaxFromFile();

    return numClients;
}

/// The rest of the code is neccesary for the correct functionality
/// You can add extra stuff, but try to keep the provided code as it is

void Client::readMaxFromFile() {
    fstream fs;
    if (fileExists("temp/Max")) {
        fs.open("temp/Max", fstream::in);

        for (int i = 0; i < numClients; i++)
            for (int j = 0; j < numResources; j++) {
                fs >> Max[i][j];
            }

        fs.close();
    }
}

Client::Client() {
    this->id = count++;
}

Client::~Client() {
    if (Max != NULL) {
        for (int i = 0; i < numResources; i++)
            delete []Max[i];
        delete []Max;
        Max = NULL;
    }
}

int Client::count = 0;

//Initialization of static variables
int Client::port = 0;
int Client::numClients = 0;
int Client::numResources = 0;
int Client::numRequests = 0;

// Initialization of result variables
int Client::countAccepted = 0;
int Client::countOnWait = 0;
int Client::countInvalid = 0;
int Client::countClientsDispatched = 0;

int **Client::Max = NULL;

int main(int argc, char *argv[]) {
    //Read the parameters from the configuration file specified
    int n = Client::readConfigurationFile("initValues.cfg");

    // nombre de threads instanciés
    Client client[n];

    for (int i = 0; i < n; i++) {
        client[i].id = i;
        pthread_create(&(client[i].pt_tid), NULL, &(Client::run), (void*) &client);
    }

    // on attend que tous les threads clients terminent
    for (int i = 0; i < n; i++)
        pthread_join(client[i].pt_tid, NULL);

    /// Do not erase or modify this part
    Client::printAndSaveResults("temp/resultsClient");

    return 0;
}
