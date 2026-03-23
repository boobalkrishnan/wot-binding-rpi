
#ifndef BINDSERVER
#define BINDSERVER

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include "BindCoAPServer.h"
#include "BindHttpServer.h"

using namespace std;

#define LOGGER_COMP_BINDSERVER "BindServer"

typedef void (*Adapter_interaction_get)(std::string ThingId, std::string Root_Name, std::string Interaction_Type, std::string Interaction_Name, Poco::JSON::Object *JsonData);// ostream& DataOut);
typedef void (*Adapter_interaction_put)(std::string ThingId, std::string Root_Name, std::string Interaction_Type, std::string Interaction_Name, istream& DataIn, uint32_t Datalen);
typedef void (*Adapter_thing_get)(std::string HostIn, std::string ThingId, std::string Root_Name, Poco::JSON::Object *JsonMeta);
typedef void (*Adapter_affordance_get)(std::string HostIn, std::string ThingId, std::string Root_Name, std::string Interaction_Type, Poco::JSON::Object *ThingDesc);

#define BIND_HTTP_SERVER    1
#define BIND_COAP_SERVER    2
#define BIND_WS_SERVER      3

class BindServer: public BindHttpServer, BindCoAPServer
{
    public:
        uint8_t ServerType;
        BindHttpServer  *HttpServer;
        BindCoAPServer  *CoAPServer;
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int count;
        void EmitEvent(std::string EventID, std::string out);
        void SetHandlers(Adapter_interaction_get GetHandleIn,
                         Adapter_interaction_put SetHandleIn,
                         Adapter_thing_get thingGetIn,
                         Adapter_affordance_get affordIn,
                         Adapter_interaction_get SubsHandleIn);
        BindServer(uint8_t ServerNo);
        void Start(void);
        void Stop(void);
        void Initialize(unsigned int port);
        void Process(void);
};

#endif