#ifndef __MESHSTUFF__H__
#define __MESHSTUFF__H__

#include "UserConfig.h"
#include "PatternSerialization.h"

// TODO: Make this a proper header that be included in multiple cpp files without
// the compiler freaking out

//************************************************************
// this is a simple example that uses the easyMesh library
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 4. prints anything it receives to Serial.print
//
//
//************************************************************
#include <painlessMesh.h>

#define BLINK_PERIOD 3000   // milliseconds until cycle repeat
#define BLINK_DURATION 100  // milliseconds LED is on for

// Prototypes For painlessMesh

// Declared but not defined here.
void receiveMeshMsg(uint32_t nodeName, SharedNodeData nodeData);

class GangMesh {
   public:
	painlessMesh mesh;
	Scheduler userScheduler;  // to control your personal task
	SimpleList<uint32_t> m_currentNodeList;
	
	SimpleList<uint32_t> m_allNodeList;
	SimpleList<SharedNodeData> m_allNodeData;

	const SimpleList<uint32_t>& getNodeList() const{
		return m_currentNodeList;
	}
	
	// Number of nodes in mesh, including self.
	std::size_t getNodeCount() const {
		return m_currentNodeList.size() + 1;
	}
	
   private:
	// Task Config:
	// Task to periodically calculate delay
	void sendDelayMessage();
	Task taskCalculateDelay;  //(TASK_SECOND * 1, TASK_FOREVER, &sendDelayMessage);  // start with a one second interval
	bool calc_delay = false;

	// Task to blink the number of mesh nodes
	Task blinkNoNodes;
	bool meshStatusLightOn = false;

   public:
	GangMesh() : taskCalculateDelay(TASK_SECOND * 1, TASK_FOREVER,
	                                std::bind(&GangMesh::sendDelayMessage, this)) {
		// Nothing to do here
	}

	void setup();
	void update() {
		mesh.update();
		digitalWrite(STATUS_LED, !meshStatusLightOn);
	}

	void sendBroadcast(String message) {
		mesh.sendBroadcast(message);
	}

	void receivedCallback(uint32_t from, String &msg);
	void newConnectionCallback(uint32_t nodeId);
	void changedConnectionCallback();
	void nodeTimeAdjustedCallback(int32_t offset);
	void delayReceivedCallback(uint32_t from, int32_t delay);

   private:
	void resetBlinkTask();
};

#endif  //!__MESHSTUFF__H__