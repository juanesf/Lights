/*
 * This file allows you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * EEPROM bytes 2944 to 3071 are reserved for your custom use case.
 */

//Use userVar0 and userVar1 (API calls &U0=,&U1=, uint16_t)

#include <ArduinoJson.h>

struct state {
  uint8_t colors[3], bri = briS, sat = 254, colorMode = 2;
  bool lightState;
  int ct = 200, hue;
  float stepLevel[3], currentColors[3], x, y;
};

//core
#define entertainmentTimeout 1500 // millis

state lights[10];
bool inTransition, entertainmentRun, useDhcp = true;
byte mac[6], packetBuffer[46];
unsigned long lastEPMillis;
uint16_t pixelCount = ledCount, lightLedsCount;

ESP8266HTTPUpdateServer httpUpdateServer;

RgbColor red = RgbColor(255, 0, 0);
RgbColor green = RgbColor(0, 255, 0);
RgbColor white = RgbColor(255);
RgbColor black = RgbColor(0);

NeoPixelBrightnessBus<PIXELFEATURE3,PIXELMETHOD>* bus = NULL;

void apply_scene(uint8_t new_scene) {
  for (uint8_t light = 0; light < lightsCount; light++) {
    if ( new_scene == 1) {
      lights[light].bri = 254; lights[light].ct = 346; lights[light].colorMode = 2; convertCt(light);
    } else if ( new_scene == 2) {
      lights[light].bri = 254; lights[light].ct = 233; lights[light].colorMode = 2; convertCt(light);
    }  else if ( new_scene == 3) {
      lights[light].bri = 254; lights[light].ct = 156; lights[light].colorMode = 2; convertCt(light);
    }  else if ( new_scene == 4) {
      lights[light].bri = 77; lights[light].ct = 367; lights[light].colorMode = 2; convertCt(light);
    }  else if ( new_scene == 5) {
      lights[light].bri = 254; lights[light].ct = 447; lights[light].colorMode = 2; convertCt(light);
    }  else if ( new_scene == 6) {
      lights[light].bri = 1; lights[light].x = 0.561; lights[light].y = 0.4042; lights[light].colorMode = 1; convertXy(light);
    }  else if ( new_scene == 7) {
      lights[light].bri = 203; lights[light].x = 0.380328; lights[light].y = 0.39986; lights[light].colorMode = 1; convertXy(light);
    }  else if ( new_scene == 8) {
      lights[light].bri = 112; lights[light].x = 0.359168; lights[light].y = 0.28807; lights[light].colorMode = 1; convertXy(light);
    }  else if ( new_scene == 9) {
      lights[light].bri = 142; lights[light].x = 0.267102; lights[light].y = 0.23755; lights[light].colorMode = 1; convertXy(light);
    }  else if ( new_scene == 10) {
      lights[light].bri = 216; lights[light].x = 0.393209; lights[light].y = 0.29961; lights[light].colorMode = 1; convertXy(light);
    } else {
      lights[light].bri = 144; lights[light].ct = 447; lights[light].colorMode = 2; convertCt(light);
    }
  }
}

void processLightdata(uint8_t light, float transitiontime) {
  transitiontime *= 17 - (pixelCount / 40); //every extra led add a small delay that need to be counted
  if (lights[light].colorMode == 1 && lights[light].lightState == true) {
    convertXy(light);
  } else if (lights[light].colorMode == 2 && lights[light].lightState == true) {
    convertCt(light);
  } else if (lights[light].colorMode == 3 && lights[light].lightState == true) {
    convertHue(light);
  }
  for (uint8_t i = 0; i < 3; i++) {
    if (lights[light].lightState) {
      lights[light].stepLevel[i] = ((float)lights[light].colors[i] - lights[light].currentColors[i]) / transitiontime;
    } else {
      lights[light].stepLevel[i] = lights[light].currentColors[i] / transitiontime;
    }
  }
}

