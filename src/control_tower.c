#include "airport.h"

pthread_mutex_t lockArrival = PTHREAD_MUTEX_INITIALIZER;
int systemTime;

// Sends slot information back to the requesting flight thread.
void MQRequestToFlight(MQRequest request)
{  
    request.msgtype = request.tid;
    request.shmSlotNr = request.shmSlotNr;
    
    if(msgsnd(mqID, &request, sizeof(request), 0) < 0)
    {   
        printf("error sending\n");
    }
    else
    {   
        printf("Send slot to thread %ld\n", request.tid);
    }
}

// Dispatches the MQ request to the right queue, applying priorities.
void handleRequest(MQRequest request) //Meter sincronizações!
{   
    //Aumentar o contador de numero de voos - Pode ser um semaforo ! 
    int isPriority = 0;
    request.shmSlotNr = findShmSlot();

    if(request.msgtype == DEPARTURE)
    {   
        MQRequestToFlight(request);
        addDepartureTrack(request);
    }
    else if (request.msgtype == ARRIVAL)
    {
        MQRequestToFlight(request);
        addArrivalTrack(request, isPriority);
    }
    else if (request.msgtype == PRIORITY)
    {
        MQRequestToFlight(request);
        isPriority = 1;
        addArrivalTrack(request, isPriority);
    }

}

// Marks a shared-memory slot as reusable by clearing its state.
void freeShmSlot(int shmSlotnr) //Mecanismo de sincronização !!!-?
{
    Slot *tempSlot = (Slot*)slotsP;
    tempSlot[shmSlotnr].isAvaiable = 1;
    tempSlot[shmSlotnr].trackName = 0; 
    tempSlot[shmSlotnr].holdingTime = 0;
}

// Returns the first available shared-memory slot or -1 if none exist.
int findShmSlot() //Mecanismos de sincronização !!!-?
{
    int max = config.maxDepartures+config.maxDepartures;
    Slot *tempSlot = (Slot*)slotsP;
    
    for (int i=0; i<max; i++)
    {    
        if (tempSlot[i].isAvaiable == 1)
        {   
            tempSlot[i].isAvaiable = 0;
            return i;
        }
    }

    return -1;
}

void *readMQPriorityRequest(void *args)
{
    (void)args;
    MQRequest request;
    
    //Fazer semafros, só faço handle quando o ultimo acabar
    while(1)
    {
        if(msgrcv(mqID, &request, sizeof(MQRequest)-sizeof(long), PRIORITY, 0) < 0)
        {
            printf("error receiving\n");
        }
        else
        {
            handleRequest(request);
        }
    } 
}

void createMQPriorityRequestThread(pthread_t *pthread_idPriority)
{
    Config *args = malloc(sizeof(Config)); 
      
    args->unitTime = config.unitTime;
    args->duractionDeparture = config.duractionDeparture;
    args->intervalDeparture = config.intervalDeparture;
    args->duractionArrival = config.duractionArrival;
    args->intervalArrival = config.intervalArrival;
    args->holdingMin = config.holdingMin;
    args->holdingMax = config.holdingMax;
    args->maxDepartures = config.maxDepartures;
    args->maxArrivals = config.maxArrivals;
    
    pthread_create(pthread_idPriority, NULL, readMQPriorityRequest, args);
}

void printArrivalTracks() 
{
    ArrivalTrackNode *ta = headArrivalTracks;
    ta = ta->next;    

    printf("\n*****Queue of Arrival Flight in the System (inits)*****\n");
    printf("-------------------------------------------------------\n");

    while (ta != NULL)
    {
        //printf("ARRIVAL eta: %d fuel: %d\n", ta->arrivalTrack.eta, ta->arrivalTrack.fuel);

        //printf("ARRIVAL eta: %d fuel: %d slot: %d\n", ta->arrivalTrack.eta, ta->arrivalTrack.fuel, ta->arrivalTrack.shmSlotNr);
        printf("ARRIVAL eta: %d fuel: %d PRIORITY: %d\n", ta->arrivalTrack.eta, ta->arrivalTrack.fuel, ta->arrivalTrack.isPriority);
        
        ta = ta->next;
    }
    
    printf("-------------------------------------------------------\n");
}

