/* Computer Model Railroad Interface
 *
 * This package implements CMRInet, allowing a device
 * to be a participant on the CMRI RS-485 network.
 *
 */

#include "Arduino.h"

class CMRI {
  public:
    CMRI(Stream & stream, uint8_t nodeId);

    void check();

    static const int MAX_MESG_LEN = 72;

    void addDebugStream(Stream * s);
    enum currentStateType { START, ATTN_NEXT, STX_NEXT, ADDR_NEXT,
	    TYPE_NEXT, MAYBE_DATA_NEXT, DATA_NEXT };

    unsigned long int getTickCount();

    void printSummary();

  private:
    void nextChar(unsigned char b);

    boolean isForMe();

    void resetMessage();

    // true if successful, false if any error (buffer overflow, mainly)
    boolean addCharToMessage(unsigned char b);

    void processMessage();


    char *printState(currentStateType);
    currentStateType currentState;
     Stream & stream;
    uint8_t nodeId;

    int messageDest;
    unsigned char messageType;
    unsigned char buf[MAX_MESG_LEN];
    int bufTop;

    Stream *debug;

    unsigned long int tickCount;
    unsigned long int charCount;
    unsigned long int messagesSeen;
    unsigned long int messagesProcessed;

    void changeState(currentStateType newstate, unsigned char inputChar);

};
