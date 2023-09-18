#include "headers.h"

//-----------------------------------------------------------------------------------------

sem_t *mutex;
pthread_mutex_t lockAddFlight = PTHREAD_MUTEX_INITIALIZER;
SlotSHMChanged *tshm;
pthread_cond_t condVerify = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lockVerify = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockTest = PTHREAD_MUTEX_INITIALIZER;

bool SMHChangedFlight;

void handlerTerminate(int signum)
{
    pid_t  p;
    int status;
    
    while ((p = wait(&status)) > 0) {
        //If the child process for which status was returned by the wait or waitpid function exited normally, the WIFEXITED macro evaluates to TRUE and the WEXITSTATUS macro is used to query the exit code of the child process. Otherwise, the WIFEXITED macro evaluates to FALSE.
        if (WIFEXITED(status))
        {
            kill(controlTower_pid, SIGUSR1);
            
            printf("\n");
            //printf("Child %ld exit status was %d.\n", (long)p, WEXITSTATUS(status));
            writeLogReport(currentTime(), "CLOSING PROGRAM");
            printf("\n");
            
            //Apaga MQ
            msgctl(mqID,IPC_RMID,NULL);

            //Tira a shm
            shmdt(slotsP); 
            shmdt(statsP);
            shmdt(slotT);
            shmdt(slotCV);
            shmdt(slotSHMChanged);

            //Faz unmap da shared memory
            munmap(&shmidSlots, sizeof(Slot)); 
            munmap(&shmidStats, sizeof(Stats));
            munmap(&shmidSlotTime, sizeof(SlotTime)); 
            munmap(&shmidCondVar, sizeof(SlotCondVar));
            munmap(&shmidSMHChanged, sizeof(SlotSHMChanged));

            kill(controlTower_pid, SIGKILL);
        }
        
        fflush(stdout);
    }

    exit(signum);
}

void handlerStats(int signum)
{
    Stats *ts = (Stats*)statsP;
    
    doAverages();
    
    printf("Total flights: %d", ts[0].nFlights);
    printf("Arrive flights: %d", ts[0].nFlights_Arrived);
    printf("Departure flights: %d", ts[0].nFlights_Departured);
    printf("Average number of arrivals time %d", ts[0].tAverageWait_Arrival);
    printf("Average number of departures time %d", ts[0].tAverageWait_Departure);
    printf("Average number of arrival holdings %d", ts[0].nAverageHoldigns_Arrival);
    printf("Average number of holdings in urgency state: %d", ts[0].nAverageHoldigns_UrgencyState);
    printf("Divert flights: i%d", ts[0].nFlights_Diverted);
    printf("Rejected flights: %d", ts[0].nFlights_Rejected);
}

void doAverages()
{
    Stats *ts = (Stats*)statsP;

    ts[0].tAverageWait_Arrival = (ts[0].tAverageWait_Arrival)/(ts[0].nFlights_Arrived);
    ts[0].tAverageWait_Departure = (ts[0].tAverageWait_Departure)/(ts[0].nFlights_Departured);
    ts[0].nAverageHoldigns_Arrival = (ts[0].nAverageHoldigns_Arrival)/(ts[0].nAverageHoldigns_Arrival);
    ts[0].nAverageHoldigns_UrgencyState = (ts[0].nAverageHoldigns_UrgencyState)/(ts[0].tAverageWait_Arrival);
}

int checkPriority(MQRequest request)
{
    if(request.fuel <= (4 + request.eta + config.duractionArrival))
    {
        return true;
    }

    return false;
}

void addMQRequest(MQRequest request) 
{   
    bool isPriority;
    char aux[200];

    if(request.msgtype == ARRIVAL)
    {
        isPriority = checkPriority(request);
        if(isPriority)
        {
            request.msgtype = PRIORITY;
            sprintf(aux, "%s EMERGENCY LANDING REQUEST\n", request.flightCode);
            writeLogReport(currentTime(), aux);
        }
    }

    if(msgsnd(mqID, &request, sizeof(MQRequest)-sizeof(long), 0) < 0)
    {
        printf("error sending\n");
    }
    else
    {   
        printf("Message Sent\n"); 
    }
}

void readMQRequest(long msgtype)
{
    MQRequest request;
    
    while(1)
    {
        if(msgrcv(mqID, &request, sizeof(MQRequest)-sizeof(long), msgtype, 0) < 0)
        {
            printf("error receiving1\n");
        }
        else
        {
            printf("Message Received: eta %d\n", request.takeoff);
        }
    }
    
}