void lightEngine() {
  for (int light = 0; light < lightsCount; light++) {
    if (lights[light].lightState) {
      if (lights[light].colors[0] != lights[light].currentColors[0] || lights[light].colors[1] != lights[light].currentColors[1] || lights[light].colors[2] != lights[light].currentColors[2]) {
        inTransition = true;
        for (uint8_t k = 0; k < 3; k++) {
          if (lights[light].colors[k] != lights[light].currentColors[k]) lights[light].currentColors[k] += lights[light].stepLevel[k];
          if ((lights[light].stepLevel[k] > 0.0 && lights[light].currentColors[k] > lights[light].colors[k]) || (lights[light].stepLevel[k] < 0.0 && lights[light].currentColors[k] < lights[light].colors[k])) lights[light].currentColors[k] = lights[light].colors[k];
        }
        if (lightsCount > 1) {
          if (light == 0) {
            for (uint8_t pixel = 0; pixel < lightLedsCount + transitionLeds / 2; pixel++) {
              if (pixel < lightLedsCount - transitionLeds / 2) {
                bus->SetPixelColor(pixel, convFloat(lights[light].currentColors));
              } else {
                bus->SetPixelColor(pixel, blending(lights[0].currentColors, lights[1].currentColors, pixel + 1 - (lightLedsCount - transitionLeds / 2 )));
              }
            }
          } else if (light == lightsCount - 1) {
            for (uint8_t pixel = 0; pixel < lightLedsCount + transitionLeds / 2 ; pixel++) {
              if (pixel < transitionLeds) {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light, blending( lights[light - 1].currentColors, lights[light].currentColors, pixel + 1));
              } else {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light, convFloat(lights[light].currentColors));
              }
            }
          } else {
            for (uint8_t pixel = 0; pixel < lightLedsCount + transitionLeds; pixel++) {
              if (pixel < transitionLeds) {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light,  blending( lights[light - 1].currentColors, lights[light].currentColors, pixel + 1));
              } else if (pixel > lightLedsCount - 1) {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light,  blending( lights[light].currentColors, lights[light + 1].currentColors, pixel + 1 - lightLedsCount));
              } else  {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light, convFloat(lights[light].currentColors));
              }
            }
          }
        } else {
          bus->ClearTo(convFloat(lights[light].currentColors), 0, pixelCount - 1);
        }
        bus->Show();
		colorUpdated(1);
      }
    } else {
      if (lights[light].currentColors[0] != 0 || lights[light].currentColors[1] != 0 || lights[light].currentColors[2] != 0) {
        inTransition = true;
        for (uint8_t k = 0; k < 3; k++) {
          if (lights[light].currentColors[k] != 0) lights[light].currentColors[k] -= lights[light].stepLevel[k];
          if (lights[light].currentColors[k] < 0) lights[light].currentColors[k] = 0;
        }
        if (lightsCount > 1) {
          if (light == 0) {
            for (uint8_t pixel = 0; pixel < lightLedsCount + transitionLeds / 2; pixel++) {
              if (pixel < lightLedsCount - transitionLeds / 2) {
                bus->SetPixelColor(pixel, convFloat(lights[light].currentColors));
              } else {
                bus->SetPixelColor(pixel,  blending( lights[light].currentColors, lights[light + 1].currentColors, pixel + 1 - (lightLedsCount - transitionLeds / 2 )));
              }
            }
          } else if (light == lightsCount - 1) {
            for (uint8_t pixel = 0; pixel < lightLedsCount + transitionLeds / 2 ; pixel++) {
              if (pixel < transitionLeds) {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light,  blending( lights[light - 1].currentColors, lights[light].currentColors, pixel + 1));
              } else {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light, convFloat(lights[light].currentColors));
              }
            }
          } else {
            for (uint8_t pixel = 0; pixel < lightLedsCount + transitionLeds; pixel++) {
              if (pixel < transitionLeds) {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light,  blending( lights[light - 1].currentColors, lights[light].currentColors, pixel + 1));
              } else if (pixel > lightLedsCount - 1) {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light,  blending( lights[light].currentColors, lights[light + 1].currentColors, pixel + 1 - lightLedsCount));
              } else  {
                bus->SetPixelColor(pixel - transitionLeds / 2 + lightLedsCount * light, convFloat(lights[light].currentColors));
              }
            }
          }
        } else {
          bus->ClearTo(convFloat(lights[light].currentColors), 0, pixelCount - 1);
        }
        bus->Show();
		colorUpdated(1);
      }
    }
  }
  if (inTransition) {
    delay(6);
    inTransition = false;
  } else if (hwSwitch == true) {
    if (digitalRead(onPin) == HIGH) {
      int i = 0;
      while (digitalRead(onPin) == HIGH && i < 30) {
        delay(20);
        i++;
      }
      for (int light = 0; light < lightsCount; light++) {
        if (i < 30) {
          // there was a short press
          lights[light].lightState = true;
        }
        else {
          // there was a long press
          lights[light].bri += 56;
          if (lights[light].bri > 255) {
            // don't increase the brightness more then maximum value
            lights[light].bri = 255;
          }
        }
      }
    } else if (digitalRead(offPin) == HIGH) {
      int i = 0;
      while (digitalRead(offPin) == HIGH && i < 30) {
        delay(20);
        i++;
      }
      for (int light = 0; light < lightsCount; light++) {
        if (i < 30) {
          // there was a short press
          lights[light].lightState = false;
        }
        else {
          // there was a long press
          lights[light].bri -= 56;
          if (lights[light].bri < 1) {
            // don't decrease the brightness less than minimum value.
            lights[light].bri = 1;
          }
        }
      }
    }
  }
}

