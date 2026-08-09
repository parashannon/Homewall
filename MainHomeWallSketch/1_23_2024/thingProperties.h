#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <Arduino_NetworkConfigurator.h>
#include "configuratorAgents/agents/BLEAgent.h"
#include "configuratorAgents/agents/SerialAgent.h"
void onInternetStatusChange();
void onColumnChange();
void onProblemChange();
void onRandomProblemChange();
void onRowChange();
void onSaveProblemChange();

String internetStatus;
CloudColoredLight column;
CloudColoredLight problem;
CloudColoredLight randomProblem;
CloudColoredLight row;
CloudLight saveProblem;

KVStore kvStore;
BLEAgentClass BLEAgent;
SerialAgentClass SerialAgent;
WiFiConnectionHandler ArduinoIoTPreferredConnection; 
NetworkConfiguratorClass NetworkConfigurator(ArduinoIoTPreferredConnection);

void initProperties(){
  NetworkConfigurator.addAgent(BLEAgent);
  NetworkConfigurator.addAgent(SerialAgent);
  NetworkConfigurator.setStorage(kvStore);
  // For changing the default reset pin uncomment and set your preferred pin. Use DISABLE_PIN for disabling the reset procedure.
  //NetworkConfigurator.setReconfigurePin(your_pin);
  ArduinoCloud.setConfigurator(NetworkConfigurator);

  ArduinoCloud.addProperty(internetStatus, READWRITE, ON_CHANGE, onInternetStatusChange);
  ArduinoCloud.addProperty(column, READWRITE, ON_CHANGE, onColumnChange);
  ArduinoCloud.addProperty(problem, READWRITE, ON_CHANGE, onProblemChange);
  ArduinoCloud.addProperty(randomProblem, READWRITE, ON_CHANGE, onRandomProblemChange);
  ArduinoCloud.addProperty(row, READWRITE, ON_CHANGE, onRowChange);
  ArduinoCloud.addProperty(saveProblem, READWRITE, ON_CHANGE, onSaveProblemChange);

}
