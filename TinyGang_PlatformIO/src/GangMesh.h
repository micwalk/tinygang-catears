#ifndef __MESHSTUFF__H__
#define __MESHSTUFF__H__

#include "PatternSerialization.h"
#include "config/UserConfig.h"
#include "ustd_map.h"

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
// Sorta like statically defined callbacks
void receiveMeshMsg(uint32_t nodeName, SharedNodeData nodeData);
void rescheduleLightsCallbackMain();

class GangMesh {
   public:
	painlessMesh mesh;
	Scheduler userScheduler;  // to control your personal task
	SimpleList<uint32_t> m_currentNodeList;

	ustd::map<uint32_t, SharedNodeData> m_nodeData;

	const SimpleList<uint32_t>& getNodeList() const {
		return m_currentNodeList;
	}

	// Number of nodes in mesh, including self.
	std::size_t getNodeCount() const {
		return m_currentNodeList.size();
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

	int32_t m_meshTimeOffset = 0;

	IPAddress m_localIp;
	IPAddress getlocalIP() {
		return IPAddress(mesh.getStationIP());
	}

   public:
	GangMesh() : taskCalculateDelay(TASK_SECOND * 1, TASK_FOREVER,
	                                std::bind(&GangMesh::sendDelayMessage, this)),
				 m_nodeData(2, MAX_PEERS, 2, false),
				 m_localIp(0, 0, 0, 0) {
		// Nothing to do here
	}

	// Initialie mesh
	void setup();

	// Run mesh update
	void update() {
		mesh.update();
		digitalWrite(STATUS_LED, !meshStatusLightOn);

		if (m_localIp != getlocalIP()) {
			m_localIp = getlocalIP();
			Serial.printf("\n*** GOT NEW LOCAL IP: %s ***\n", m_localIp.toString());
		}
	}

	// Wrapper for mesh.sendBroadcast
	void sendBroadcast(String message) {
		mesh.sendBroadcast(message);
	}

	void receivedCallback(uint32_t from, String& msg);
	void newConnectionCallback(uint32_t nodeId);
	void changedConnectionCallback();
	void nodeTimeAdjustedCallback(int32_t offset);
	void delayReceivedCallback(uint32_t from, int32_t delay);

	uint32_t TimeMeshToLocal(uint32_t meshMicros) {
		return meshMicros - m_meshTimeOffset;
	}

	uint32_t TimeLocalToMesh(uint32_t localMicros) {
		return localMicros + m_meshTimeOffset;
	}

   private:
	void resetBlinkTask();
};

#endif  //!__MESHSTUFF__H__