void addArrivalTrack(MQRequest request, int isPriority)
{   
    if (nrMaxArrivals >= config.maxArrivals)
    {
        printf("Arrival flight rejected! Arrival flights are full.\n");
        return;
    }

    ArrivalTrackNode *ta = headArrivalTracks;
    ArrivalTrackNode *newFlightTrack = malloc(sizeof(ArrivalTrackNode));

    pthread_mutex_lock(&lockArrival);

    if (ta->next == NULL)
    {       
        if (isPriority == 0)
        {
            newFlightTrack->arrivalTrack.eta = request.eta;
            newFlightTrack->arrivalTrack.fuel = request.fuel;
            newFlightTrack->arrivalTrack.shmSlotNr = request.shmSlotNr;
            newFlightTrack->arrivalTrack.isPriority = 0;
            newFlightTrack->arrivalTrack.isOnHolding = -1;
            

            newFlightTrack->next = ta->next;
            ta->next = newFlightTrack;
        }
        else if (isPriority == 1)
        {
            while (ta->next!=NULL)
            {  
                if((request.eta) < ta->next->arrivalTrack.eta )
                {
                    break;
                }
                ta = ta->next;  
            }
            
            newFlightTrack->arrivalTrack.eta = request.eta;
            newFlightTrack->arrivalTrack.fuel = request.fuel;
            newFlightTrack->arrivalTrack.shmSlotNr = request.shmSlotNr;
            newFlightTrack->arrivalTrack.isPriority = 1;
            newFlightTrack->arrivalTrack.isOnHolding = -1;

            newFlightTrack->next = ta->next;
            ta->next = newFlightTrack;
        }
    }
    else
    {
        if (isPriority == 0)
        {
            newFlightTrack->arrivalTrack.eta = request.eta;
            newFlightTrack->arrivalTrack.fuel = request.fuel;
            newFlightTrack->arrivalTrack.shmSlotNr = request.shmSlotNr;
            newFlightTrack->arrivalTrack.isPriority = 0;
            newFlightTrack->arrivalTrack.isOnHolding = -1;
            

            newFlightTrack->next = ta->next;
            ta->next = newFlightTrack;
        }
        else if (isPriority == 1)
        {
            while (ta->next!=NULL)
            {  
                if((request.eta) < ta->next->arrivalTrack.eta )
                {
                    break;
                }
                ta = ta->next;  
            }
            
            newFlightTrack->arrivalTrack.eta = request.eta;
            newFlightTrack->arrivalTrack.fuel = request.fuel;
            newFlightTrack->arrivalTrack.shmSlotNr = request.shmSlotNr;
            newFlightTrack->arrivalTrack.isPriority = 1;
            newFlightTrack->arrivalTrack.isOnHolding = -1;

            newFlightTrack->next = ta->next;
            ta->next = newFlightTrack;
        }
    }

    nrMaxArrivals++;
    //printArrivalTracks();
    pthread_mutex_unlock(&lockArrival);
    
}

void removeArrivalTrack(ArrivalTrackNode *temp, ArrivalTrackNode *previous)
{
    ArrivalTrackNode *toDelete, *third;
    toDelete = temp;
    third = toDelete->next;

    pthread_mutex_lock(&lockArrival);
    
    if(previous == headArrivalTracks && third == NULL)
    {
        headArrivalTracks->next = NULL;
    }
    if(third == NULL)
    {
        previous->next = NULL;
    }
    else
    {
        previous->next = third;
    }
    
    nrMaxArrivals--;

    free(toDelete);
    pthread_mutex_unlock(&lockArrival);
}


void *readMQArrivalRequest(void *args)
{
    (void)args;
    MQRequest request;
    
    //Fazer semafros, só faço handle quando o ultimo acabar
    while(1)
    {
        if(msgrcv(mqID, &request, sizeof(MQRequest)-sizeof(long), ARRIVAL, 0) < 0)
        {
            printf("error receiving\n");
        }
        else
        {
            handleRequest(request);
        }
    } 
}

