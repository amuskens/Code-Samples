/*
 * BiDirectionalMailbox.cpp
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
 *  Created on: Nov 7, 2014
 *      Author: Anders Muskens
 */

#include "BiDirectionalMailbox.h"

/**
    Bidirectional Mailbox Constructor

    @param size_in the size of the mailbox (number of envelopes it can hold)

*/
BiDirectionalMailbox::BiDirectionalMailbox(unsigned int size_in)
{
	// Initialize TI-RTOS mailbox with default parameters and the size
	Mailbox_Params_init(&mbxParams);
	size = size_in;
	Mbx_Server = Mailbox_create(sizeof(Envelope), size, &mbxParams, NULL);
}

BiDirectionalMailbox::~BiDirectionalMailbox()
{
	// TODO Auto-generated destructor stub
}

/**
    Initialize a client with a given id. 

    @param size_in the size of the mailbox (number of envelopes it can hold)

*/
void BiDirectionalMailbox::InitClient(int id)
{
	Mbx_client[id] = Mailbox_create(sizeof(Envelope), size, &mbxParams, NULL);
}

/**
    Run on the server task - initializes the server pend. Blocks the task until a 
	message is received.
    @param msg pointer to a pointer to a mailbox message object which will point to the received message.
	@return true when a message is received. 
*/
bool BiDirectionalMailbox::ServerPend(MailboxMsg** msg)
{
	Envelope e;
	Mailbox_pend(Mbx_Server, &e, BIOS_WAIT_FOREVER);
	*msg = e.msg;	// copy the message into 
	return true;
}
/**
    Run in the cleint task - wait until a request from the server is received. Blocks the task.
    @param id_to_oend the ID of the client mailbox
	@param msg pointer to a MailboxMsg pointer which will point to the received message.
	@return true when a message is received. 
*/
bool BiDirectionalMailbox::ClientPend(
		int id_to_pend,
		MailboxMsg** msg
		)
{
	Envelope e;
	if(Mailbox_pend(Mbx_client[id_to_pend], &e, 1000))
	{
		*msg = e.msg;
		return true;
	}
	else
	{
		return false;
	}

}
/**
    Run on the client task - sends a request to the server, and waits for the response. 
    @param msg_to_send pointer to a MailboxMsg to send to the server.
	@param msg_response pointer to a MailboxMsg pointer which will point to the received message.
	@return true when a message is received. False if a timeout occurs. 
*/
bool BiDirectionalMailbox::ClientRequestAndWaitForResponse(
		int id_to_pend,
		MailboxMsg* msg_to_send,
		MailboxMsg** msg_response
		)
{
	Envelope e;
	e.msg = msg_to_send;
	if(!Mailbox_post(Mbx_Server , &e, TIMEOUT))
		return false;
	if(ClientPend(id_to_pend,msg_response))	// wait until received
		return true;
	else
		return false;
}
/**
    Run on the client task - sends a request to the server, but does not wait for any response.
    @param msg_to_send pointer to a MailboxMsg to send to the server.
	@return true always.
*/
bool BiDirectionalMailbox::ClientRequestNoWait(
		int id_to_pend,
		MailboxMsg* msg_to_send
		)
{
	Envelope e;
	e.msg = msg_to_send;
	Mailbox_post(Mbx_Server , &e, BIOS_WAIT_FOREVER);
	return true;
}
/**
    Run on the server task - sends a request to the server, and waits for the response. 
    @param id the id of the client
	@param msg_to_send pointer to the message to send in response to the client. 
	@return true
*/
bool BiDirectionalMailbox::ServerRespondToClient(
		int id,
		MailboxMsg* msg_to_send
		)
{
	Envelope e;
	e.msg = msg_to_send;
	Mailbox_post(Mbx_client[id] , &e, BIOS_WAIT_FOREVER);
	return true;
}

/**
    Helper function for a client task. Posts a message to server, and pends for a response
    @param mbx pointer to the bidirectional mailbox object. 
	@param msg message to send to the server
	@param received_msg pointer to a MailboxMsg pointer which will point to the received message. 
	@return the status of the success of the operation. 
*/
bool send_msg_wait_response(BiDirectionalMailbox* mbx, MailboxMsg* msg,MailboxMsg** received_msg)
{
	return mbx->ClientRequestAndWaitForResponse(msg->GetOrigin(),msg,received_msg);
}

/**
    Helper function for a client task. Posts a message to the server, but does not wait for a response. 
    @param mbx pointer to the bidirectional mailbox object. 
	@param msg message to send to the server
	@return the status of the success of the operation. 
*/
Void send_msg_no_wait(BiDirectionalMailbox* mbx, MailboxMsg* msg)
{
	mbx->ClientRequestNoWait(msg->GetOrigin(),msg);
}