void saveState() {
  DynamicJsonDocument json(1024);
  for (uint8_t i = 0; i < lightsCount; i++) {
    JsonObject light = json.createNestedObject((String)i);
    light["on"] = lights[i].lightState;
    light["bri"] = lights[i].bri;
    if (lights[i].colorMode == 1) {
      light["x"] = lights[i].x;
      light["y"] = lights[i].y;
    } else if (lights[i].colorMode == 2) {
      light["ct"] = lights[i].ct;
    } else if (lights[i].colorMode == 3) {
      light["hue"] = lights[i].hue;
      light["sat"] = lights[i].sat;
    }
  }
  File stateFile = SPIFFS.open("/state.json", "w");
  serializeJson(json, stateFile);

}

void restoreState() {
  File stateFile = SPIFFS.open("/state.json", "r");
  if (!stateFile) {
    saveState();
    return;
  }

  DynamicJsonDocument json(1024);
  DeserializationError error = deserializeJson(json, stateFile.readString());
  if (error) {
    //Serial.println("Failed to parse config file");
    return;
  }
  for (JsonPair state : json.as<JsonObject>()) {
    const char* key = state.key().c_str();
    int lightId = atoi(key);
    JsonObject values = state.value();
    lights[lightId].lightState = values["on"];
    lights[lightId].bri = (uint8_t)values["bri"];
    if (values.containsKey("x")) {
      lights[lightId].x = values["x"];
      lights[lightId].y = values["y"];
      lights[lightId].colorMode = 1;
    } else if (values.containsKey("ct")) {
      lights[lightId].ct = values["ct"];
      lights[lightId].colorMode = 2;
    } else {
      if (values.containsKey("hue")) {
        lights[lightId].hue = values["hue"];
        lights[lightId].colorMode = 3;
      }
      if (values.containsKey("sat")) {
        lights[lightId].sat = (uint8_t) values["sat"];
        lights[lightId].colorMode = 3;
      }
    }
  }
}

bool saveConfig() {
  DynamicJsonDocument json(1024);
  json["name"] = lightName;
  json["startup"] = startup;
  json["scene"] = scene;
  json["on"] = onPin;
  json["off"] = offPin;
  json["hw"] = hwSwitch;
  json["dhcp"] = useDhcp;
  json["lightsCount"] = lightsCount;
  json["pixelCount"] = pixelCount;
  json["transLeds"] = transitionLeds;
  JsonArray addr = json.createNestedArray("addr");
  addr.add(staticIP[0]);
  addr.add(staticIP[1]);
  addr.add(staticIP[2]);
  addr.add(staticIP[3]);
  JsonArray gw = json.createNestedArray("gw");
  gw.add(staticGateway[0]);
  gw.add(staticGateway[1]);
  gw.add(staticGateway[2]);
  gw.add(staticGateway[3]);
  JsonArray mask = json.createNestedArray("mask");
  mask.add(staticSubnet[0]);
  mask.add(staticSubnet[1]);
  mask.add(staticSubnet[2]);
  mask.add(staticSubnet[3]);
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    //Serial.println("Failed to open config file for writing");
    return false;
  }

  serializeJson(json, configFile);
  return true;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    //Serial.println("Create new file with default values");
    return saveConfig();
  }

  if (configFile.size() > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  DynamicJsonDocument json(1024);
  DeserializationError error = deserializeJson(json, configFile.readString());
  if (error) {
    //Serial.println("Failed to parse config file");
    return false;
  }

  strcpy(lightName, json["name"]);
  startup = (uint8_t) json["startup"];
  scene  = (uint8_t) json["scene"];
  onPin = (uint8_t) json["on"];
  offPin = (uint8_t) json["off"];
  hwSwitch = json["hw"];
  lightsCount = (uint16_t) json["lightsCount"];
  pixelCount = (uint16_t) json["pixelCount"];
  transitionLeds = (uint8_t) json["transLeds"];
  useDhcp = json["dhcp"];
  staticIP = {json["addr"][0], json["addr"][1], json["addr"][2], json["addr"][3]};
  staticSubnet = {json["mask"][0], json["mask"][1], json["mask"][2], json["mask"][3]};
  staticGateway = {json["gw"][0], json["gw"][1], json["gw"][2], json["gw"][3]};
  return true;
}