void createMQArrivalRequestThread(pthread_t *pthread_idArrival) 
{  
    Config *args = malloc(sizeof(Config)); 
      
    args->unitTime = config.unitTime;
    args->duractionDeparture = config.duractionDeparture;
    args->intervalDeparture = config.intervalDeparture;
    args->duractionArrival = config.duractionArrival;
    args->intervalArrival = config.intervalArrival;
    args->holdingMin = config.holdingMin;
    args->holdingMax = config.holdingMax;
    args->maxDepartures = config.maxDepartures;
    args->maxArrivals = config.maxArrivals;
    
    pthread_create(pthread_idArrival, NULL, readMQArrivalRequest, args);
}

void printDepartureTracks()
{
    DepartureTrackNode *td = headDepartureTracks;
    td = td->next;    
    printf("\n****Queue of Departure Flight in the System (inits)****\n");
    printf("-------------------------------------------------------\n");

    while (td != NULL)
    {
        printf("DEPARTURE takeoff: %d\n", td->departureTrack.takeoff);
        //printf("DEPARTURE takeoff: %d slot: %d\n", td->departureTrack.takeoff, td->departureTrack.shmSlotNr);
        
        td = td->next;
    }
    
    printf("-------------------------------------------------------\n");
}

void addDepartureTrack(MQRequest request)
{   
    if (nrMaxDepartures > config.maxDepartures)
    {
        //Não aceitar voo
        printf("Departure flight rejected! Departure flights are full.\n");
        return;
    }

    DepartureTrackNode *td = headDepartureTracks;
    DepartureTrackNode *newFlightTrack = malloc(sizeof(DepartureTrackNode));

    if (td->next == NULL) //Se a lista estiver vazia, ou seja, apenas existir o header
    {       
        newFlightTrack->departureTrack.takeoff = request.takeoff;
        newFlightTrack->departureTrack.shmSlotNr = request.shmSlotNr;

        td->next = newFlightTrack;
        newFlightTrack->next = NULL;
    }
    else
    {
        while (td->next!=NULL && request.takeoff > td->next->departureTrack.takeoff)  
        {  
            td = td->next;  
        }

        newFlightTrack->departureTrack.takeoff = request.takeoff;
        newFlightTrack->departureTrack.shmSlotNr = request.shmSlotNr;

        newFlightTrack->next = td->next;
        td->next = newFlightTrack;
    }

    nrMaxDepartures++;
    //printDepartureTracks(); 
}

void removeDepartureTrack()
{
    DepartureTrackNode *toDelete, *third;
    toDelete = headDepartureTracks->next;
    third = toDelete->next;
    
    if(third == NULL)
    {
        headDepartureTracks->next = NULL;
    }
    else
    {
        headDepartureTracks->next = third;
    }
    
    nrMaxDepartures--;

    free(toDelete);
}

void *readMQDepartureRequest(void *args)
{
    (void)args;
    MQRequest request;
    
    //Fazer semafros, só faço handle quando o ultimo acabar
    while(1)
    {
        if(msgrcv(mqID, &request, sizeof(MQRequest)-sizeof(long), DEPARTURE, 0) < 0)
        {   
            printf("error receiving\n");
        }
        else
        {
            handleRequest(request);
            //addDepartureTrack(request, config);
            //Confirmar se há espaço na track
        }
    } 
}

void createMQDepartureRequestThread(pthread_t *pthread_idDeparture)
{
    Config *args = malloc(sizeof(Config)); 
      
    args->unitTime = config.unitTime;
    args->duractionDeparture = config.duractionDeparture;
    args->intervalDeparture = config.intervalDeparture;
    args->duractionArrival = config.duractionArrival;
    args->intervalArrival = config.intervalArrival;
    args->holdingMin = config.holdingMin;
    args->holdingMax = config.holdingMax;
    args->maxDepartures = config.maxDepartures;
    args->maxArrivals = config.maxArrivals;
    
    pthread_create(pthread_idDeparture, NULL, readMQDepartureRequest, args); 
}

void startTracks()
{
    headDepartureTracks = (DepartureTrackNode*)malloc(sizeof(DepartureTrackNode)*(config.maxDepartures+1));
    headArrivalTracks = (ArrivalTrackNode*)malloc(sizeof(ArrivalTrackNode)*(config.maxArrivals+1));

    headDepartureTracks->next = NULL;
    headArrivalTracks->next = NULL;
}

