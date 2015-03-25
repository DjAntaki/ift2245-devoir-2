#include "server.h"


///In this function initialize the Banker's Algorithm
///data structures as you see convinient

void ServerThreads::initializationOfDataStructures()
{
    /// TP2_TO_DO

    // If 'initialValuesProvided' is true in the configuraton file
    // the matrices Available and Max are already initialized to
    // the specified values. If not, you need to initialize them
    // here (use the bool variable 'initDataProvided' to cover both
    // scenarios.


    if (!initDataProvided)
    {
        // génération des données
        for (int i = 0; i < numResources; i++)
        {
            Available[i] = rand() % (2 * numClients);
            for (int j = 0; j < numClients; j++)
            {
                Max[j][i] = rand() % Available[i];
            }
        }
    }
    /// TP2_END_TO_DO

    ///DO NOT ERASE THIS PART
    // The initilized values will be written to a file for the
    // client to read them afterwards
    writeMaxToFile();
}

/**
 * Traite une requête d'un client.
 *
 * @param threadID ID du thread do serveur (devrait être propre à l'object)
 * @param socketFD descipteur de fichier pour communiquer avec le client
 * @return la réponse retournée au client
 */
void ServerThreads::processRequest(int threadID, int sockfd)
{
    // contient la requête du client
    int request[1 + numResources + 1];
    bzero(&request, sizeof (int) * (1 + numResources));

    //pthread_mutex_lock(&accept_lock);
    // lecture de la requête du client
    //
    int n = read(sockfd, request, (1 + numResources + 1) * sizeof (int));

    //pthread_mutex_unlock(&accept_lock);

    if (n < 0)
        error("ERROR reading from socket");

    int clientID = request[0];

    printf("server %u handling request from client %u", threadID, clientID);

    cout << "server " << threadID << ": client: " << clientID << ": ";

    for (int i = 1; i < 1 + numResources; i++)
        cout << " " << request[i];

    cout << " last request? " << request[numResources + 1] << endl;
    bool lastrequest = request[numResources + 1];

    //Traitement de la requete
    // C'est une requete qui demande des ressources
    // Algorithme du Banquier!
    //
    int answer[1] = {0}; ///On suppose la requête valide

    pthread_mutex_lock(&ServerThreads::available_lock);
    for (int i = 0; i < numResources; i++)
    {
        cout << "Allocation " << i << " : " << Allocation[clientID][i] << endl;
        if ((request[i + 1] <= Max[clientID][i]) && (-request[i + 1] <= Allocation[clientID][i]))
        {

            if (request[i + 1] > Available[i] || !BankersSimulation(request)) // 
            {
                cout << "on wait" << endl;
                //Not enough ressources, waiting time as answer. 1000 pour l'instant.
                answer[0] = 1000;
                countOnWait++;
                break;
            }

        }
        else
        {
            //Invalid request
            cout << "invalid" << endl;
            answer[0] = -1;
            countInvalid++;
            break;
        }
    }

    if (answer[0] == 0)
    {
        cout << "accepted" << endl;

        for (int i = 0; i < numResources; i++)
        {
            Available[i] -= request[i + 1];
            Allocation[clientID][i] += request[i + 1];
            Need[clientID][i] = 0;
        }
        countAccepted++;

    }
    else if (answer[0] > 0)
    {
        for (int i = 0; i < numResources; i++)
        {
            Need[clientID][i] = request[i + 1];
        }

    }

    // considère que la requête est processé
    if (answer[0] <= 0)
        requestProcessed++;

    if (lastrequest)
    {
        ClientsRunning[clientID] = false;
    }

    pthread_mutex_unlock(&available_lock);

    cout << "server " << threadID << ": written " << answer[0] << " to client " << clientID << endl;
    cout << "Total processed request " << requestProcessed << "/" << totalNumRequests << endl;

    // écrit un entier pour répondre au client
    if (write(sockfd, &answer, sizeof (int)) < 0)
        error("could not respond to client");

    // le client devrait avoir terminé à ce point et commencé à traiter une
    // nouvelle requête
}

//Return a boolean indicating if the system is safe if it proceed with that request.
//Disclaimer : J'ai lu la page wikipedia de l'algorithme du banquier pour le comprendre.
//Une fois compris, je ne lai pas relu pour coder l'algo. Toute ressemble avec la page wikipedia est a blamer sur ma mémoire.

