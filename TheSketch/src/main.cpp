//#define FASTLED_ESP8266_D1_PIN_ORDER
#include <FastLED.h>
#include <ArduinoJson.h>
// #include "FS.h"


#include "mypixels.h"

#define USE_DOTSTAR 1
// #define USE_NEOPIXEL 1

#define CLOCK_PIN     D5
#define NUM_LEDS      72
#define DATA_PIN    D7

#if defined(USE_DOTSTAR)
#define CHIPSET     SK9822
#define COLOR_ORDER BGR
#endif

#if defined(USE_NEOPIXEL)
#define CHIPSET     NEOPIXEL
#define COLOR_ORDER GRB
#endif

bool gReverseDirection = false;

CRGB leds[NUM_LEDS];

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <WebSocketsServer.h>
#include <WebSocketsClient.h>

WebSocketsServer webSocketServer = WebSocketsServer(81);
WebSocketsClient webSocketClient;

IPAddress master_IP(42,42,42,42);
IPAddress gateway(42,42,42,40);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

#define POI_MODE_OFF 0
#define POI_MODE_STANDBY 1
#define POI_MODE_JSONCONROL 2
#define POI_MODE_IMG 3
#define POI_MODE_FIRE 4
#define POI_MODE_PATTERN 5

uint poimode = POI_MODE_STANDBY;
uint dynSPARKLING = 10;
uint showimg = 0;
uint imagelines = 0;
uint imageline = 0;



boolean ismaster = false;

JsonVariant currcommand;
DynamicJsonDocument currcommanddoc(5000);

//returns true if element is finished
CRGB templeds[NUM_LEDS];
boolean processPoiCommands(JsonVariant jdata, uint level, uint seqcnt){

  JsonVariant currjdata;

  // check repeats, return false and reset counter if limit reached
  currjdata = jdata["r_i"];
  if(!currjdata.isNull()){
    JsonVariant current_r = jdata["_r"];
    if(current_r.isNull()){
      jdata["_r"] = 0;
    }

    if(jdata["_r"].as<long>() >= currjdata.as<long>()){
      jdata["_r"] = 0;
      // jdata.remove("_r");
      return true;
    }
    // jdata["_r"] = current_r.as<long>()+1;  //moved to img
  }


  // check time, return false and reset starttime if limit reached
  currjdata = jdata["r_t"];
  if(!currjdata.isNull()){
    JsonVariant current_r = jdata["_r"];
    if(current_r.as<long>() == 0){
      jdata["_r"] = millis();
    }

    if(millis()-jdata["_r"].as<long>() >= currjdata.as<long>()){
      jdata["_r"] = 0;
      // jdata.remove("_r");
      return true;
    }
  }

  //process if no limits reached

  //process sequence
  currjdata = jdata["seq"];
  if(!currjdata.isNull()){

    JsonVariant current_r = jdata["_i"];
    if(current_r.isNull()){
      jdata["_i"] = 0;
    }
    JsonVariant seqelem = currjdata[jdata["_i"]];
    if(processPoiCommands(seqelem,level+1,jdata["_i"].as<long>())){
      jdata["_i"] = jdata["_i"].as<long>()+1;
      if(jdata["_i"].as<long>() == currjdata.as<JsonArray>().size()){ //last element reached
        jdata["_i"] = 0;
        // jdata.remove("_i");
        return true;
      }
      Serial.printf("\nseq %u:%u =>", level, jdata["_i"].as<long>());
    }
    return false;
  }

  //process mix
  
  currjdata = jdata["mix"];
  if(!currjdata.isNull()){
    //loop mix array
    //always mix (add, multiply,..) leds-array to temparray, copy to leds in the end
    //
  }

  //process image
  currjdata = jdata["id"];
  if(!currjdata.isNull()){

    
    JsonVariant current_l = jdata["_l"];
    if(current_l.as<long>()==0){
      jdata["_l"] = 0;
    }
    
    //fill pixels
    for( uint j = 0; j < 72; j++) {     
       leds[j] = palette[pgm_read_word(&(img_table[jdata["id"].as<long>()][ jdata["_l"].as<long>()*72+j]))];
    }

    jdata["_l"] = jdata["_l"].as<long>()+1;
    if(jdata["_l"].as<long>() >= imglengths[jdata["id"].as<long>()]){ //iteration done
      jdata["_l"] = 0;
      // jdata.remove("_l");
      jdata["_r"] = jdata["_r"].as<long>()+1;
      
      if(jdata["r_i"].as<long>() == jdata["_r"].as<long>()){ // required iterations done
        jdata["_r"] =0;
        // jdata.remove("_r");
        return true;
      }
    }
    return false;
  }
  

  //process pattern
  currjdata = jdata["patid"];
  if(!currjdata.isNull()){
    // Fire2012();
    // xx.printf("p%u:%u.", currjdata.as<int>(), jdata["_r"]);
    //some patterns need last pixels must be serialized an written to jdatas
    return false;
  }
  return false;
}



