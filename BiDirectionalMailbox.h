/*
 * BiDirectionalMailbox.h
 *
 * BiDirectional Mmailbox class for TI-RTOS
 * This class creates a client-server functionality that can run between two disparate
 * TI-RTOS threads. It allows the request and receipt of data between two threads 
 * in a non-blocking way.
 *
 * Calling sequence:
 * ServerPend [ in server task]
 * ClientRequestAndWaitForResponse [client is now waiting]
 * [ server receives message and stops pending]
 * ServerRespondToClient
 * [Client receives message and stops pending]
 *
 * Each client will be assigned a unique ID so the server knows which task to 
 * respond to.
 *
 *  Created on: Nov 7, 2014
 *      Author: Anders Muskens
 */

#ifndef BIDIRECTIONALMAILBOX_H_
#define BIDIRECTIONALMAILBOX_H_

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Mailbox.h>

#include <map>
#include "MsgObj.h"

using namespace std;

const int TIMEOUT = 1000;

/**
    Envelope structure - structure which contians a pointer to the message data.
*/
struct Envelope
{
	MailboxMsg* msg;
};

class BiDirectionalMailbox {

public:

	BiDirectionalMailbox(unsigned int size_in);
	virtual ~BiDirectionalMailbox();

	void InitClient(int);
	bool ClientRequestAndWaitForResponse(int,MailboxMsg*,MailboxMsg**);
	bool ServerPend(MailboxMsg**);
	bool ServerRespondToClient(int,MailboxMsg*);
	bool ClientRequestNoWait(int id_to_pend,MailboxMsg* msg_to_send);

protected:
	Mailbox_Handle Mbx_Server;
	map<int,Mailbox_Handle> Mbx_client;

	Mailbox_Params   mbxParams;
	int size;

	bool ClientPend(int,MailboxMsg**);
};

bool send_msg_wait_response(BiDirectionalMailbox* mbx, MailboxMsg* msg,MailboxMsg** received_msg);
Void send_msg_no_wait(BiDirectionalMailbox* mbx, MailboxMsg* msg);


#endif /* BIDIRECTIONALMAILBOX_H_ */
