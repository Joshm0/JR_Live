#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <DFRobotDFPlayerMini.h>
#include "parse.h"

#define TFIELD "odpt:trainNumber"
#define TSTION "odpt:toStation"

const char *toStation;
const char *prevStation = NULL;
// Takes in read api data and returns status
int parseData(char *api_data, int data_len) {

  StaticJsonDocument<384> doc;

  DeserializationError error = deserializeJson(doc, api_data, data_len);

  if(error) {

    Serial.print("Deserialization failed: ");
    Serial.println(error.f_str());

    return 1;
  }

  // Go through data and call necessary functions
  
  // Only update the train if we have none
  
  trainNumber = doc[TFIELD];
  train = true;

  toStation = doc[TSTION];

  Serial.print("Train Number: ");
  Serial.println(trainNumber);

  if (toStation) {
    Serial.print("To Station: ");
    Serial.println(toStation);
  }

  Serial.print("To Station Null?: ");
  Serial.println(!toStation ? "Yes" : "No");
  
  // Play audio based on station

  if ((prevStation != toStation) && toStation) {

    Serial.println("Play Station Melody");
    myDFPlayer.play(1);
  }

  prevStation = toStation;

  return 0;
}