bool ServerThreads::BankersSimulation(int *request)
{

    bool running[numClients]; //doit etre une copie de letat des clients au demaragge de la simulation

    for (int i = 0; i < numClients; i++)
        running[i] = ClientsRunning[i];

    int c = 0;
    int avl[numResources]; //faire une copie de Available

    for (int i = 0; i < numResources; i++)
        avl[i] = Available[i];

    int clientID = request[0];
    for (int x = 1; x < numResources + 1; x++)
    {
        avl[x] -= request[x];
    }

    while (c != 0)
    {
        bool safe = 0;
        for (int i; i < numClients; i++)
        {

            if (running[i])
            {
                bool release = true;

                for (int y; y < numResources; y++)
                {
                    if (Max[i][y] - Allocation[i][y] > avl[y])
                    {
                        release = false;
                    }
                }

                if (release)
                {
                    safe = 1;
                    c--;
                    running[i] = false;
                    for (int y; y < numResources; y++)
                    {
                        avl[y] += Allocation[i][y];
                    }

                }

            }

        }
        if (safe == 0)
        {
            return false;
        }
    }
    return true;
}

/// Do not modify this function
/// Rather use it as an example of socket functonality
/// to do the conections on the clients

/**
 * Code du thread du serveur.
 */
void* ServerThreads::threadCode(void * param)
{
    int ID = *((int*) param);

    struct sockaddr_in thread_addr;
    socklen_t threadSL = sizeof (thread_addr); //Thread socket lenght
    int start = time(NULL);

    // Now loop until the server has completely dispatched all clients
    do
    {
        // Loop until accept() returns the first valid conection
        pthread_mutex_lock(&ServerThreads::accept_lock);

        // accept a new connection
        int thread_fd;
        while ((thread_fd = accept(sock, (struct sockaddr *) &thread_addr, &threadSL)) < 0)
        {
            if ((time(NULL) - start) >= timeout)
            {
                cerr << "Time out on thread " << ID << endl;
                pthread_mutex_unlock(&ServerThreads::accept_lock);
                pthread_exit(NULL);
            }
        }

        pthread_mutex_unlock(&ServerThreads::accept_lock);

        cout << "thread " << ID << " with FD " << thread_fd << endl;

        // Process request from connection
        processRequest(ID, thread_fd);

        // one fd per connection, this one is done
        close(thread_fd);

    }
    while (requestProcessed < totalNumRequests);

    pthread_exit(NULL);
}

/// Do not modify this function
/// Rather use it as an example to uderstand socket functionality

void ServerThreads::createAndStart()
{
    // création du socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
        error("ERROR opening socket");

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
        error("ERROR on binding");

    listen(sock, backlog);

    //Create the thread variables
    realID = new int[numServerThreads];
    pt_tid = new pthread_t[numServerThreads];

    cout << "Now waiting for clients..." << endl;

    //Finally create each child thread and run the function threadCode
    for (int i = 0; i < numServerThreads; i++)
    {
        realID[i] = i; //Assign the real ID (from 0 to numServerThreads-1)
        pthread_create(&pt_tid[i], NULL, &threadCode, &realID[i]);
    }

    // wait until all server threads have finished
    for (int i = 0; i < numServerThreads; i++)
        pthread_join(pt_tid[i], NULL);
}

/// You can modify this function to print other values at
/// the end of the client application excecution. But do not
/// modify the results file output, as it is neccesary for evaluation

void ServerThreads::printAndSaveResults(const char* fileName)
{
    cout << endl << "------Server Results-------" << endl;
    cout << "Requests accepted:\t\t" << countAccepted << endl;
    cout << "Requests sent to wait:\t\t" << countOnWait << endl;
    cout << "Invalid requests:\t\t" << countInvalid << endl;
    cout << "Clients dispatched:\t\t" << countClientsDispatched << endl;
    cout << "Total requests proccesed:\t" << requestProcessed << endl;

    ///DO NOT MODIFY THIS PART
    // Save the counted values for evaluation
    if (fileExists(fileName)) remove(fileName);
    ofstream fs(fileName);
    fs << countAccepted << " " << countOnWait << " " << countInvalid << " " << countClientsDispatched << endl;
    fs.close();
}

/// You can modify this function if you want to add other fields
/// to the configuration field, but normally you will not need to