void ChangeNeoPixels(uint16_t newCount)
{
  if (bus != NULL) {
    delete bus; // delete the previous dynamically created strip
  }
  bus = new NeoPixelBrightnessBus<PIXELFEATURE3,PIXELMETHOD>(newCount); // and recreate with new count
  bus->Begin();
}

void userBeginPreConnection()
{
  
}

void userBegin()
{
  //Serial.begin(115200);
  //Serial.println();
  delay(1000);

  //Serial.println("mounting FS...");

  if (!SPIFFS.begin()) {
    //Serial.println("Failed to mount file system");
    return;
  }

  if (!loadConfig()) {
    //Serial.println("Failed to load config");
  } else {
    ////Serial.println("Config loaded");
  }

  lightLedsCount = pixelCount / lightsCount;
  ChangeNeoPixels(pixelCount);


  if (startup == 1) {
    for (uint8_t i = 0; i < lightsCount; i++) {
      lights[i].lightState = true;
    }
  }
  if (startup == 0) {
    restoreState();
  } else {
    apply_scene(scene);
  }
  for (uint8_t i = 0; i < lightsCount; i++) {
    processLightdata(i, 4);
  }
  if (lights[0].lightState) {
    for (uint8_t i = 0; i < 200; i++) {
      lightEngine();
    }
  }


  if (useDhcp) {
    staticIP = WiFi.localIP();
    staticGateway = WiFi.gatewayIP();
    staticSubnet = WiFi.subnetMask();
  }

  WiFi.macAddress(mac);
  httpUpdateServer.setup(&server);
  Udp.begin(2100);

  if (hwSwitch == true) {
    pinMode(onPin, INPUT);
    pinMode(offPin, INPUT);
  }

  server.on("/state", HTTP_PUT, []() {
    bool stateSave = false;
    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, server.arg("plain"));
    if (error) {
      server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
    } else {
      for (JsonPair state : root.as<JsonObject>()) {
        const char* key = state.key().c_str();
        int light = atoi(key) - 1;
        JsonObject values = state.value();
        int transitiontime = 4;

        if (values.containsKey("xy")) {
          lights[light].x = values["xy"][0];
          lights[light].y = values["xy"][1];
          lights[light].colorMode = 1;
        } else if (values.containsKey("ct")) {
          lights[light].ct = values["ct"];
          lights[light].colorMode = 2;
        } else {
          if (values.containsKey("hue")) {
            lights[light].hue = values["hue"];
            lights[light].colorMode = 3;
          }
          if (values.containsKey("sat")) {
            lights[light].sat = values["sat"];
            lights[light].colorMode = 3;
          }
        }

        if (values.containsKey("on")) {
          if (values["on"]) {
            lights[light].lightState = true;
          } else {
            lights[light].lightState = false;
          }
          if (startup == 0) {
            stateSave = true;
          }
        }

        if (values.containsKey("bri")) {
          lights[light].bri = values["bri"];
        }

        if (values.containsKey("briS")) {
          lights[light].bri = values["briS"];
        }

        if (values.containsKey("bri_inc")) {
          lights[light].bri += (int) values["bri_inc"];
          if (lights[light].bri > 255) lights[light].bri = 255;
          else if (lights[light].bri < 1) lights[light].bri = 1;
        }

        if (values.containsKey("transitiontime")) {
          transitiontime = values["transitiontime"];
        }

        if (values.containsKey("alert") && values["alert"] == "select") {
          if (lights[light].lightState) {
            lights[light].currentColors[0] = 0; lights[light].currentColors[1] = 0; lights[light].currentColors[2] = 0; lights[light].currentColors[3] = 0;
          } else {
            lights[light].currentColors[3] = 126; lights[light].currentColors[4] = 126;
          }
        }
        processLightdata(light, transitiontime);
      }
      String output;
      serializeJson(root,output);
      server.send(200, "text/plain", output);
      if (stateSave) {
        saveState();
      }
    }
  });

  server.on("/state", HTTP_GET, []() {
    uint8_t light = server.arg("light").toInt() - 1;
    DynamicJsonDocument root(1024);
    root["on"] = lights[light].lightState || (bri > 0);
    root["bri"] = lights[light].bri;
    JsonArray xy = root.createNestedArray("xy");
    xy.add(lights[light].x);
    xy.add(lights[light].y);
	  JsonArray colS = root.createNestedArray("colS");
    colS.add(colS[0]);
    colS.add(colS[1]);
    colS.add(colS[2]);
	  root["briS"] = bri;
    root["ct"] = lights[light].ct;
    root["hue"] = lights[light].hue;
    root["sat"] = lights[light].sat;
    if (lights[light].colorMode == 1)
      root["colormode"] = "xy";
    else if (lights[light].colorMode == 2)
      root["colormode"] = "ct";
    else if (lights[light].colorMode == 3)
      root["colormode"] = "hs";
    String output;
    serializeJson(root, output);
    server.send(200, "text/plain", output);
  });

  server.on("/detect", []() {
    char macString[32] = {0};
    sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DynamicJsonDocument root(1024);
    root["name"] = lightName;
    root["lights"] = lightsCount;
    root["protocol"] = "native_multi";
    root["modelid"] = "LST002";
    root["type"] = "ws2812_strip";
    root["mac"] = String(macString);
    root["version"] = 2.0;
    String output;
    serializeJson(root, output);
    server.send(200, "text/plain", output);
  });

  server.on("/config", []() {
    DynamicJsonDocument root(1024);
    root["name"] = lightName;
    root["scene"] = scene;
    root["startup"] = startup;
    root["hw"] = hwSwitch;
    root["on"] = onPin;
    root["off"] = offPin;
    root["hwswitch"] = (int)hwSwitch;
    root["lightscount"] = lightsCount;
    root["pixelcount"] = pixelCount;
    root["transitionleds"] = transitionLeds;
    root["dhcp"] = (int)useDhcp;
    root["addr"] = (String)staticIP[0] + "." + (String)staticIP[1] + "." + (String)staticIP[2] + "." + (String)staticIP[3];
    root["gw"] = (String)staticGateway[0] + "." + (String)staticGateway[1] + "." + (String)staticGateway[2] + "." + (String)staticGateway[3];
    root["sn"] = (String)staticSubnet[0] + "." + (String)staticSubnet[1] + "." + (String)staticSubnet[2] + "." + (String)staticSubnet[3];
    String output;
    serializeJson(root, output);
    server.send(200, "text/plain", output);
  });

  server.begin();
}