//------------------------------------------------

int holdingDuration()
{
    int lower = config.holdingMin;
    int upper = config.holdingMax;

    return (rand() % (upper - lower + 1)) + lower; 
}

void addArrivalTrackNode(ArrivalTrackNode *newFlightTrack)
{
    ArrivalTrackNode *ta = headArrivalTracks;
    newFlightTrack = malloc(sizeof(ArrivalTrackNode));

    pthread_mutex_lock(&lockArrival);

    if (ta->next == NULL)
    {       
        ta->next = newFlightTrack;
        newFlightTrack->next = NULL;
    }
    else 
    {
        while (ta->next!=NULL)
        {  
            if((newFlightTrack->arrivalTrack.eta) < ta->next->arrivalTrack.eta )
            {
                break;
            }
            ta = ta->next;  
        }

        newFlightTrack->next = ta->next;
        ta->next = newFlightTrack;
    }

    pthread_mutex_unlock(&lockArrival);
}

void decreaseFueleta() //Fazer com que para depois dos 5 primeiros façam holding - Falta isto
{
    ArrivalTrackNode *temp, *previous, *new;
    int timeHolding, shmSlotNr, count;
    Slot *tempSlot = (Slot*)slotsP;
    SlotSHMChanged *tshm = (SlotSHMChanged*)slotSHMChanged;
    SlotCondVar *tcv = (SlotCondVar*)slotCV;

    count = 0;
    previous = headArrivalTracks;
    temp = headArrivalTracks->next;
    while (temp != NULL)
    {   
        count++;
        if (temp->arrivalTrack.eta > 0)
        {   
            temp->arrivalTrack.eta = temp->arrivalTrack.eta - 1;

            if (temp->arrivalTrack.eta == -1) //SE ETA = -1 / Não faço a zero para o voo ter a oportunidade de aterrar quando o eta = 0;s
            {   
                if (temp->arrivalTrack.isPriority == 0 && systemTime > temp->arrivalTrack.isOnHolding) //Se não for prioritário
                {
                    timeHolding = holdingDuration();
                    temp->arrivalTrack.isOnHolding = systemTime + timeHolding;
                    temp->arrivalTrack.eta = temp->arrivalTrack.eta + timeHolding;
                    shmSlotNr = temp->arrivalTrack.shmSlotNr;
                    tempSlot[shmSlotNr].instruction = HOLDING;
                    
                    tshm[0].SMHChanged = true;
                    pthread_cond_broadcast(&tcv[0].condSHM);
                    
                    new = temp;
                    removeArrivalTrack(temp, previous);
                    addArrivalTrackNode(new);
                }  
            }
        }

        if (temp->arrivalTrack.fuel > 0)
        {
            temp->arrivalTrack.fuel = temp->arrivalTrack.fuel -1;

            if (temp->arrivalTrack.fuel == 0) //SE FUEL = 0
            {       
                shmSlotNr = temp->arrivalTrack.shmSlotNr;
                tempSlot[shmSlotNr].instruction = DIVERT;
                
                tshm[0].SMHChanged = true;
                pthread_cond_broadcast(&tcv[0].condSHM);
                removeArrivalTrack(temp, previous);
            }
        }
        previous = temp;
        temp = temp->next;
    }

    printArrivalTracks();
    printDepartureTracks();
}

int nrADepartureTracksAvaiable(int systemTime, int *dep01L, int *dep01R)
{   
    int count = 0;

    if (systemTime > *dep01L && *dep01L != -1)
    {
        *dep01L = -1;
    }
    if (*dep01L == -1)
    {
        count++;
    }
    if (systemTime > *dep01R && *dep01R != -1)
    {
        *dep01R = -1;
    }
    if (*dep01R == -1)
    {
        count++;
    }
    
    return count;
}