void ServerThreads::readConfigurationFile(const char *fileName)
{
    if (!fileExists(fileName))
    {
        cout << "No configuration file " << fileName << " found" << endl;
        exit(1);
    }

    libconfig::Config cfg;
    cfg.readFile(fileName);

    cfg.lookupValue("portNumber", port);
    cfg.lookupValue("maxWaitTime", timeout);
    cfg.lookupValue("serverBacklogSize", backlog);

    cfg.lookupValue("serverThreads", numServerThreads);
    cfg.lookupValue("numClients", numClients);
    cfg.lookupValue("numResources", numResources);
    cfg.lookupValue("numRequestsPerClient", numRequestsPerClient);
    totalNumRequests = numClients*numRequestsPerClient;

    //Dynamic allocation of memory (will be freed at destructor)
    Available = new int[numResources];
    //ClientsOnWait = new bool[numClients];
    Max = new int*[numClients];
    Allocation = new int*[numClients];
    Need = new int*[numClients];
    ClientsRunning = new bool[numClients];
    bzero(ClientsRunning, sizeof (bool) * numClients);
    for (int i = 0; i < numClients; i++)
    {
        Max[i] = new int[numResources];
        Allocation[i] = new int[numResources];
        bzero(Allocation[i], sizeof (int) * numResources);
        Need[i] = new int[numResources];
    }

    cfg.lookupValue("initialValuesProvided", initDataProvided);

    if (initDataProvided)
    {
        //Initialize the Available and Max structures
        libconfig::Setting& available = cfg.lookup("availableResources");
        libconfig::Setting& maximum = cfg.lookup("maximumPerClient");

        for (int i = 0; i < numResources; i++)
            Available[i] = available[i];

        for (int i = 0; i < numClients; i++)
        {
            for (int j = 0; j < numResources; j++)
            {
                Max[i][j] = maximum[i][j];
                //Verification
                if (Max[i][j] > Available[j])
                {
                    cerr << "Invalid maximumPerClient values" << endl;
                    exit(1);
                }
            }
        }
    }

    cout << "Server started with configuration: " << endl << endl;
    cout << "Maximum wait time: " << timeout << endl;
    cout << "Number of server threads: " << numServerThreads << endl;
    cout << "Server backlog size: " << backlog << endl;
    cout << "Number of clients: " << numClients << endl;
    cout << "Number of resources: " << numResources << endl;
    if (initDataProvided)
    {
        cout << "Available resources at start:" << endl;
        for (int i = 0; i < numResources; i++)
            cout << Available[i] << " ";
        cout << endl << endl;
    }
    else
        cout << endl << "No initial values provided, needs aditional initialization" << endl;
}


/// The rest of the code is neccesary for the correct functionality
/// You can add extra stuff, but try to keep the provided code as it is

void ServerThreads::writeMaxToFile()
{

    if (!fileExists("temp")) mkdir("temp", 0755);
    if (fileExists("temp/Max")) remove("temp/Max");
    ofstream fs("temp/Max");

    for (int i = 0; i < numClients; i++)
    {
        for (int j = 0; j < numResources; j++)
            fs << Max[i][j] << " ";
        fs << endl;
    }
    fs.close();
}

ServerThreads::ServerThreads()
{
    // General initialization
    realID = NULL;
    pt_tid = NULL;
    pt_attr = NULL;
}

ServerThreads::~ServerThreads()
{
    if (realID != NULL)
        delete []realID;
    if (pt_tid != NULL)
        delete []pt_tid;
    if (pt_attr != NULL)
        delete []pt_attr;

    /*
if (Available != NULL)
    delete []Available;
if (Max != NULL)
{
    for (int i = 0; i < numResources; i++)
    {
        delete []Max[i];
        delete []Allocation[i];
        delete []Need[i];
>>>>>>> a723906283783980b71058ba657403617fd505cd
    }
    delete []Max;
    delete []Allocation;
    delete []Need;
}
     */

    if (sock == -1)
        close(sock);
}

//Declaration of static variables
int ServerThreads::numClients;
int ServerThreads::numResources;
int ServerThreads::numRequestsPerClient;
//Initialization of static variables
int ServerThreads::requestProcessed = 0;
int ServerThreads::totalNumRequests = 0;
int ServerThreads::timeout = 0;
int ServerThreads::sock = -1;
bool* ServerThreads::ClientsRunning = NULL;

// Initialization of result variables
int ServerThreads::countAccepted = 0;
int ServerThreads::countOnWait = 0;
int ServerThreads::countInvalid = 0;
int ServerThreads::countClientsDispatched = 0;

int* ServerThreads::Available = NULL;
int** ServerThreads::Max = NULL;
int** ServerThreads::Allocation = NULL;
int** ServerThreads::Need = NULL;

pthread_mutex_t ServerThreads::accept_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ServerThreads::available_lock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
    ServerThreads server;

    //Read the parameters from the configuration file specified
    server.readConfigurationFile("initValues.cfg");

    //Fill the Banker's Algorithm data structures with
    //whatever you think convenient
    server.initializationOfDataStructures();

    //Start the server with the configuration loaded
    server.createAndStart();

    /// Do not erase or modify this part
    server.printAndSaveResults("temp/resultsServer");

    return 0;
}

