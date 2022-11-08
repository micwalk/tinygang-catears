#include "GangMesh.h"

void GangMesh::setup() {
	Serial.println("******GangMesh Setup******");
	
	pinMode(PIN_LED_STATUS, OUTPUT);
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
	Serial.printf("  Own NodeId is %u\n", nodeId);
	changedConnectionCallback();

	userScheduler.addTask(taskCalculateDelay);
	taskCalculateDelay.enable();
	
	meshStatusLightOn = false;	
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
	Serial.println("\n******GangMesh Setup finish******");
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
	
	int fromIdx = m_nodeData.find(from);
	if(fromIdx < 0) {
		Serial.printf("%u: Haven't seen node %u before. Adding to list.\n", millis(), from);
		m_nodeData[from] = deserializedData;
	} else {
		// m_allNodeData[dataIndex] = deserializedData;
		Serial.printf("%u: Have seen seen node %u before. It is at position %i/%u\n", millis(), from, 
			fromIdx, m_nodeData.length());
		auto oldData = m_nodeData.values[fromIdx];
		Serial.printf("   Old Data Pattern: %i", oldData.nodePattern);
		m_nodeData.values[fromIdx] = deserializedData;
		Serial.printf("   New Data Pattern: %i\n", m_nodeData.values[fromIdx].nodePattern);		
		
		if(oldData.nodePattern != deserializedData.nodePattern){
			rescheduleLightsCallbackMain();
		}
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
	m_currentNodeList = mesh.getNodeList(true);
	m_currentNodeList.sort();
	
	Serial.printf("Num nodes: %d\n", m_currentNodeList.size());
	Serial.printf("Connection list:");

	SimpleList<uint32_t>::iterator node = m_currentNodeList.begin();
	while (node != m_currentNodeList.end()) {
		Serial.printf(" %u", *node);
		node++;
	}
	Serial.println();
	calc_delay = true;
	
	rescheduleLightsCallbackMain();
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
	m_meshTimeOffset = mesh.getNodeTime() - micros();
	
	Serial.printf("Adjusted time %u. Offset = %d. MyOffset: %u\n", mesh.getNodeTime(), offset, m_meshTimeOffset);
	
	rescheduleLightsCallbackMain();
}