void broadcastCurrCommandIfMaster(){
  if(ismaster){
    Serial.println("i am master: broadcast");
    String json;
    serializeJson(currcommanddoc, json);
    Serial.println(json);
		webSocketServer.broadcastTXT(json);
  }
}
void processJsonRequest(String json){
  deserializeJson(currcommanddoc, json);
  if(currcommanddoc.isNull()){
    Serial.println("doc null");
    return;
  }
  Serial.println("doc ok");
  currcommand = currcommanddoc.as<JsonVariant>();
  broadcastCurrCommandIfMaster();
  JsonVariant jdata;



  jdata = currcommanddoc["seq"];
  if(!jdata.isNull()){
    poimode = POI_MODE_JSONCONROL;
  }

  jdata = currcommanddoc["spark"];
  if(!jdata.isNull()){
    poimode = POI_MODE_FIRE;
    dynSPARKLING = jdata.as<int>();
    Serial.println("spark: "+dynSPARKLING);
  }
  
  jdata = currcommanddoc["img"];
  if(!jdata.isNull()){
    poimode = POI_MODE_IMG;
    showimg = jdata.as<int>();
    imagelines = imglengths[showimg];
    imageline = 0;
    Serial.println("showimg: "+showimg);
  }

  jdata = currcommanddoc["pattern"];
  if(!jdata.isNull()){
    poimode = POI_MODE_PATTERN;

  }
  jdata = currcommanddoc["off"];
  if(!jdata.isNull()){
    poimode = POI_MODE_OFF;

  }jdata = currcommanddoc["standby"];
  if(!jdata.isNull()){
    poimode = POI_MODE_STANDBY;

  }

  
}

void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_TEXT:
      processJsonRequest((char*)payload);
			break;
    }

}

void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_CONNECTED:
            {
                broadcastCurrCommandIfMaster();
                webSocketServer.sendTXT(num, "Connected"); //for controller in browser
            }
            break;
        case WStype_TEXT:
            processJsonRequest((char*)payload);
            break;
    }

}



void handleRequest(){

  Serial.println("handleRequest ");
  if(server.hasArg("json")){
    Serial.println(server.arg("json"));
    processJsonRequest(server.arg("json"));
  }

  server.send(200, "text/html", myhtml);       //Response to the HTTP request
}



String listNetworks(String ssidsearchstr) {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    Serial.println("Couldn't get a wifi connection");
    while (true);
  }

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  String master_ssid  = "";
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    if(WiFi.SSID(thisNet).startsWith(ssidsearchstr)){
      master_ssid = WiFi.SSID(thisNet);
      Serial.println("MASTER:");
    }
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.println(" dBm");
  }

  return master_ssid;
}

uint indicator = 0;
void indicatorLed(uint32_t color){
  leds[indicator++] = color;FastLED.show();
}


