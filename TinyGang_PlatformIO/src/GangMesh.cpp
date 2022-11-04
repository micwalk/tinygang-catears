#include "GangMesh.h"

void GangMesh::setup() {
	pinMode(STATUS_LED, OUTPUT);

	mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

	mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
	mesh.onReceive(std::bind(&GangMesh::receivedCallback, this, std::placeholders::_1, std::placeholders::_2));

	// mesh.onNewConnection(std::bind(&GangMesh::newConnectionCallback, this) );
	// mesh.onChangedConnections(std::bind(&GangMesh::changedConnectionCallback, this) );
	// mesh.onNodeTimeAdjusted(std::bind(&GangMesh::nodeTimeAdjustedCallback, this) );
	// mesh.onNodeDelayReceived(std::bind(&GangMesh::delayReceivedCallback, this) );

	auto nodeId = mesh.getNodeId();
	Serial.printf("*Initializing mesh. Own NodeId is %u\n*", nodeId);

	userScheduler.addTask(taskCalculateDelay);
	taskCalculateDelay.enable();

	blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, [this]() {
        // If on, switch off, else switch on
        this->meshStatusLightOn = !this->meshStatusLightOn;
        blinkNoNodes.delay(BLINK_DURATION);

        if (blinkNoNodes.isLastIteration()) {
            // Finished blinking. Reset task for next run
            // blink number of nodes (including this node) times
            blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
            // Calculate delay based on current mesh time and BLINK_PERIOD
            // This results in blinks between nodes being synced
            blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                        (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
        } });
	userScheduler.addTask(blinkNoNodes);
	blinkNoNodes.enable();

	randomSeed(analogRead(A0));
}

void GangMesh::sendDelayMessage() {
	// String msg = "Hello from node ";
	// msg += mesh.getNodeId();
	// msg += " myFreeMemory: " + String(ESP.getFreeHeap());
	// Serial.printf("Sending message: %s\n", msg.c_str());
	// mesh.sendBroadcast(msg);

	if (calc_delay) {
		Serial.printf("%u: TaskSendMessage launching delay calculation\n", millis());

		SimpleList<uint32_t>::iterator node = nodes.begin();
		while (node != nodes.end()) {
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

void GangMesh::receivedCallback(uint32_t from, String &msg) {
	Serial.printf("%u: painlessMesh: Received from %u msg=%s\n", millis(), from, msg.c_str());
	receiveMeshMsg(msg[0]);
}

// Resets the blink task.
void GangMesh::resetBlinkTask() {
	meshStatusLightOn = false;
	blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
	blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
}

void GangMesh::newConnectionCallback(uint32_t nodeId) {
	Serial.printf("%u: --> painlessMesh: New Connection, nodeId = %u\n", millis(), nodeId);
	Serial.printf("%u: --> painlessMesh: New Connection, %s\n", millis(), mesh.subConnectionJson(true).c_str());

	resetBlinkTask();
}

void GangMesh::nodeTimeAdjustedCallback(int32_t offset) {
	Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}
void GangMesh::changedConnectionCallback() {
	Serial.printf("painlessMesh: Changed connections\n");
	resetBlinkTask();

	nodes = mesh.getNodeList();
	Serial.printf("Num nodes: %d\n", nodes.size());
	Serial.printf("Connection list:");

	SimpleList<uint32_t>::iterator node = nodes.begin();
	while (node != nodes.end()) {
		Serial.printf(" %u", *node);
		node++;
	}
	Serial.println();
	calc_delay = true;
}