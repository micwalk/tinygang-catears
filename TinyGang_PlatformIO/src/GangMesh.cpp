#include "GangMesh.h"

void GangMesh::setup() {
	pinMode(STATUS_LED, OUTPUT);

	mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

	mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
	//Bind callbacks. Since they are class methods now instead of free functions,
	// we must bind this to them explicitly using std::bind.
	//The ones that take arguments must have std::placeholders added for them
	mesh.onReceive(std::bind(&GangMesh::receivedCallback, this, std::placeholders::_1, std::placeholders::_2));
	mesh.onNewConnection(std::bind(&GangMesh::newConnectionCallback, this, std::placeholders::_1) );
	mesh.onChangedConnections(std::bind(&GangMesh::changedConnectionCallback, this) );
	mesh.onNodeTimeAdjusted(std::bind(&GangMesh::nodeTimeAdjustedCallback, this, std::placeholders::_1) );
	mesh.onNodeDelayReceived(std::bind(&GangMesh::delayReceivedCallback, this, std::placeholders::_1, std::placeholders::_2) );

	auto nodeId = mesh.getNodeId();
	Serial.printf("*Initializing mesh. Own NodeId is %u\n*", nodeId);

	userScheduler.addTask(taskCalculateDelay);
	taskCalculateDelay.enable();

	blinkNoNodes.set(BLINK_PERIOD, getNodeCount() * 2, [this]() {
        // If on, switch off, else switch on
        this->meshStatusLightOn = !this->meshStatusLightOn;
        blinkNoNodes.delay(BLINK_DURATION);

        if (blinkNoNodes.isLastIteration()) {
            // Finished blinking. Reset task for next run
            // blink number of nodes (including this node) times
            blinkNoNodes.setIterations(this->getNodeCount() * 2);
            // Calculate delay based on current mesh time and BLINK_PERIOD
            // This results in blinks between nodes being synced
            blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                        (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
        } });
	userScheduler.addTask(blinkNoNodes);
	blinkNoNodes.enable();

	randomSeed(analogRead(A0));
}

void GangMesh::receivedCallback(uint32_t from, String &msg) {
	Serial.printf("%u: painlessMesh: Received from %u msg=%s\n", millis(), from, msg.c_str());
	
	// Deserialize message, don't forget to check for error return value
	SerializedNodeData sPattern(msg[0]);
	SharedNodeData deserializedData = DeserializeNodeData(sPattern);
	if(!deserializedData.isValid()) {
		Serial.printf("%u: Node %u has just sent an invalid message. Ignoring.\n", millis(), from);
		return;
	}
	
	//Lookup/add/update node data.
	auto foundIter = std::find(m_allNodeList.begin(), m_allNodeList.end(), from);
	if(foundIter == m_allNodeList.end()) {
		Serial.printf("%u: Haven't seen node %u before. Adding to list.\n", millis(), from);
		m_allNodeList.push_back(from);
		m_allNodeData.push_back(deserializedData);
	} else {
		int dataIndex = std::distance(m_allNodeList.begin(), foundIter);
		
		//OMG this doesn't work because simple list is a linked list.
		//TODO: switch to ustl...
		//SharedNodeData& oldNodeData = m_allNodeData[dataIndex];
		
		// m_allNodeData[dataIndex] = deserializedData;
		Serial.printf("%u: Have seen seen node %u before. It is at position %i/%u\n", millis(), from, 
			dataIndex, m_allNodeData.size());
	}
	
	receiveMeshMsg(from, deserializedData);
}

// Resets the blink task.
void GangMesh::resetBlinkTask() {
	meshStatusLightOn = false;
	blinkNoNodes.setIterations(getNodeCount() * 2);
	blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
}

// Note this ONLY fires for direct connections
// Therefore it's not super useful for TinyGang.
void GangMesh::newConnectionCallback(uint32_t nodeId) {
	Serial.printf("%u: --> painlessMesh: New Connection, nodeId = %u\n", millis(), nodeId);
	Serial.printf("%u: --> painlessMesh: New Connection, %s\n", millis(), mesh.subConnectionJson(true).c_str());

	resetBlinkTask(); //This isn't
}

// This fires for any change in mesh topology, including dropping nodes and transitive connection changes
// This will run AFTER newConnectionCallback
void GangMesh::changedConnectionCallback() {
	Serial.printf("%u: --> painlessMesh: Changed connections\n", millis());
	resetBlinkTask();

	//Re-linearalize node list
	m_currentNodeList = mesh.getNodeList();
	// m_currentNodeList.sort(); //Need to add self to this!
		
	Serial.printf("Num nodes: %d\n", m_currentNodeList.size());
	Serial.printf("Connection list:");

	SimpleList<uint32_t>::iterator node = m_currentNodeList.begin();
	while (node != m_currentNodeList.end()) {
		Serial.printf(" %u", *node);
		node++;
	}
	Serial.println();
	calc_delay = true;
	
}


// Callback for delay calculation task.
// Might be useful for location finding? Delay is probably more a fn of latency between hops though.
void GangMesh::sendDelayMessage() {
	// String msg = "Hello from node ";
	// msg += mesh.getNodeId();
	// msg += " myFreeMemory: " + String(ESP.getFreeHeap());
	// Serial.printf("Sending message: %s\n", msg.c_str());
	// mesh.sendBroadcast(msg);

	if (calc_delay) {
		Serial.printf("%u: TaskSendMessage launching delay calculation\n", millis());

		SimpleList<uint32_t>::iterator node = m_currentNodeList.begin();
		while (node != m_currentNodeList.end()) {
			mesh.startDelayMeas(*node);
			node++;
		}
		calc_delay = false;
	}

	taskCalculateDelay.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
}

void GangMesh::delayReceivedCallback(uint32_t from, int32_t delay) {
	Serial.printf("Delay to node %u is %d us\n", from, delay);
}

// Whenever we update the mesh time.
// TODO: re-schedule lights when this happens!
void GangMesh::nodeTimeAdjustedCallback(int32_t offset) {
	Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}