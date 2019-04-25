#include <AWS_IOT.h>
#include <WiFi.h>

#include "config.h"

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 512

gpio_num_t LED_PIN = GPIO_NUM_18;
gpio_num_t BTN_PIN = GPIO_NUM_15;

AWS_IOT hornbill;

int status = WL_IDLE_STATUS;
int tick=0,msgCount=0,msgReceived = 0, rc = 0;

char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);


jsonStruct_t ledHandler;
jsonStruct_t btnHandler;
uint8_t ledState = 0;
uint8_t btnState = 0;

bool shouldUpdate = true;

void led_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
  
  Serial.println("led_Callback");

  ledState= strncmp(pJsonString,"true",4) ? 0 : 1 ;
  digitalWrite(LED_PIN,!ledState); // sink to activate up LED , so reverse 
  Serial.print("State: ");
  Serial.println(ledState);
  shouldUpdate = true;
}


void setup() {

    Serial.begin(115200);

    pinMode(BTN_PIN,INPUT);
    
    pinMode(LED_PIN,OUTPUT);
    digitalWrite(LED_PIN,HIGH); // sink to activate up LED , so reverse 

    // IOT Structures, two handlers no callback for btn since no action is required 

    ShadowConnectParameters_t params;

    params.pMyThingName = CLIENT_ID;
    params.pMqttClientId = CLIENT_ID;
    params.mqttClientIdLen = (uint16_t) strlen(CLIENT_ID);
    params.deleteActionHandler = NULL;

    
    ledHandler.cb = led_Callback;
    ledHandler.pData = &ledState;
    ledHandler.pKey = "led";
    ledHandler.type = SHADOW_JSON_BOOL;

    
    btnHandler.cb = NULL;
    btnHandler.pData = &btnState;
    btnHandler.pKey = "btn";
    btnHandler.type = SHADOW_JSON_BOOL;

    while (status != WL_CONNECTED)
    {
      // wait 5 seconds before connection attempt
        delay(5000);
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(WIFI_SSID);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);        
    }

    Serial.println("Connected to wifi");

    if(hornbill.connect(HOST_ADDRESS, CLIENT_ID,
                aws_root_ca_pem, certificate_pem_crt, private_pem_key)== 0)
    {
        Serial.println("Connected to AWS");

        int rc = hornbill.attach_shadow(&params,&ledHandler); 
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
          rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 2, &ledHandler,
                                                                                         &btnHandler);
          if(SUCCESS == rc) {
            rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
            if(SUCCESS == rc) {
              Serial.print("Update Shadow:");
              Serial.println(JsonDocumentBuffer);
              rc = hornbill.shadow_update(JsonDocumentBuffer);
              shouldUpdate = false;
            }                                                                                  
          }
        }
      }
     
      
    }
   
    vTaskDelay(1000 / portTICK_RATE_MS);
    tick++;
    
}