int verifyFlightID(char *s)
{
    if (s[0] == 'T' && s[1] == 'P')
    {   
        return 1;
    }
    else
    {
        return 0;
    }
}

int verifyCommand(char *s)
{   
    char command[200];
    strcpy(command, s);
    char *token;
    token = strtok(command, " ");
    int init, takeoff, eta, fuel;
    
    if (strcmp("DEPARTURE", token) == 0)
    {   
        token = strtok(NULL, " ");
        if (!verifyFlightID(token))
        {   
            return 0;
        }
        
        token = strtok(NULL, " ");
        if (strcmp("init:", token) != 0)
        {
            return 0;
        }
        
        token = strtok(NULL, " ");
        init = atoi(token);
        if (init < 0)
        {
            return 0;
        }

        if(currentTime() > init)
        {
            return 0;
        }

        token = strtok(NULL, " ");
        if (strcmp("takeoff:", token) != 0)
        {
            return 0;
        }

        token = strtok(NULL, " ");
        takeoff = atoi(token);
        if (takeoff < init)
        {
            return 0;
        }
        
        token = strtok(NULL, " ");
        if (token != NULL)
        {
            return 0;
        }
    }

    else if (strcmp("ARRIVAL", token) == 0)
    {   
        token = strtok(NULL, " ");
        if (!verifyFlightID(token))
        {   
            return 0;
        }

        token = strtok(NULL, " ");
        if (strcmp("init:", token) != 0)
        {   
            return 0;
        }

        token = strtok(NULL, " ");
        init = atoi(token);
        if (init < 0)
        {
            return 0;
        }

        if(currentTime() > init)
        {
            return 0;
        }

        token = strtok(NULL, " ");
        if (strcmp("eta:", token) != 0)
        {   
            return 0;
        }

        token = strtok(NULL, " ");
        eta = atoi(token);
        if (eta <= 0)
        {   
            return 0;
        }

        token = strtok(NULL, " ");
        if (strcmp("fuel:", token) != 0)
        {   
            return 0;
        }

        token = strtok(NULL, " ");
        fuel = atoi(token);
        if (fuel-eta < 0 || fuel <= 0)
        {   
            return 0;
        }

        token = strtok(NULL, " ");
        if (token != NULL)
        {   
            return 0;
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

Flight flightInfo(char *s)
{   
    Flight flight;

    char command[200];
    strcpy(command, s);

    char *token;
    token = strtok(command, " ");
    
    if (strcmp("DEPARTURE", token) == 0)
    {   
        flight.f = 0; 

        token = strtok(NULL, " ");
        strcpy(flight.flightCode, token);
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");
        flight.dFlight.init = atoi(token);
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");
        flight.dFlight.takeoff = atoi(token);
    }
    if (strcmp("ARRIVAL", token) == 0)
    {
        flight.f = 1;

        token = strtok(NULL, " ");
        strcpy(flight.flightCode, token);
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");
        flight.aFlight.init = atoi(token);
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");
        flight.aFlight.eta = atoi(token);
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");
        flight.aFlight.fuel = atoi(token);
    }
    

    return flight;
}

void addFlight(void *headFlights, Flight flight)
{  
    FlightNode *t = headFlights;
    FlightNode *newFlight = malloc(sizeof(FlightNode));
 
    int x;
    if (flight.f == 0)
    {
        x = flight.dFlight.init;
    }
    else if (flight.f == 1)
    {
        x = flight.aFlight.init;
    }
 
    pthread_mutex_lock(&lockAddFlight);  
 
    if (t->next == NULL) //Se a lista estiver vazia, ou seja, apenas existir o header
    {      
        if(flight.f == 0)
        {  
            newFlight->flight.f = flight.f;
            strcpy(newFlight->flight.flightCode, flight.flightCode);
            newFlight->flight.dFlight.init = flight.dFlight.init;
            newFlight->flight.dFlight.takeoff = flight.dFlight.takeoff;
        }
        else if(flight.f == 1)
        {  
            newFlight->flight.f = flight.f;
            strcpy(newFlight->flight.flightCode, flight.flightCode);
            newFlight->flight.aFlight.init = flight.aFlight.init;
            newFlight->flight.aFlight.eta = flight.aFlight.eta;
            newFlight->flight.aFlight.fuel = flight.aFlight.fuel;
        }
        else
        {
            printf("Error in addFlight()\n");
        }
        t->next = newFlight;
        newFlight->next = NULL;
    }
    else
    {
       if (flight.f == 0)
        {  
            while (t->next !=NULL)  
            {  
                if (t->next->flight.f == 0)
                {
                    if (x < t->next->flight.dFlight.init)
                    {
                        break;
                    }
                }
                else if(t->next->flight.f == 1)
                {
                    if(x < t->next->flight.aFlight.init)
                    {
                        break;
                    }
                }
               
                t = t->next;  
            }
 
            newFlight->flight.f = flight.f;
            strcpy(newFlight->flight.flightCode, flight.flightCode);
            newFlight->flight.dFlight.init = flight.dFlight.init;
            newFlight->flight.dFlight.takeoff = flight.dFlight.takeoff;
 
            newFlight->next = t->next;
            t->next = newFlight;
        }
        else if (flight.f == 1)
        {
            while (t->next != NULL)  
            {  
                if (t->next->flight.f == 0)
                {
                    if (x < t->next->flight.dFlight.init)
                    {
                        break;
                    }
                }
                else if(t->next->flight.f == 1)
                {
                    if(x < t->next->flight.aFlight.init)
                    {
                        break;
                    }
                }
 
                t = t->next;  
            }
 
            newFlight->flight.f = flight.f;
            strcpy(newFlight->flight.flightCode, flight.flightCode);
            newFlight->flight.aFlight.init = flight.aFlight.init;
            newFlight->flight.aFlight.eta = flight.aFlight.eta;
            newFlight->flight.aFlight.fuel = flight.aFlight.fuel;
 
            newFlight->next = t->next;
            t->next = newFlight;
        }
        else
        {
            printf("Error in addFlight()");
        }
       
    }
   
    pthread_mutex_unlock(&lockAddFlight);
}

void removeFlight(FlightNode *headFlights)
{
    FlightNode *toDelete, *third;
    toDelete = headFlights->next;
    third = toDelete->next;
    
    if(third == NULL)
    {
        headFlights->next = NULL;
    }
    else
    {
        headFlights->next = third;
    }
    
    free(toDelete);
}

void printFlights(FlightNode *headFlights) 
{
    FlightNode *t = headFlights;
    t = t->next;    
    printf("\n**********List of Flight in the System (inits)**********\n");
    printf("--------------------------------------------------------\n");

    char str[200];
    while (t != NULL)
    {
        memset(str, 0 ,strlen(str));

        if (t->flight.f == 0)
        {
            printf("DEPARTURE init: %d takeoff: %d\n", t->flight.dFlight.init, t->flight.dFlight.takeoff);
        }
        else 
        {
            printf("ARRIVAL init: %d eta: %d fuel: %d\n", t->flight.aFlight.init, t->flight.aFlight.eta, t->flight.aFlight.fuel);
        }

        fflush(stdout);
        t = t->next;
    }
    
    printf("--------------------------------------------------------\n");
    
}

void *manageFlightsList(void *headFlights)
{   
    FlightNode *t = headFlights;
    int time;

    while(1) 
    {   
        pthread_mutex_lock(&lockAddFlight);

        time = currentTime();

        if (t->next != NULL)
        {   
            if (t->next->flight.f == 0)
            {
                if (time >= t->next->flight.dFlight.init)
                {
                    printf("\nInit %d reached\n", t->next->flight.dFlight.init);
                    createFlightThread(t->next->flight); 
                    removeFlight(headFlights);
                }
            }
            else if (t->next->flight.f == 1)
            {

                if (time >= t->next->flight.aFlight.init)
                {
                    printf("\nInit %d reached\n", t->next->flight.aFlight.init);
                    createFlightThread(t->next->flight);
                    removeFlight(headFlights);
                }
            }
            else
            {
                printf("Manage flight error\n");    
            } 

	    }
        
        pthread_mutex_unlock(&lockAddFlight);
        
        usleep(unitTime*1000000);
    }
}

void *startFlightThread(void *args) 
{   
    Flight *flight = (Flight*)args;
    MQRequest request; 

    int shmSlotNr;

    pthread_t tid = pthread_self();
    if (flight->f == 0)
    {   
        request.msgtype = DEPARTURE;
        strcpy(request.flightCode, flight->flightCode);
        request.tid = tid;
        request.takeoff = flight->dFlight.takeoff;
    }

    if (flight->f == 1)
    {
        request.msgtype = ARRIVAL;
        strcpy(request.flightCode, flight->flightCode);
        request.tid = tid;
        request.eta = flight->aFlight.eta;
        request.fuel = flight->aFlight.fuel;
    }

    char aux[200];
    printf(aux, "%s thread %ld started\n", flight->flightCode, tid);
    addMQRequest(request);

    if(msgrcv(mqID, &request, sizeof(request), tid, 0) < 0)
    {
        printf("error receiving\n");
    }
    else
    {
        shmSlotNr = request.shmSlotNr;
        //printf("Message Received: slot %d\n", shmSlotNr);
    }   

    SlotSHMChanged *tshm = (SlotSHMChanged*)slotSHMChanged;
    SlotCondVar *tcv = (SlotCondVar*)slotCV;
    bool done = false;

    while(!done)
    {
        pthread_mutex_lock(&tcv[0].lockSHM);
        while(tshm[0].SMHChanged != true)
        {   
            pthread_cond_wait(&tcv[0].condSHM, &tcv[0].lockSHM);
        }
        pthread_mutex_unlock(&tcv[0].lockSHM);
        printf("Verify data in thread %ld\n", tid);
        done = checkSHM(shmSlotNr, request);
        tshm[0].SMHChanged = false;
    }

    return NULL;
}

bool checkSHM(int shmSlotNr, MQRequest request)
{
    Slot *t = (Slot*)slotsP;
    char aux[MAXSIZE];
    char trackName[10];

    if(t[shmSlotNr].instruction != - 1)
    {
        if(t[0].trackName == DEPARTURE_01L)
        {
            strcat(trackName, "01L");
        }
        else if(t[0].trackName == DEPARTURE_01R)
        {
            strcat(trackName, "01R");
        }
        else if(t[0].trackName == ARRIVAL_28L)
        {
            strcat(trackName, "28L");
        }
        else if(t[0].trackName == ARRIVAL_28R)
        {
            strcat(trackName, "28R");
        } 

        if(t[shmSlotNr].instruction == TAKEOFF)
        {
            sprintf(aux, "%s DEPARTURE %s started\n", request.flightCode, trackName);
            writeLogReport(currentTime(), aux);
           
            usleep(config.unitTime * config.duractionDeparture * 1000);
            memset(aux, 0 ,strlen(aux));

            sprintf(aux, "%s DEPARTURE %s concluded\n", request.flightCode, trackName);
            writeLogReport(currentTime(), aux);
            memset(aux, 0 ,strlen(aux));
            return true;
        }
        else if (t[shmSlotNr].instruction == LAND)
        {
            sprintf(aux, "%s LANDING %s started\n", request.flightCode, trackName);
            writeLogReport(currentTime(), aux);
           
            usleep(config.unitTime * config.duractionDeparture * 1000);
            memset(aux, 0 ,strlen(aux));

            sprintf(aux, "%s LANDING %s concluded\n", request.flightCode, trackName);
            writeLogReport(currentTime(), aux);
            memset(aux, 0 ,strlen(aux));
            return true;
        }
        else if (t[shmSlotNr].instruction == HOLDING)
        {
            sprintf(aux, "%s HOLDING %d\n", request.flightCode, t[0].holdingTime);
            writeLogReport(currentTime(), aux);
            memset(aux, 0 ,strlen(aux));
        }
        else if (t[shmSlotNr].instruction == DIVERT)
        {
            sprintf(aux, "%s LEAVING TO OTHER AIRPORT\n", request.flightCode);
            writeLogReport(currentTime(), aux);
            memset(aux, 0 ,strlen(aux));
            return true;
        }

        t[shmSlotNr].instruction = - 1;
        memset(trackName, 0 ,strlen(trackName));
    }

    return false;

}

void createFlightThread(Flight flight)
{   
    Flight *args = malloc(sizeof(Flight)); 
    if (flight.f == 0)
    {   
        args->f = 0;
        strcpy(args->flightCode, flight.flightCode);
        args->dFlight.init = flight.dFlight.init;
        args->dFlight.takeoff = flight.dFlight.takeoff;

        printf("%s with init %d created\n", flight.flightCode, flight.dFlight.init);
    }
    else
    {
        args->f = 1;
        strcpy(args->flightCode, flight.flightCode);
        args->aFlight.init = flight.aFlight.init;
        args->aFlight.eta = flight.aFlight.eta;
        args->aFlight.fuel = flight.aFlight.fuel;
        
        printf("%s with init %d created\n", flight.flightCode, flight.aFlight.init);
    }
    
    pthread_t thread_id[thread_incr];
    pthread_create(&thread_id[thread_incr], NULL, startFlightThread, args);

    thread_incr ++;
}

int convertToSeconds(int hour, int minute, int sec)
{   
    return hour*60*60 + minute*60 + sec;
}

int currentTime()
{   
    SlotTime *tt = (SlotTime*)slotT;

    return tt[0].systemCurrentTime;
}

void writeLogReport(int time, char *s) 
{   

    FILE *fp;
    fp = fopen("log.txt", "a+");

    sem_wait(mutex);

    char str[100];
    strcpy(str, s);

    if (fp == NULL)
    {
        printf("Program cannot open the log!\n");
        return;
    }
 
    printf("%d: %s", time, str);
 
    fflush(stdout);
    fprintf(fp, "%d: %s", time, str);

    sem_post(mutex);

    fclose(fp);
}

void loadConfig()
{
    FILE *fp;
    fp=fopen("config.txt", "r");
    if (fp == NULL){
        printf("Program cannot open the config!\n");
        exit(0);
    }
    
    fscanf(fp,"%d\n", &config.unitTime);   
    fscanf(fp,"%d %*s %d\n",&config.duractionDeparture, &config.intervalDeparture);
	fscanf(fp,"%d %*s %d\n",&config.duractionArrival, &config.intervalArrival);
    fscanf(fp,"%d %*s %d\n",&config.holdingMin, &config.holdingMax);
    fscanf(fp,"%d\n",&config.maxDepartures);                          
    fscanf(fp,"%d\n",&config.maxArrivals);

    unitTime = (double)config.unitTime/(double)1000;
    
    fclose(fp);
	
	if (config.unitTime<=0 || config.duractionDeparture<=0 || config.intervalDeparture<=0 || config.duractionArrival<=0 || config.intervalArrival<=0 || config.holdingMin<=0 || config.holdingMax<=0 || config.maxDepartures<=0 || config.maxArrivals<=0)
    {   
        printf("File config.txt contains invalid data!\n");   
        exit(0);
    }

}

void startSemaphores()
{
    sem_unlink("mutex_semaphore");
    
    mutex = sem_open("mutex_semaphore",O_CREAT,0644,1);

    if(mutex == SEM_FAILED)
    {
        perror("Program cannot create semaphore");
        sem_unlink("mutex_semaphore");
        exit(-1);
    }

    #ifdef DEBUGE
    printf("Semaphores created with sucess\n");
    #endif

}

void startSharedMemory() 
{   
    //Pode ser IPC Private? - Torre de controlo vai ser child deste processo
    shmidSlots = shmget(SHM_KEY_FLIGHTS, sizeof(Slot)*(config.maxDepartures+config.maxArrivals), IPC_CREAT|0777);
    if (shmidSlots == -1) {
        perror("Shared memory");
    }
    slotsP = shmat(shmidSlots, NULL, 0);

    Slot *t = (Slot*)slotsP;
    for (int i=0; i<(config.maxDepartures+config.maxArrivals); i++)
    {
        t[i].isAvaiable = 1;
        t[i].instruction = 0;
        t[i].trackName = 0; 
        t[i].holdingTime = 0;
    }

    shmidStats = shmget(SHM_KEY_STATS, sizeof(Stats*), IPC_CREAT|0777);
    if (shmidStats == -1) {
        perror("Shared memory");
    }
    statsP = shmat(shmidStats, NULL, 0);

    Stats *ts = (Stats*)statsP;
    
    ts[0].nFlights = 0;
    ts[0].nFlights_Arrived = 0;
    ts[0].nFlights_Departured = 0;
    ts[0].tAverageWait_Arrival = 0;
    ts[0].tAverageWait_Departure = 0;
    ts[0].nAverageHoldigns_Arrival = 0;
    ts[0].nAverageHoldigns_UrgencyState = 0;
    ts[0].nFlights_Diverted = 0;
    ts[0].nFlights_Rejected = 0;
    
    shmidCondVar = shmget(SHM_KEY_CONDVAR, sizeof(SlotCondVar*), IPC_CREAT|0777);
    if (shmidCondVar == -1) {
        perror("Shared memory");
    }
    slotCV = shmat(shmidCondVar, NULL, 0);

    SlotCondVar *tcv = (SlotCondVar*)slotCV;
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
	pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    if (pthread_cond_init(&tcv[0].condSHM, &cond_attr) != 0) 
    { 
        perror("Não foi possível criar a variavel de condicao condSHM da SHM");
        exit(-1);
    }
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    if (pthread_mutex_init(&tcv[0].lockSHM, &mutex_attr) != 0) 
    { 
        perror("Não foi possível criar a variavel de condicao lockSHM da SHM");
        exit(-1);
    }  

    shmidSlotTime = shmget(SHM_KEY_TIME, sizeof(SlotTime*), IPC_CREAT|0777);
    if (shmidStats == -1) {
        perror("Shared memory");
    }
    slotT = shmat(shmidSlotTime, NULL, 0);

    SlotTime *tt = (SlotTime*)slotT;
    tt[0].systemCurrentTime = 0;

    shmidSMHChanged = shmget(SHM_KEY_SHMCHANGED, sizeof(SlotSHMChanged*), IPC_CREAT|0777);
    if (shmidSMHChanged == -1) {
        perror("Shared memory");
    }
    slotSHMChanged = shmat(shmidSMHChanged, NULL, 0);

    tshm = (SlotSHMChanged*)slotSHMChanged;
    tshm[0].SMHChanged = false;
    SMHChangedFlight = false;
    
    #ifdef DEBUGE
    printf("Shared memory created with sucess\n");
    #endif
}

void startMQ()
{
    mqID=msgget(IPC_PRIVATE, IPC_CREAT|0777);

    if (mqID < 0){
        perror("Program cannot open the MQ: \n");
        exit(0);
    }

    #ifdef DEBUGE
    printf("MQ created with sucess\n");
    #endif
}

void startPipe() 
{   
    FlightNode *headFlights = NULL;
    headFlights = malloc(sizeof(FlightNode));
    if (headFlights == NULL)
    {   
        printf("Error in linked list!\n"); 
        return;
    }
    headFlights->next = NULL;

    thread_incr = 0;

    pthread_t flightList;
    pthread_create(&flightList, NULL, manageFlightsList, headFlights);

    Flight flight;

    int fd;
    char command[50];
    char rCommand[20];
    char wCommand[20];

    unlink("input_pipe");
    if ((mkfifo("input_pipe", O_CREAT|O_EXCL|0666)<0) && (errno!= EEXIST))
    {
        perror("Program cannot create the pipe: ");
        exit(0);
    } 

    if ((fd = open("input_pipe", O_RDWR)) < 0) //Meter permissões certas
    {
        perror("Program cannot create the open the pipe for reading: ");
        exit(0);
    }

    #ifdef DEBUGE
    printf("Pipe created with sucess\n");
    #endif

    while(1) 
    {    
        //Limpa as strings
        memset(command, 0 ,strlen(command));
        memset(rCommand, 0 ,strlen(rCommand));
        memset(wCommand, 0 ,strlen(wCommand));

        read(fd, command, 400);

        strcpy(rCommand, "NEW COMMAND => ");
        strcpy(wCommand, "WRONG COMMAND => ");

        if (verifyCommand(command))
        {   
            writeLogReport(currentTime(), strcat(rCommand, command));
            flight = flightInfo(command);
            addFlight(headFlights, flight);
            printFlights(headFlights);
        }
        else
        {   
            writeLogReport(currentTime(), strcat(wCommand, command));
        }

    }

    close(fd);
}

int main(int argc, char const *argv[])
{   
    system("clear");

    signal(SIGHUP,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGILL,SIG_IGN);
    signal(SIGTRAP,SIG_IGN);
    signal(SIGABRT,SIG_IGN);
    signal(SIGBUS,SIG_IGN);
    signal(SIGFPE,SIG_IGN);
    signal(SIGKILL,SIG_IGN);
    signal(SIGSEGV,SIG_IGN);
    signal(SIGUSR2,SIG_IGN);
    signal(SIGTERM,SIG_IGN);
    signal(SIGCONT,SIG_IGN);
    signal(SIGSTOP,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGUSR1, handlerStats);

    signal(SIGINT,handlerTerminate);
    
    loadConfig();

    startSharedMemory();
    
    startSemaphores();

    startMQ();

    writeLogReport(0, "STARTING PROGRAM\n");

    if (fork() == 0)
    {
        controlTower_pid = getpid();
        controlTower();
    }
    else
    {
        startPipe();
    }

    return 0;
}