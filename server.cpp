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

        // On initialise a quelle valeur pour Available???


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
 * @param threadID
 * @param socketFD
 */
void ServerThreads::processRequest(int threadID, int sock)
{
    // contient la requête du client
    int request[1 + numResources];

    // lecture de la requête du client
    int n = read(sock, request, (1 + numResources) * sizeof (int));

    if (n < 0)
        error("ERROR reading from socket");

    int clientID = request[0];

    printf("thread %u handling request from client %u", threadID, clientID);

    for (int i = 1; i < 1 + numResources; i++)
        cout << " " << request[i];

    cout << endl;

    //Traitement de la requete
    // C'est une requete qui demande des ressources
    // Algorithme du Banquier!
    int answerToClient = 0; ///On suppose la requête valide
    for (int i = 0; i < numResources; i++)
    {
        if (request[i + 1] <= Max[clientID][i])
        {
            if (request[i + 1] > Available[i])
            {
                //Not enough ressources, waiting time as answer. 1000 pour l'instant.
                answerToClient = 1000;
                countOnWait++;
                break;
            }

        }
        else
        {
            //Invalid request
            answerToClient = -1;
            countInvalid++;
            break;
        }
    }
    if (answerToClient == 0)
    {
        for (int i = 0; i < numResources; i++)
        {
            Available[i] -= request[i + 1];
            Allocation[clientID][i] += request[i + 1];
        }
        countAccepted++;

    }
    else if (answerToClient > 0)
    {

        for (int i = 0; i < numResources; i++)
        {
            Need[clientID][i] = request[i + 1];
        }

    }

    if (write(sock, &answerToClient, sizeof (int)) < 0)
        error("ERROR writing to socket");

    // considère que la requête est processé
    if (answerToClient == 0)
        requestProcesed++;
}

/// Do not modify this function
/// Rather use it as an example of socket functonality
/// to do the conections on the clients

void* ServerThreads::threadCode(void * param)
{
    int ID = *((int*) param);

    struct sockaddr_in thread_addr;
    socklen_t threadSL = sizeof (thread_addr); //Thread socket lenght
    int threadSocketFD = -1;
    int start = time(NULL);

    // Loop until accept() returns the first valid conection
    pthread_mutex_lock(&ServerThreads::accept_lock);

    while (threadSocketFD < 0)
    {
        threadSocketFD = accept(sock,
                                (struct sockaddr *) &thread_addr,
                                &threadSL);

        if ((time(NULL) - start) >= maxWaitTime ||
                requestProcesed == totalNumRequests)
            break;
    }

    // Now loop until the server has completely dispatched all clients
    while (requestProcesed < totalNumRequests)
    {

        if ((time(NULL) - start) >= maxWaitTime)
        {
            cerr << "Time out on thread " << ID << endl;
            pthread_exit(NULL);
        }

        if (threadSocketFD > 0)
        {
            // Process request from connection
            processRequest(ID, threadSocketFD);
            close(threadSocketFD);
        }

        //Take next conextion available
        threadSocketFD = accept(sock,
                                (struct sockaddr *) &thread_addr,
                                &threadSL);
    }
}

/// Do not modify this function
/// Rather use it as an example to uderstand socket functionality

void ServerThreads::createAndStart()
{
    // création du socket
    sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (sock < 0)
        error("ERROR opening socket");

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portNumber);
    if (bind(sock, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
        error("ERROR on binding");

    listen(sock, serverBacklogSize);

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
    cout << "Total requests proccesed:\t" << requestProcesed << endl;

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

    cfg.lookupValue("portNumber", portNumber);
    cfg.lookupValue("maxWaitTime", maxWaitTime);
    cfg.lookupValue("serverThreads", numServerThreads);
    cfg.lookupValue("serverBacklogSize", serverBacklogSize);
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
    for (int i = 0; i < numClients; i++)
    {
        Max[i] = new int[numResources];
        Allocation[i] = new int[numResources];
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
    cout << "Maximum wait time: " << maxWaitTime << endl;
    cout << "Number of server threads: " << numServerThreads << endl;
    cout << "Server backlog size: " << serverBacklogSize << endl;
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
    pthread_mutex_init(&ServerThreads::accept_lock, NULL);
}

ServerThreads::~ServerThreads()
{
    if (realID != NULL)
        delete []realID;
    if (pt_tid != NULL)
        delete []pt_tid;
    if (pt_attr != NULL)
        delete []pt_attr;

    if (Available != NULL)
        delete []Available;
    if (Max != NULL)
    {
        for (int i = 0; i < numResources; i++)
        {
            delete []Max[i];
            delete []Allocation[i];
            delete []Need[i];
        }
        delete []Max;
        delete []Allocation;
        delete []Need;
    }

    if (sock == -1)
        close(sock);
}

//Declaration of static variables
int ServerThreads::numClients;
int ServerThreads::numResources;
int ServerThreads::numRequestsPerClient;
//Initialization of static variables
int ServerThreads::requestProcesed = 0;
int ServerThreads::totalNumRequests = 0;
int ServerThreads::maxWaitTime = 0;
int ServerThreads::sock = -1;

// Initialization of result variables
int ServerThreads::countAccepted = 0;
int ServerThreads::countOnWait = 0;
int ServerThreads::countInvalid = 0;
int ServerThreads::countClientsDispatched = 0;

int* ServerThreads::Available = NULL;
int** ServerThreads::Max = NULL;
int** ServerThreads::Allocation = NULL;
int** ServerThreads::Need = NULL;

pthread_mutex_t ServerThreads::accept_lock;

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