void entertainment() {
  uint8_t packetSize = Udp.parsePacket();
  if (packetSize) {
    if (!entertainmentRun) {
      entertainmentRun = true;
    }
    lastEPMillis = millis();
    Udp.read(packetBuffer, packetSize);
    for (uint8_t i = 0; i < packetSize / 4; i++) {
      lights[packetBuffer[i * 4]].currentColors[0] = packetBuffer[i * 4 + 1];
      lights[packetBuffer[i * 4]].currentColors[1] = packetBuffer[i * 4 + 2];
      lights[packetBuffer[i * 4]].currentColors[2] = packetBuffer[i * 4 + 3];
    }
    for (uint8_t light = 0; light < lightsCount; light++) {
      if (lightsCount > 1) {
        if (light == 0) {
          for (uint8_t pixel = 0; pixel < lightLedsCount + transitionLeds / 2; pixel++) {
            if (pixel < lightLedsCount - transitionLeds / 2) {
              bus->SetPixelColor(pixel, convInt(lights[light].currentColors));
            } else {
              bus->SetPixelColor(pixel, blendingEntert(lights[0].currentColors, lights[1].currentColors, pixel + 1 - (lightLedsCount - transitionLeds / 2 )));
            }
          }
        } else if (light == lightsCount - 1) {
          for (uint8_t pixel = 0; pixel < lightLedsCount - transitionLeds / 2 ; pixel++) {
            bus->SetPixelColor(pixel + transitionLeds / 2 + lightLedsCount * light, convInt(lights[light].currentColors));
          }
        } else {
          for (uint8_t pixel = 0; pixel < lightLedsCount; pixel++) {
            if (pixel < lightLedsCount - transitionLeds) {
              bus->SetPixelColor(pixel + transitionLeds / 2 + lightLedsCount * light, convInt(lights[light].currentColors));
            } else {
              bus->SetPixelColor(pixel + transitionLeds / 2 + lightLedsCount * light, blendingEntert(lights[light].currentColors, lights[light + 1].currentColors, pixel - (lightLedsCount - transitionLeds ) + 1));
            }
          }
        }
      } else {
        for (uint8_t pixel = 0; pixel < lightLedsCount; pixel++) {
          bus->SetPixelColor(pixel, convInt(lights[light].currentColors));
		  }
		}
    }
    bus->Show();
	colorUpdated(1);
  }
}

void userLoop()
{
  server.handleClient();
  if (!entertainmentRun) {
    lightEngine();
  } else {
    if ((millis() - lastEPMillis) >= entertainmentTimeout) {
      entertainmentRun = false;
      for (uint8_t i = 0; i < lightsCount; i++) {
        processLightdata(i, 4); //return to original colors with 0.4 sec transition
      }
    }
  }
  entertainment();
}
