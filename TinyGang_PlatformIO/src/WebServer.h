#ifndef __WEBSERVER__H__
#define __WEBSERVER__H__

//************************************************************
// this is a simple example that uses the painlessMesh library to
// connect to a another network and broadcast message from a webpage to the edges of the mesh network.
// This sketch can be extended further using all the abilities of the AsyncWebserver library (WS, events, ...)
// for more details
// https://gitlab.com/painlessMesh/painlessMesh/wikis/bridge-between-mesh-and-another-network
// for more details about my version
// https://gitlab.com/Assassynv__V/painlessMesh
// and for more details about the AsyncWebserver library
// https://github.com/me-no-dev/ESPAsyncWebServer
//************************************************************

#include "IPAddress.h"
#include "painlessMesh.h"

#ifdef ESP8266
#include <ESPAsyncTCP.h>

#include "Hash.h"
#else
#include <AsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>

// For SPIFFS
#include <FS.h>

// Prototype
AsyncWebServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";

// Assign output variables to GPIO pins
const int output26 = 26;
const int output27 = 27;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

#include "StringBuilder.h"

String serveSomething() {
	StringBuilder sb;

	sb.concat(R"(
<!DOCTYPE html><html>
<head><meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
.button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;
text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}
.button2 {background-color: #555555;}</style></head>
<body>
<h1>Glitter Cats!</h1>
<h2>Ears: Fancy_v3</h2>

<p>Current Pattern: 3</p>
<p><a href="/27/on"><button class="button">ON</button></a></p>
<p>Current Hue: 244</p>
</body></html>
)");
	// Display the HTML web page

	// // Display current state, and ON/OFF buttons for GPIO 27
	// client.println("<p>GPIO 27 - State " + output27State + "</p>");
	// // If the output27State is off, it displays the ON button
	// if (output27State == "off") {
	// 	client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
	// } else {
	// 	client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
	// }
	// client.println("");
	auto len = sb.length();
	String bob((char *)sb.string());
	return bob;
}

void logMemory() {
	log_i("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());
}

void runPsramTest() {
	logMemory();
	byte *psdRamBuffer = (byte *)ps_malloc(500000);
	logMemory();
	free(psdRamBuffer);
	logMemory();
}

String getContentType(String filename);  // convert the file extension to the MIME type
bool handleFileRead(String path);        // send the right file to the client (if it exists)

String getContentType(String filename) {  // convert the file extension to the MIME type
	if (filename.endsWith(".htm"))
		return "text/html";
	else if (filename.endsWith(".html"))
		return "text/html";
	else if (filename.endsWith(".css"))
		return "text/css";
	else if (filename.endsWith(".js"))
		return "application/javascript";
	else if (filename.endsWith(".png"))
		return "image/png";
	else if (filename.endsWith(".gif"))
		return "image/gif";
	else if (filename.endsWith(".jpg"))
		return "image/jpeg";
	else if (filename.endsWith(".ico"))
		return "image/x-icon";
	else if (filename.endsWith(".xml"))
		return "text/xml";
	else if (filename.endsWith(".pdf"))
		return "application/x-pdf";
	else if (filename.endsWith(".zip"))
		return "application/x-zip";
	else if (filename.endsWith(".gz"))
		return "application/x-gzip";
	return "text/plain";
}

bool handleFileRead(AsyncWebServerRequest *request) {  // send the right file to the client (if it exists)
	
    // extract the filename from request->url()
    String path = request->url();
    if (path.endsWith("/")) path += "index.html";            // If a folder is requested, send the index file
    int fnsstart = path.lastIndexOf('/');
    String fileName = path.substring(fnsstart);
    
	Serial.println("handleFileRead: " + path);
	
	
	if (SPIFFS.exists(fileName)) {                               // If the file exists
        String contentType = getContentType(fileName);               // Get the MIME type
        request->send(SPIFFS, fileName, contentType, false);
		return true;
	}
	Serial.println("\tFile Not Found");
	return false;  // If the file doesn't exist, return false
}

void serverSetup() {
	WiFi.setHostname("YeetBoi");

	runPsramTest();

	log_d("start SPIFFS");
	SPIFFS.begin();  // Start the SPI Flash Files System
	log_d("SPIFFS ready");

	server.on("/hi", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "text/plain", "Hello, world");
	});

	server.on("/thing", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "text/html", serveSomething());
	});

	// Async webserver
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "text/html", "<form>Text to Broadcast<br><input type='text' name='BROADCAST'><br><br><input type='submit' value='Submit'></form>");
		if (request->hasArg("BROADCAST")) {
			String msg = request->arg("BROADCAST");
			Serial.printf("\n*****\nWEB MSG:\n%s\n*****\n\n", msg);
		}
	});

	server.onNotFound([](AsyncWebServerRequest *request) {
		// send it if it exists
		if (!handleFileRead(request)) {
            // otherwise, respond with a 404 (Not Found) error
			request->send(404, "text/plain", "404: Not Found");
		}
	});

	server.begin();
}

#endif  //!__WEBSERVER__H__