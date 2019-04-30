#include <AWS_IOT.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "config.h"

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 512

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1200       /* ESP32 will go to sleep for (x seconds) */

#define MAX_DEVICE_ID_LEN 20

gpio_num_t LED_PIN = GPIO_NUM_18;
gpio_num_t BTN_PIN = GPIO_NUM_15;
gpio_num_t LED_BUILTIN = GPIO_NUM_2;

AWS_IOT hornbill;

int status = WL_IDLE_STATUS;
int tick=0,msgCount=0,msgReceived = 0, rc = 0;

char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);

/*
void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
    strncpy(JsonDocumentBuffer,payLoad,sizeOfJsonDocumentBuffer);
    JsonDocumentBuffer[payloadLen] = 0;
    msgReceived = 1;
}
*/

jsonStruct_t ledHandler;
jsonStruct_t btnHandler;
jsonStruct_t awakeHandler;
jsonStruct_t stateHandler;

uint8_t builtInState = 0;
uint8_t ledState = 0;
uint8_t btnState = 0;
uint8_t awakeState = 1;

bool shouldUpdate = true;
bool canSleep = false;

uint8_t loopCount = 0;

char deviceId[MAX_DEVICE_ID_LEN];

void state_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
 
  StaticJsonDocument<200> doc;
  
  Serial.print("state_Callback, state: ");
  Serial.println(pJsonString);

  DeserializationError error = deserializeJson(doc, pJsonString);

  // Test if parsing succeeds.
  if (error) {
    Serial.println("parseObject() failed");
    Serial.println(error.c_str());
    return;
  }
  

  if (doc.containsKey("led") ) {
    bool do_led = doc["led"];
    Serial.print("Do led:");
    Serial.println(do_led);
    if (do_led != ledState) {
      ledState = do_led;
      digitalWrite(LED_PIN,!ledState); // sink to activate up LED , so reverse 
      shouldUpdate = true;
    };
  };
  if (doc.containsKey("awake") ) {
    bool do_awake = doc["awake"]; 
    Serial.print("Do awake:");
    Serial.println(do_awake);
    if (do_awake != awakeState) {
      awakeState = do_awake;
      shouldUpdate = true;
    };
  };
}


void led_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
  
  Serial.println("led_Callback");

  ledState= strncmp(pJsonString,"true",4) ? 0 : 1 ;
  digitalWrite(LED_PIN,!ledState); // sink to activate up LED , so reverse 
  Serial.print("State: ");
  Serial.println(ledState);
  shouldUpdate = true;
}


void awake_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
  
  Serial.println("awake_Callback");

  awakeState= strncmp(pJsonString,"true",4) ? 0 : 1 ;
   Serial.print("State: ");
  Serial.println(awakeState);
  shouldUpdate = true;
  canSleep = true;
}


void setup() {

    canSleep = false;
    loopCount = 0;
    Serial.begin(115200);

    pinMode(BTN_PIN,INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);

    pinMode(LED_BUILTIN,OUTPUT);
    pinMode(LED_PIN,OUTPUT);
    digitalWrite(LED_PIN,HIGH); // sink to activate up LED , so reverse 

    ShadowConnectParameters_t params;

    if (strlen(CLIENT_ID)<1) {
        uint64_t chipid=ESP.getEfuseMac();
        sprintf(deviceId,"%04X%08X\n",(uint16_t)(chipid>>32),(uint32_t)chipid);   
    } else {
        strncpy(deviceId,CLIENT_ID,MAX_DEVICE_ID_LEN);
    };
    Serial.print("Using deviceId: ");
    Serial.println(deviceId);  

    params.pMyThingName = deviceId;
    params.pMqttClientId = deviceId;
    params.mqttClientIdLen = (uint16_t) strlen(deviceId);
    params.deleteActionHandler = NULL;

    
    ledHandler.cb = led_Callback;
    ledHandler.pData = &ledState;
    ledHandler.pKey = "led";
    ledHandler.type = SHADOW_JSON_BOOL;

    
    btnHandler.cb = NULL;
    btnHandler.pData = &btnState;
    btnHandler.pKey = "btn";
    btnHandler.type = SHADOW_JSON_BOOL;

    awakeHandler.cb = awake_Callback;
    awakeHandler.pData = &awakeState;
    awakeHandler.pKey = "awake";
    awakeHandler.type = SHADOW_JSON_BOOL;

    stateHandler.cb = state_Callback;
    stateHandler.pData = NULL;
    stateHandler.pKey = "state";
    stateHandler.type = SHADOW_JSON_BOOL;


    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(WIFI_SSID);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // wait 5 seconds for connection:
        delay(5000);
    }

    Serial.println("Connected to wifi");

      if(hornbill.connect(HOST_ADDRESS, CLIENT_ID,
                aws_root_ca_pem, certificate_pem_crt, private_pem_key)== 0)
    {
        Serial.println("Connected to AWS");

        int rc = hornbill.attach_shadow(&params,&stateHandler); 
        if ( rc == 0 ) {
          
          Serial.println("Attached to Shadow");
          

        } else {
          Serial.print("Attached to Shadow (aws_iot_shadow_register_delta) FAILED :");
          Serial.println(rc);
        ;
        }
    }
    else
    {
        Serial.println("AWS connection failed, Check the HOST Address");
        while(1);
    }

    //xTaskCreatePinnedToCore(&aws_iot_shadow_task, "aws_iot_shadow_task", 9216, NULL, 5, NULL, 1);
    
    delay(2000);

}


void loop() {

    builtInState = !builtInState;
    digitalWrite(LED_BUILTIN, builtInState); 
    rc = hornbill.shadow_yield(200);
    if(NETWORK_ATTEMPTING_RECONNECT == rc) {
          vTaskDelay(1000 / portTICK_RATE_MS);
          // If the client is attempting to reconnect we will skip the rest of the loop.                                     
    } else {
      uint8_t btn_in = digitalRead(BTN_PIN);
      
      if (btn_in == btnState) {
        btnState = !btn_in;
        shouldUpdate = true;
      }

      if ( shouldUpdate ) {
        rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
        if(SUCCESS == rc) {
          rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 3, 
                                                                                          &awakeHandler,
                                                                                          &ledHandler,
                                                                                          &btnHandler);
          if(SUCCESS == rc) {
            rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
            if(SUCCESS == rc) {
              Serial.print("Update Shadow:");
              Serial.println(JsonDocumentBuffer);
              rc = hornbill.shadow_update(JsonDocumentBuffer);
              shouldUpdate = false;
              loopCount = 0;
            }                                                                                  
          }
        }
      }
      if (!canSleep) 
         if (10<loopCount++) {
         
          canSleep = true;
      };
      
      if (canSleep && (!awakeState) && (!btnState)) {
       vTaskDelay(2500 / portTICK_RATE_MS);
       hornbill.shadow_disconnect();
       Serial.println("Going to sleep now");
       esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
       esp_sleep_enable_ext0_wakeup(BTN_PIN,LOW);
       vTaskDelay(500 / portTICK_RATE_MS);
       esp_deep_sleep_start();
      };
    }
   
    vTaskDelay(1000 / portTICK_RATE_MS);
    tick++;
    
}
