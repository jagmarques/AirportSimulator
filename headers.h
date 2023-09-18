#ifndef headers_h
#define headers_h

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <sys/msg.h> 
#include <signal.h>
#include <sys/mman.h>

#define SHM_KEY_FLIGHTS 11
#define SHM_KEY_STATS 22
#define SHM_KEY_CONDVAR 33
#define SHM_KEY_TIME 44
#define SHM_KEY_SHMCHANGED 55
#define DEBUG 0

#define SIZENAMES 10
#define MAXSIZE 150
#define PRIORITY 1
#define DEPARTURE 2
#define ARRIVAL 3
#define TAKEOFF 4
#define LAND 5
#define HOLDING 6
#define DIVERT 7
#define DEPARTURE_01L 8  
#define DEPARTURE_01R 9  
#define ARRIVAL_28L 10 
#define ARRIVAL_28R 11 

typedef struct
{
    long msgtype;
    pthread_t tid;
    int shmSlotNr;
    char flightCode[10];
    int takeoff;
	int eta;
	int fuel;
} MQRequest;

typedef struct slot
{
    int isAvaiable;
    int instruction;
    int trackName;
    int holdingTime;
} Slot;

typedef struct slotTime
{
    int systemCurrentTime;
} SlotTime;

typedef struct slotSHMChanged
{
    bool SMHChanged;
} SlotSHMChanged;

typedef struct slotCondVar {
	pthread_mutex_t lockSHM;
	pthread_cond_t condSHM;
} SlotCondVar;

typedef struct config 
{
    int unitTime, 
    duractionDeparture, 
    intervalDeparture,
    duractionArrival,
    intervalArrival,
    holdingMin,
    holdingMax,
    maxDepartures,
    maxArrivals;
} Config;

typedef struct stats{
    int nFlights;
    int nFlights_Arrived;
	int nFlights_Departured;
	int tAverageWait_Arrival;
	int tAverageWait_Departure;
	int nAverageHoldigns_Arrival;
	int nAverageHoldigns_UrgencyState;
	int nFlights_Diverted;
	int nFlights_Rejected;
} Stats;

typedef struct departureFlight
{
    int init;
    int takeoff;   
} DepartureFlight;

typedef struct arrivalFlight
{
    int init;
    int eta;
    int fuel;
} ArrivalFlight;

typedef struct flight
{
    int f; //0 é partida, 1 é chegada
    char flightCode[MAXSIZE];
    DepartureFlight dFlight;
    ArrivalFlight aFlight;
} Flight;

typedef struct flightNode
{
    Flight flight;
    struct flightNode *next;
} FlightNode;

//--Structs da Tower
typedef struct
{   
    int shmSlotNr;
    int takeoff;
} DepartureTrack;

typedef struct 
{   
    int shmSlotNr;
    int isPriority;
    int isOnHolding;
    int eta;
    int fuel;
} ArrivalTrack;

typedef struct departureTrackNode
{
    DepartureTrack departureTrack;
    struct departureTrackNode *next;
} DepartureTrackNode;

typedef struct arrivalTrackNode
{
    ArrivalTrack arrivalTrack;
    struct arrivalTrackNode *next;
} ArrivalTrackNode;

void *slotsP;
void *statsP;
void *slotT;
void *slotCV;
void *slotSHMChanged;

int shmidSlots, shmidStats, shmidSlotTime, shmidCondVar, shmidSMHChanged;

int startTime;
double unitTime;

int mqID;

int thread_incr;
//--
//--Tower
DepartureTrackNode *headDepartureTracks;
ArrivalTrackNode *headArrivalTracks;
//--

void handlerTerminate(int signum);

void addMQRequest(MQRequest request);
void readMQRequest();

int verifyFlightID(char *s);
int verifyCommand(char *s);

Flight flightInfo(char *s);
void addFlight(void *headFlights, Flight flight);
void removeFlight(FlightNode *headFlights);
void printFlights(FlightNode *headFlights);
void *manageFlightsList(void *headFlights);
void *startFlightThread(void *args);
void createFlightThread(Flight flight);

int currentTime();

void writeLogReport(int time, char *s);
void loadConfig();
void startSemaphores();
void startSharedMemory();
void startMQ();
void startPipe();

//--- Funcões da control tower
void startTracks();
int findShmSlot();
void freeShmSlot(int shmSlotnr);
void controlTower();

void MQRequestToFlight(MQRequest request);

void handleRequest(MQRequest request);
void createMQDepartureRequestThread(pthread_t *thread_id);
void createMQArrivalRequestThread(pthread_t *thread_id);
void addDepartureTrack(MQRequest request);
void addArrivalTrack(MQRequest request, int isPriority);
int checkPriority(MQRequest request);
void printArrivalTracks();
void printDepartureTracks();
void startSystemTime();
void removeDepartureTrack();
void removeArrivalTrack(ArrivalTrackNode *temp, ArrivalTrackNode *previous);
int controlTower_pid;
void handlerStats(int signum);
void createMQPriorityRequestThread(pthread_t *pthread_idPriority);
void *readMQDepartureRequest(void *args);
void *readMQArrivalRequest(void *args);
void *readMQPriorityRequest(void *args);
Config config;
bool checkSHM(int shmSlotNr, MQRequest request);
void doAverages();


int nrMaxDepartures;
int nrMaxArrivals;
#endif /* headers_h */