int nrArrivalTracksAvaiable(int systemTime, int *arr28L, int *arr28R)
{
    int count = 0;
    
    if (systemTime > *arr28L && *arr28L != -1)
    {
        *arr28L = -1;
    }
    if (*arr28L == -1)
    {
        count++;
    }
    if (systemTime > *arr28R  && *arr28R != -1) 
    {
        *arr28R = -1;
    }
    if (*arr28R == -1)
    {
        count ++;
    }

    return count;
}

void doInstruction(int instruction, void *tempTrack, void *previous, int trackName)
{   
    int shmSlotNr = -1;
    Slot *tempSlot = (Slot*)slotsP;
    SlotSHMChanged *tshm = (SlotSHMChanged*)slotSHMChanged;
    SlotCondVar *tcv = (SlotCondVar*)slotCV; 

    if (instruction == TAKEOFF)
    {   
        DepartureTrackNode *tempDepartureTrack = (DepartureTrackNode*)tempTrack;

        shmSlotNr = tempDepartureTrack->departureTrack.shmSlotNr;

        tempSlot[shmSlotNr].instruction = instruction;
        tempSlot[shmSlotNr].trackName = trackName;
        tshm[0].SMHChanged = true;
        pthread_cond_broadcast(&tcv[0].condSHM);
        removeDepartureTrack();
        freeShmSlot(shmSlotNr);

    }
    else if(instruction == LAND)
    {   
        ArrivalTrackNode *tempArrivalTrack = (ArrivalTrackNode*)tempTrack;
        ArrivalTrackNode *previousArrivalTrack = (ArrivalTrackNode*)previous;

        shmSlotNr = tempArrivalTrack->arrivalTrack.shmSlotNr;

        tempSlot[shmSlotNr].instruction = instruction;
        tempSlot[shmSlotNr].trackName = trackName;

        tshm[0].SMHChanged = true;
        pthread_cond_broadcast(&tcv[0].condSHM);
        removeArrivalTrack(tempArrivalTrack, previousArrivalTrack);
        freeShmSlot(shmSlotNr);
    }   
    else
    {
        printf("Error in start smth\n");
    }
}

