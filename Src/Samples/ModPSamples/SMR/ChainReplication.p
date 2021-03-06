/**
We are modelling the chain replication protocol 
**/

event predSucc : (pred: machine, succ:machine);
event update : (client:machine, kv: (key:int, value:int));
event query : (client:machine, key:int);
event responsetoquery : (client: machine, value : int);
event responsetoupdate;
event backwardAck : (seqmachine:int);
event forwardUpdate : (mess : (seqmachine:int, client:machine, kv: (key:int, value:int)), pred : machine);
event local;
event done;


machine ChainReplicationServer {
	var nextSeqmachine : int;
	var keyvalue : map[int, int];
	var history : seq[int];
	var isHead : bool;
	var isTail : bool;
	var succ : machine; //NULL for tail
	var pred : machine; //NULL for head
	var sent : seq[(seqmachine:int, client : machine, kv: (key:int, value:int))];
	var iter : int;
	var tempIndex : int;
	var removeIndex : int;
	
	fun InitPred (){
		pred = payload.pred;
		succ = payload.succ;
		raise(local);
	}
	
	fun SendPong (){
		send payload, CR_Pong;
	}
	
	start state Init {
		defer update, query, backwardAck, forwardUpdate, CR_Ping;
		entry {
			isHead = (payload as (isHead:bool, isTail:bool, smid:int)).isHead;
			isTail = (payload as (isHead:bool, isTail:bool, smid:int)).isTail;
			nextSeqmachine = 0;
		}
		on predSucc do InitPred;
		on local goto WaitForRequest;
	
	}
	fun becomeTailAction() {
		isTail = true;
		succ = this;
		//send all the events in sent to both client and the predecessor
		iter = 0;
		while(iter < sizeof(sent))
		{
			//invoke the monitor
			monitor UpdateResponse_QueryResponse_Seq, monitor_reponsetoupdate, (tail = this, key = sent[iter].kv.key, value = sent[iter].kv.value);
			
			//invoke livenessUpdatetoResponse(monitor_responseLiveness, (reqmachine = sent[iter].seqmachine, ));
			
			
			//send the response to client
			send sent[iter].client, responsetoupdate;
			
			// the backward ack to the pred
			send pred, backwardAck, (seqmachine = sent[iter].seqmachine, );
			iter = iter + 1;
		}
		
		send payload, tailChanged;
		
	}
	
	fun becomeHeadAction() {
		isHead = true;
		pred = this;
		
		send payload, headChanged;
		
	}
	
	fun updatePredecessor() {
		pred = payload.pred;
		if(sizeof(history) > 0)
		{
			if(sizeof(sent) > 0)
				send payload.master, newSuccInfo , (lastUpdateRec = history[sizeof(history) - 1], lastAckSent = sent[0].seqmachine);
			else
				send payload.master, newSuccInfo , (lastUpdateRec = history[sizeof(history) - 1], lastAckSent = history[sizeof(history) - 1]);
		}
		
	}
	
	fun updateSuccessor() {
		tempIndex = -1; //some large number
		succ = payload.succ;
		if(sizeof(sent) > 0)
		{
			//send the remaining history
			iter = 0;
			while(iter < sizeof(sent))
			{
				if(sent[iter].seqmachine > payload.lastUpdateRec)
					send succ, forwardUpdate, (mess = sent[iter], pred = this);
				
				iter = iter + 1;
			}
			
			//send the backward acks
			iter = sizeof(sent) - 1;
			while(iter >= 0)
			{
				if(sent[iter].seqmachine == payload.lastAckSent)
					tempIndex = iter;
				
				iter = iter - 1;
			}
			
			iter = 0;
			while(iter < tempIndex)
			{
				send pred, backwardAck, (seqmachine = sent[0].seqmachine, );
				sent -= (0);
				iter = iter + 1;
			}
		}
		
		send payload.master, success;
	}
	
	state WaitForRequest {
		
		/***********fault tolerance****************/
		on CR_Ping do SendPong;
		on becomeTail do becomeTailAction;
		on becomeHead do becomeHeadAction;
		on newPredecessor do updatePredecessor;
		on newSuccessor do updateSuccessor;
		/***************************/
		
		on update goto ProcessUpdate with 
		{
			nextSeqmachine = nextSeqmachine + 1;
			assert(isHead);
			//new livenessUpdatetoResponse(nextSeqmachine);
			//invoke livenessUpdatetoResponse(monitor_updateLiveness, (reqmachine = nextSeqmachine));
		};
		
		on query goto WaitForRequest with
		{
			assert(isTail);
			if(payload.key in keyvalue)
			{
				//Invoke the monitor
				monitor UpdateResponse_QueryResponse_Seq, monitor_responsetoquery, (tail = this, key = payload.key, value = keyvalue[payload.key]);
				
				send payload.client, responsetoquery, (client = payload.client,value = keyvalue[payload.key]);
			}
			else
			{
				send payload.client, responsetoquery, (client = payload.client, value = -1);
			}
		};
		on forwardUpdate goto ProcessfwdUpdate;
		on backwardAck goto ProcessAck;
	}
	state ProcessUpdate {
		entry {	
			//Add the update message to keyvalue store
			keyvalue[payload.kv.key] = payload.kv.value;
		
			//add it to the history seq (represents the successfully serviced requests)
			history += (sizeof(history), nextSeqmachine);
			//invoke the monitor
			monitor Update_Propagation_Invariant, monitor_history_update, (smid = this, history = history);
			
			//Add the update request to sent seq
			sent += (sizeof(sent), (seqmachine = nextSeqmachine, client = payload.client, kv = (key = payload.kv.key, value = payload.kv.value)));
			//call the monitor
			monitor Update_Propagation_Invariant, monitor_sent_update, (smid = this, sent = sent);
			//forward the update to the succ
			send succ, forwardUpdate, (mess = (seqmachine = nextSeqmachine, client = payload.client, kv = payload.kv), pred = this);
	
			raise(local);
		}
		on local goto WaitForRequest;
	}
	state ProcessfwdUpdate {
		entry {
			if(payload.pred == pred)
			{
				//update my nextSeqmachine
				nextSeqmachine = payload.mess.seqmachine;
				
				//Add the update message to keyvalue store
				keyvalue[payload.mess.kv.key] = payload.mess.kv.value;
				
				if(!isTail)
				{
					//add it to the history seq (represents the successfully serviced requests)
					history += (sizeof(history), payload.mess.seqmachine);
					//invoke the monitor
					monitor Update_Propagation_Invariant, monitor_history_update, (smid = this, history = history);
					//Add the update request to sent seq
					sent += (sizeof(sent), (seqmachine = payload.mess.seqmachine, client = payload.mess.client, kv = (key = payload.mess.kv.key, value = payload.mess.kv.value)));
					//call the monitor
					monitor Update_Propagation_Invariant, monitor_sent_update, (smid = this, sent = sent);
					//forward the update to the succ
					send succ, forwardUpdate, (mess = (seqmachine = payload.mess.seqmachine, client = payload.mess.client, kv = payload.mess.kv), pred = this);
				}
				else
				{
					if(!isHead)
					{
						//add it to the history seq (represents the successfully serviced requests)
						history += (sizeof(history), payload.mess.seqmachine);
					}
					
					//invoke the monitor
					monitor UpdateResponse_QueryResponse_Seq, monitor_reponsetoupdate, (tail = this, key = payload.mess.kv.key, value = payload.mess.kv.value);
					
					//invoke livenessUpdatetoResponse(monitor_responseLiveness, (reqmachine =payload.mess.seqmachine));
					
					
					//send the response to client
					send payload.mess.client, responsetoupdate;
					
					//send ack to the pred
					send pred, backwardAck, (seqmachine = payload.mess.seqmachine, );

				}
			}
			raise(local);
		}
		on local goto WaitForRequest;
	}
	
	state ProcessAck
	{
		entry {
			//remove the request from sent seq.
			RemoveItemFromSent(payload.seqmachine);
			
			if(!isHead)
			{
				//forward it back to the pred
				send pred, backwardAck, (seqmachine = payload.seqmachine,);
			}
			raise(local);
		}
		on local goto WaitForRequest;
	}
	
	fun RemoveItemFromSent(req : int) {
		removeIndex = -1;
		iter = sizeof(sent) - 1;
		while(iter >=0)
		{
			if(req == sent[iter].seqmachine)
				removeIndex = iter;
			
			iter = iter - 1;
		}
		
		if(removeIndex != -1)
		{
			sent -= removeIndex;
		}
	}
	
}






