void setup() {
    
  
  Serial.begin(9600);



  
  Serial.println("start");
  delay(500); // sanity delay
  //FSInfo fs_info;
  //SPIFFS.begin();
  // SPIFFS.format(); // for first usage
  // SPIFFS.begin();
  /* SPIFFS.info(fs_info);
  Serial.println("totalBytes: ");
  Serial.println(fs_info.totalBytes);
  Serial.println("usedBytes: ");
  Serial.println(fs_info.usedBytes);
  Serial.println("blockSize: ");
  Serial.println(fs_info.blockSize);
  Serial.println("pageSize: ");
  Serial.println(fs_info.pageSize);
  Serial.println("maxOpenFiles: ");
  Serial.println(fs_info.maxOpenFiles);
  Serial.println("maxPathLength: ");
  Serial.println(fs_info.maxPathLength); */
  #if defined(USE_DOTSTAR)
  FastLED.addLeds<CHIPSET, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  #endif

  #if defined(USE_NEOPIXEL)
  FastLED.addLeds<CHIPSET, DATA_PIN>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  #endif
  
  //FastLED.setBrightness( 255 );
  //FastLED.setMaxPowerInVoltsAndMilliamps(5,2000);
  indicatorLed(CRGB::BlueViolet);

  const char* ssidprefix = "bitpixelpoi_";

  String master_ssid = listNetworks(ssidprefix);
  
  indicatorLed(CRGB::BlueViolet);

  const char* password = "1234567890";

  // WiFi.persistent(false);

  
  if(master_ssid!=""){
    indicatorLed(CRGB::Blue);
    Serial.print("SSID: ");
    Serial.println(master_ssid);

    
    //WiFi.softAPdisconnect();
    // WiFi.disconnect();
    Serial.print("set mode to WIFI_STA");
    WiFi.mode(WIFI_STA);
    Serial.print("begin");
    WiFi.begin(master_ssid, password);
    indicatorLed(CRGB::Blue);

    uint maxtries = 20;

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
      indicatorLed(CRGB::Yellow);
      delay(500);
      Serial.print(".");
      maxtries--;
      if(maxtries<=0){
        break;
      }
    }
    Serial.println("");

    if (WiFi.status() == WL_CONNECTED){
      Serial.print("Connected to ");
      Serial.println(master_ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      Serial.println(WiFi.macAddress());

      
      // server address, port and URL
      webSocketClient.begin(master_IP.toString(), 81, "/");

      // event handler
      webSocketClient.onEvent(webSocketClientEvent);

      // use HTTP Basic Authorization this is optional remove if not needed
      // webSocketClient.setAuthorization("user", "Password");

      // try ever 5000 again if connection has failed
      webSocketClient.setReconnectInterval(500);
      
      // start heartbeat (optional)
      // ping server every 15000 ms
      // expect pong from server within 3000 ms
      // consider connection disconnected if pong is not received 2 times
      webSocketClient.enableHeartbeat(1000, 500, 1);
      
    
      indicatorLed(CRGB::LightGreen);
      delay(300);

    }else{
      ismaster = true;
    }

  }else{
    ismaster = true;
  }
  if(ismaster){
    indicatorLed(CRGB::Orange);
    Serial.println("");
    Serial.println("not connected to master.");
    Serial.println("Setting SoftAP Config");
    Serial.println(WiFi.softAPConfig(master_IP, gateway, subnet) ? "Ready" : "Failed");

    Serial.println("Setting SoftAP ...");
     
    // WiFi.softAPdisconnect();
    // WiFi.disconnect();
    WiFi.mode(WIFI_AP);

    String ap_sid = ssidprefix + WiFi.macAddress();
    ap_sid.replace(":","_");
    indicatorLed(CRGB::Orange);
    Serial.println(WiFi.softAP(ap_sid,password) ? "Ready" : "Failed");

    Serial.print("SoftAP IP =");
    Serial.println(WiFi.softAPIP());
    Serial.print("SoftAP SSID =");
    Serial.println(ap_sid);

    
    webSocketServer.begin();
    webSocketServer.onEvent(webSocketServerEvent);
    
    server.onNotFound(handleRequest); 
    server.begin();
    Serial.println("Server listening");
      indicatorLed(CRGB::DarkGreen);
    delay(300);
    
  }
}

////// FastLED Examples Start ------>


#define FRAMES_PER_SECOND  120

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))


void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}
void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}


void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}


// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  77

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
// #define SPARKING 180

void Fire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < dynSPARKLING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}
void sampleloop()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}

////// FastLED Examples End <------


uint offset=0;
uint last_offset=0;
uint32_t t = millis();
uint32_t last_t = t;
void loop()
{



    if (ismaster){
      //Webserver
      server.handleClient();
      webSocketServer.loop();
    }else{
      webSocketClient.loop();
    }
  
  //Performance Check
  /*  
  if(t-last_t>1000){
    Serial.println(offset-last_offset + "Hz");
    last_offset=offset;
    last_t=t;
  }
  t=millis();
  offset++;
   */

  if(poimode == POI_MODE_IMG){
   for( uint j = 0; j < 72; j++) {
     
       leds[j] = palette[pgm_read_word(&(img_table[showimg][imageline*72+j]))];
    }
    imageline++;
    imageline=imageline % imglengths[showimg];
  }else if(poimode == POI_MODE_FIRE){
    Fire2012();
  }else if(poimode == POI_MODE_PATTERN){
    sampleloop();
  }else if(poimode == POI_MODE_STANDBY){
    FastLED.clear();
    leds[0] = CRGB::LawnGreen;
  }else if(poimode == POI_MODE_JSONCONROL){
    if(!currcommand.isNull()){
      processPoiCommands(currcommand,0,1);
    }
  }else{
    FastLED.clear();
  }
  
  FastLED.show();
}