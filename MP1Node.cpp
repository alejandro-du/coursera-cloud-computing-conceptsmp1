/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        // create JOINREQ message: format of data is {struct Address myaddr}
        MessageHdr msg;
        msg.msgType = JOINREQ;
        msg.fromAddress = memberNode->addr;
        msg.data = &memberNode->addr;

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)&msg, sizeof(msg));
    }

    return 1;
}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
    MessageHdr* receivedMessage = (MessageHdr*) data;

    switch(receivedMessage->msgType) {
        case JOINREQ:
            handleJOINREQ(receivedMessage);
            break;
        case JOINREP:
            handleJOINREP(receivedMessage);
            break;
        case GOSSIP:
            handleGOSSIP(receivedMessage);
            break;
    }
}

MemberListEntry MP1Node::toMemberListEntry(Address address) {
    MemberListEntry entry(address.addr[0], address.addr[4], 1, par->getcurrtime());
    return entry;
}

Address MP1Node::toAddress(MemberListEntry entry) {
    Address address;
    address.addr[0] = entry.getid();
    address.addr[1] = 0;
    address.addr[2] = 0;
    address.addr[3] = 0;
    address.addr[4] = entry.getport();
    return address;
}

void MP1Node::handleJOINREQ(MessageHdr* joinReqMessage) {
    Address newAddr = *((Address*) joinReqMessage->data);
    MemberListEntry entry = toMemberListEntry(newAddr);
    entry.heartbeat = 0;
    entry.timestamp = par->globaltime;
    memberNode->memberList.push_back(entry);

    MessageHdr joinRepMessage;
    joinRepMessage.msgType = JOINREP;
    joinRepMessage.fromAddress = memberNode->addr;
    joinRepMessage.data = &memberNode->addr;
    emulNet->ENsend(&memberNode->addr, &newAddr, (char*) &joinRepMessage, sizeof(joinRepMessage));
    log->logNodeAdd(&memberNode->addr, &newAddr);
}

void MP1Node::handleJOINREP(MessageHdr* joinRepMessage) {
    memberNode->inGroup = true;
    Address joinedAddr = *((Address*) joinRepMessage->data);
    
    MemberListEntry entry = toMemberListEntry(joinedAddr);
    entry.heartbeat = 0;
    entry.timestamp = par->globaltime;
    memberNode->memberList.push_back(entry);

    log->logNodeAdd(&memberNode->addr, &joinedAddr);
}

void MP1Node::handleGOSSIP(MessageHdr* gossipMessage) {
    MemberListEntry fromAddressasEntry = toMemberListEntry(gossipMessage->fromAddress);
    for(int j = 0; j < memberNode->memberList.size(); j++) {
        if(fromAddressasEntry.id == memberNode->memberList[j].id && fromAddressasEntry.port == memberNode->memberList[j].port) {
            memberNode->memberList[j].heartbeat++;
            memberNode->memberList[j].timestamp = par->globaltime;
            break;
        }
    }

    vector<MemberListEntry> gossipedList = *((vector<MemberListEntry>*) gossipMessage->data);

    for(int i = 0; i < gossipedList.size(); i++) {
        MemberListEntry gossipedEntry = gossipedList[i];
        MemberListEntry myAddressAsEntry = toMemberListEntry(memberNode->addr);
        bool isMe = gossipedEntry.id == myAddressAsEntry.id && gossipedEntry.port == myAddressAsEntry.port;

        if(!isMe) {
            bool found = false;
            int j;
            int elapsed = par->globaltime - gossipedEntry.timestamp;
            if(elapsed <= TFAIL) {
                for(j = 0; j < memberNode->memberList.size(); j++) {
                    if(gossipedEntry.id == memberNode->memberList[j].id && gossipedEntry.port == memberNode->memberList[j].port) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    gossipedEntry.timestamp = par->globaltime;
                    memberNode->memberList.push_back(gossipedEntry);
                    Address newAddress = toAddress(gossipedEntry);
                    log->logNodeAdd(&memberNode->addr, &newAddress);
                } else if(gossipedEntry.heartbeat > memberNode->memberList[j].heartbeat) {
                    memberNode->memberList[j].heartbeat = gossipedEntry.heartbeat;
                memberNode->memberList[j].timestamp = par->globaltime;
            }
            }
        }
    }
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {
	/*
	 * Your code goes here
	 */

    gossipMemberList();
    removeFailed();
}

void MP1Node::gossipMemberList() {
    if(par->globaltime % GOSSIP_TIME == 0 && !memberNode->memberList.empty()) {
        for (int i = 0; i < GOSSIP_FAN_OUT; i++) {
            int randomIndex = (int) rand() % memberNode->memberList.size();
            MemberListEntry entry = memberNode->memberList[randomIndex];
            Address dest = toAddress(entry);
            MessageHdr gossipMessage;
            gossipMessage.msgType = GOSSIP;
            gossipMessage.fromAddress = memberNode->addr;
            gossipMessage.data = &memberNode->memberList;
            emulNet->ENsend(&memberNode->addr, &dest, (char*) &gossipMessage, sizeof(gossipMessage));
        }
    }
}

void MP1Node::removeFailed() {
    for(int j = 0; j < memberNode->memberList.size(); j++) {
        MemberListEntry entry = memberNode->memberList[j];
        int elapsed = par->globaltime - entry.gettimestamp();
        if(elapsed > TREMOVE) {
            Address removedAddress;
            removedAddress.addr[0] = entry.getid();
            removedAddress.addr[1] = 0;
            removedAddress.addr[2] = 0;
            removedAddress.addr[3] = 0;
            removedAddress.addr[4] = entry.getport();
            memberNode->memberList.erase(memberNode->memberList.begin() + j);
            log->logNodeRemove(&memberNode->addr, &removedAddress);
        }
    }
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
