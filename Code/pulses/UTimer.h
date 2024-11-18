#define MY_SERV "Utimer_Server"

// pulse code from OS timer to Utimer Server
#define CODE_TIMER   _PULSE_CODE_MINAVAIL+1
// pulse code from Utimer Server to Clients
#define CODE_SERVER   _PULSE_CODE_MINAVAIL+2

struct UTimer{
	int UtimerID; // a unique Utimer ID
	int priority;
	int secondsLeft;
};

typedef struct UTimer_Node{
	struct UTimer_Node* pre;
	struct UTimer_Node* next;
	struct UTimer utimer;
	int rcvid; //notify me when timeout
	struct sigevent pulseEvent;
} UTimer_Node;

struct UTimer_List{
	UTimer_Node* head;
	UTimer_Node* tail;
};

//Message types
#define MT_REGISTER		8			// message to server
#define MT_REGISTERED	9			// message to client
#define MT_CANCEL			10		// message to server
#define MT_CANCELED		11		// message to client

//Message structure
typedef struct {
	int messageType;  // MT_REGISTERED, MT_CANCELED
	int UtimerID;
} ServerMessageT;

typedef struct {
	int messageType;  // MT_REGISTER, MT_CANCEL
	int UtimerID;
	int priority;
	int seconds;
	struct sigevent	pulseEvent;
} ClientMsgT;

typedef union {
	ClientMsgT			cmsg;		// a message from a client,
	struct _pulse		pulse;	// a pulse from QNX timer
} MessageT;
