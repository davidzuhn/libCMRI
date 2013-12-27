#include <CMRI.h>

#define ATTN 0xFF
#define STX  0x02
#define ETX  0x03
#define DLE  0x10
#define ACK  0x06
#define NAK  0x15

#define STRICT_PROTOCOL_CHECKING false

#define CMRI_BROADCAST_ADDRESS 0




CMRI::CMRI(Stream & s, uint8_t n):nodeId(n + 65), stream(s)
{
    bufTop = 0;
    currentState = START;
    debug = NULL;
    tickCount = 0;
    charCount = 0;
    messagesSeen = 0;
    messagesProcessed = 0;
}

void CMRI::addDebugStream(Stream * s)
{
    if (s != NULL) {
	debug = s;
    }
}


boolean CMRI::isForMe()
{
    if (messageDest == CMRI_BROADCAST_ADDRESS || messageDest == nodeId)
	return true;
    else
	return false;
}


void CMRI::processMessage()
{
    messagesSeen += 1;

    if (debug) {
	debug->print("---- complete message received: dest ");
	debug->print(messageDest);
	debug->print("  type: 0x");
	debug->print(messageType, HEX);
	if (isprint(messageType)) {
	    debug->print(" '");
	    debug->print((char) messageType);
	    debug->print("'");
	}
	debug->print("\n---- ");

	if (bufTop > 0) {
	    for (int i = 0; i < bufTop; i++) {
		debug->print(buf[i], HEX);
		debug->print(" ");
	    }
	    debug->println("");
	}
    }
    // do not process the message if it is not addressed to us
    if (!isForMe())
	return;


    // now do something with the message
    if (debug) {
	debug->println("---- processing this message");
    }

    messagesProcessed += 1;


}


boolean CMRI::addCharToMessage(unsigned char b)
{
    // we cannot save the data if it exceeds our maximum message length
    if (bufTop >= MAX_MESG_LEN) {
	return false;
    }

    buf[bufTop++] = b;
    return true;
}


void CMRI::resetMessage()
{
    messageType = 0;
    messageDest = -1;
    bufTop = 0;
}


char *CMRI::printState(currentStateType state)
{
    char *str;
    switch (state) {
    case START:
	str = "START";
	break;
    case ATTN_NEXT:
	str = "ATTN_NEXT";
	break;
    case STX_NEXT:
	str = "STX_NEXT";
	break;
    case ADDR_NEXT:
	str = "ADDR_NEXT";
	break;
    case TYPE_NEXT:
	str = "TYPE_NEXT";
	break;
    case MAYBE_DATA_NEXT:
	str = "MAYBE_DATA_NEXT";
	break;
    case DATA_NEXT:
	str = "DATA_NEXT";
	break;
    default:
	str = "*** UNKNOWN ***";
	break;
    }

    return str;
}


void CMRI::changeState(currentStateType newState, unsigned char inputChar)
{
    if (debug) {
	debug->print("Current state: ");
	debug->print(printState(currentState));
	debug->print(", input char 0x");
	debug->print((int) inputChar, HEX);
	debug->print("  ===>  ");
	debug->println(printState(newState));

	if (newState == START) {
	    debug->print("\n\nStart of new message processing\n");
	}
    }

    currentState = newState;
}





void CMRI::nextChar(unsigned char b)
{
    charCount += 1;


    switch (currentState) {
    case START:
	changeState((b == ATTN) ? ATTN_NEXT : START, b);
	resetMessage();
	break;

    case ATTN_NEXT:
	changeState((b == ATTN) ? STX_NEXT : START, b);
	break;

    case STX_NEXT:
	changeState((b == STX) ? ADDR_NEXT : START, b);
	break;

    case ADDR_NEXT:
	messageDest = (uint8_t) b;
	changeState(TYPE_NEXT, b);
	break;

    case TYPE_NEXT:
	messageType = b;
	changeState(MAYBE_DATA_NEXT, b);
	break;

    case MAYBE_DATA_NEXT:
	switch (b) {
	case ETX:
	    processMessage();
	    changeState(START, b);
	    break;
	case DLE:
	    changeState(DATA_NEXT, b);
	    break;
	default:
	    changeState(addCharToMessage(b) ? MAYBE_DATA_NEXT : START, b);
	    break;
	}
	break;

    case DATA_NEXT:
	// to be completely true to the protocol, this should probably reject
	// escaped data characters that are not STX, ETX or DLE

	// be liberal in what you accept, strict in what you emit

#if STRICT_PROTOCOL_CHECKING
	if (b == STX || b == ETX || b == DLE) {
	    if (addCharToMessage(b)) {
		changeState(MAYBE_DATA_NEXT, b);
	    }
	}
	changeState(START, b);
#else
	changeState(addCharToMessage(b) ? MAYBE_DATA_NEXT : START, b);
#endif
	break;

    default:
	// THIS CASE SHOULD NEVER HAPPEN
	if (debug) {
	    debug->print("Unknown CMRI state transition: state is ");
	    debug->println(currentState);
	}
    }
}






void CMRI::check()
{
    tickCount += 1;


    if (stream.available() < 1) {
	return;
    }

    while (stream.available()) {
	int b = stream.read();

	if (b >= 0) {
	    nextChar((unsigned char) b);
	} else {
	    if (debug) {
		debug->println("error reading from available stream");
	    }
	}
    }
}



unsigned long int CMRI::getTickCount()
{
    return tickCount;
}


void CMRI::printSummary()
{
    if (debug) {
	debug->println("CMRI communications summary");
	debug->print("  tickCount: ");
	debug->println(tickCount);
	debug->print("  charCount: ");
	debug->println(charCount);
	debug->print("  messagesSeen: ");
	debug->println(messagesSeen);
	debug->print("  messagesProcessed: ");
	debug->println(messagesProcessed);

    }
}
