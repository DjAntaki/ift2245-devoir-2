#include "client.h"

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
void* Client::run(void * param)
{
    Client* client = (Client*) param;

    sem_wait(&Client::open_limit);

    cout << "client " << client->id << " acquired the open semaphore" << endl;

    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    sockaddr_in addr;

    if (sock < 0)
        error("couldn't open socket");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    // transmit clientID
    for (int request_id = 0; request_id < numRequests; request_id++)
    {
        /*
         * 1 int            client id
         * numResources int les resources
         */
        int request[1 + Client::numResources];

        // le premier entier correspond au ID du client
        request[0] = client->id;

        cout << "client " << client->id << ": request " << request_id << ": acquired resources";

        for (int i = 0; i < Client::numResources; i++)
            cout << " " << client->acquired[i];

        cout << endl;

        // on remplit la requête à partir de demandes et de libération aléatoires
        bool acquisition = rand() % 2;

        for (int i = 0; i < Client::numResources; i++)
        {
            cout << "resource " << i << " " << Client::Max[client->id][i] << " " << client->acquired[i] << endl;
            if (acquisition)
            {
                // allocation de ressources
                request[i + 1] = -(rand() % (Client::Max[client->id][i] - client->acquired[i]));
            }
            else
            {
                // libération de ressources
                request[i + 1] = client->acquired[i] > 0 ? (rand() % client->acquired[i]) : 0; // ...?
            }
        }

        cout << "client " << client->id << " request: " << request_id << " ";

        // impression de la requête
        for (int i = 1; i < 1 + Client::numResources; i++)
            cout << " " << request[i];

        cout << endl;

        int response = 0;

        // boucle pour écrire le message
        do
        {
            // connect on the socket
            while (connect(sock, (sockaddr*) & addr, sizeof (addr)) < 0)
                usleep(300); // attendre 300 µs avant la prochaine connexion

            cout << "client " << client->id << ": " << request_id << ": connexion établie avec le serveur" << endl;

            ssize_t written = write(sock, &request, (1 + Client::numResources) * sizeof (int));

            if (written < sizeof (int))
                error("couldn't write request to server");

            cout << "waiting on server..." << endl;

            // lit la réponse du serveur
            written = read(sock, &response, sizeof (int));

            cout << "read " << response << " from server" << endl;

            if (written < sizeof (int))
                error("couldn't read response from server");

            // vérouille les variables pour les résultats
            pthread_mutex_lock(&Client::results_lock);

            switch (response)
            {
            case ACCEPTED:
                // application de la transaction sur les données acquises
                for (int i = 1; i < 1 + Client::numResources; i++)
                {
                    client->acquired[i] += request[i];
                }
                countAccepted++;
                break;
            case INVALID:
                cout << "client " << client->id << ": request " << request_id << ": INVALID";
                countInvalid++;
                break;
            default:
                countOnWait++;
            }

            pthread_mutex_unlock(&Client::results_lock);

            // attente jusqu'à réponse du serveur
            if (response > 0)
                usleep(response * 1000);
        }
        while (response > 0);
    }

    // ferme le socket pour libérer une ressource
    close(sock);

    sem_post(&Client::open_limit);

    cout << "client " << client->id << ": finished processing" << endl;

    pthread_exit(NULL);
}

/// You can modify this function to print other values at
/// the end of the client application excecution. But do not
/// modify the result file output, as it is neccesary for evaluation

void Client::printAndSaveResults(const char* fileName)
{
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
int Client::readConfigurationFile(const char *fileName)
{
    if (!fileExists(fileName))
    {
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

void Client::readMaxFromFile()
{
    fstream fs;
    if (fileExists("temp/Max"))
    {
        fs.open("temp/Max", fstream::in);

        for (int i = 0; i < numClients; i++)
            for (int j = 0; j < numResources; j++)
            {
                fs >> Max[i][j];
            }

        fs.close();
    }
}

Client::Client()
{
    this->id = count++;
    bzero(this->acquired, sizeof (int) * numResources);
}

Client::~Client()
{
    if (Max != NULL)
    {
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
pthread_mutex_t Client::results_lock = PTHREAD_MUTEX_INITIALIZER;
sem_t Client::open_limit;

int **Client::Max = NULL;

int main(void)
{
    //Read the parameters from the configuration file specified
    int n = Client::readConfigurationFile("initValues.cfg");

    // initialisation de la mutex
    sem_init(&Client::open_limit, 0, 512);

    // nombre de threads instanciés
    Client client[n];

    for (int i = 0; i < n; i++)
    {
        pthread_create(&(client[i].pt_tid), NULL, &(Client::run), (void*) &(client[i]));
    }

    // on attend que tous les threads clients terminent
    for (int i = 0; i < n; i++)
        pthread_join(client[i].pt_tid, NULL);

    /// Do not erase or modify this part
    Client::printAndSaveResults("temp/resultsClient");

    return 0;
}