void escalonarVoo(int systemTime, int *dep01L, int *dep01R, int *arr28L, int *arr28R)
{   

    ArrivalTrackNode *tempArrivalTrack, *previous; 
    DepartureTrackNode *tempDepartureTrack;

    int nrDepTracksAvaiable, nrArrTracksAvaiable;
    nrDepTracksAvaiable = nrADepartureTracksAvaiable(systemTime, dep01L, dep01R);
    nrArrTracksAvaiable = nrArrivalTracksAvaiable(systemTime, arr28L, arr28R);
    if (nrDepTracksAvaiable == 0 && nrArrTracksAvaiable == 0) //NENHUMA PISTA DISPONIVEL
    {
        return;
    }

    int intervalDeparture = config.intervalDeparture;
    int durationDeparture = config.duractionDeparture;
    int intervalArrival = config.intervalArrival;
    int durationArrival = config.duractionArrival;

    printf("TIME: %d | Nr Dep Tracks Avaible: %d | Nr Arr Tracks Avaible: %d\n", systemTime, nrDepTracksAvaiable, nrArrTracksAvaiable);

    //Ter uma variavel que diz se existe ou nao prioritarios para não fazer isto desnecessariamente!
    previous = headArrivalTracks;
    tempArrivalTrack = headArrivalTracks->next;
    while (tempArrivalTrack != NULL) //PERCORRE A LISTA DE ARRIVALS E VE SE ALGUMA É PRIORITARIO
    {   
        nrArrTracksAvaiable = nrArrivalTracksAvaiable(systemTime, arr28L, arr28R);

        if (nrDepTracksAvaiable != 2 && nrArrTracksAvaiable > 0) //NÃO PODE HAVER NENHUMA PISTA DE DEPARTURE OCUPADA E TEM DE HAVER PISTAS DE ARRIVALS LIVRES
        {   
            return; //RETURN PQ HAVENDO UM VOO PRIORITARIO NÃO VOU PASSAR PARA OS PROXIMOS CASOS E METER UM A ENTUPIR
        }
        
        if (tempArrivalTrack->arrivalTrack.isPriority == 1 && tempArrivalTrack->arrivalTrack.eta == 0) //VOO PRIORITARIO
        {   
            if (*arr28L == -1)
            {    
                *arr28L = systemTime + durationArrival + intervalArrival;
                doInstruction(LAND, tempArrivalTrack, previous, ARRIVAL_28L);
            }
            else if(*arr28R == -1)
            {
                *arr28R = systemTime + durationArrival + intervalArrival;
                doInstruction(LAND, tempArrivalTrack, previous, ARRIVAL_28R);
            }
        }

        previous = tempArrivalTrack;
        tempArrivalTrack = tempArrivalTrack->next;
    }

    previous = headArrivalTracks;
    tempArrivalTrack = headArrivalTracks->next;
    while (tempArrivalTrack != NULL) //PERCORRE A LISTA DE ARRIVALS SE ETA = 0 CORRE O VOO
    {   
        nrArrTracksAvaiable = nrArrivalTracksAvaiable(systemTime, arr28L, arr28R);

        if (nrDepTracksAvaiable != 2 && nrArrTracksAvaiable != 0) //NÃO PODE HAVER NENHUMA PISTA DE DEPARTURE OCUPADA E TEM DE HAVER PISTAS DE ARRIVALS LIVRES
        {
            break;
        }

        if (tempArrivalTrack->arrivalTrack.eta == 0)
        {
            if (*arr28L == -1)
            {
                 *arr28L = systemTime + durationArrival + intervalArrival;
                 doInstruction(LAND, tempArrivalTrack, previous, ARRIVAL_28L);
            }
            else if (*arr28R == -1)
            {
                 *arr28R = systemTime + durationArrival + intervalArrival;
                 doInstruction(LAND, tempArrivalTrack, previous, ARRIVAL_28R);
            }
        }

        previous = tempArrivalTrack;
        tempArrivalTrack = tempArrivalTrack->next;
    }

    tempDepartureTrack = headDepartureTracks->next;
    while (tempDepartureTrack != NULL)
    {   
        nrDepTracksAvaiable = nrADepartureTracksAvaiable(systemTime, dep01L, dep01R);

        if (nrDepTracksAvaiable != 0 && nrArrTracksAvaiable != 2)
        {
            break;
        }

        if (*dep01L == -1)
            {   
                *dep01L = systemTime + durationDeparture + intervalDeparture;
                doInstruction(TAKEOFF, NULL, tempDepartureTrack, DEPARTURE_01L);
        }
        else if (*dep01R == -1)
        {       
                *dep01R = systemTime + durationDeparture + intervalDeparture;
                doInstruction(TAKEOFF, NULL, tempDepartureTrack, DEPARTURE_01R);
        }
        
        tempDepartureTrack = tempDepartureTrack->next;
    }
}

void *systemTimeThread(void *arg) //Mecanismos de sincronização?
{   
    systemTime = 0;
    int unitTime = *(int*)arg*1000;
    SlotTime *tt = (SlotTime*)slotT;

    int dep01L = -1;
    int dep01R = -1;
    int arr28L = -1;
    int arr28R = -1;

    while (1)
    {   
        
        escalonarVoo(systemTime, &dep01L, &dep01R, &arr28L, &arr28R);

        sleepForMicros(unitTime);
        //printf("Time: %d\n", tt[0].systemCurrentTime);
        systemTime += 1;
        tt[0].systemCurrentTime = systemTime;

        decreaseFueleta();
    }

    return NULL;
}

void startSystemTime()
{
    int *arg = malloc(sizeof(int));
    *arg = config.unitTime;

    pthread_t pthread_idTime;
    pthread_create(&pthread_idTime, NULL, systemTimeThread, arg);
}

void controlTower()
{   
    startSystemTime();
    startTracks();

    nrMaxDepartures = 0;
    nrMaxArrivals = 0;

    pthread_t pthread_idDeparture, pthread_idArrival, pthread_idPriority;
    createMQDepartureRequestThread(&pthread_idDeparture);
    createMQArrivalRequestThread(&pthread_idArrival);
    createMQPriorityRequestThread(&pthread_idPriority);

    pthread_join(pthread_idDeparture, NULL);
    pthread_join(pthread_idArrival, NULL);
    pthread_join(pthread_idPriority, NULL);
}