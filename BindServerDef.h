#ifndef BINDSERVERDEF
#define BINDSERVERDEF

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

using namespace std;
typedef void (*EventHandler_ptr)(std::string ThingID, std::string EventID, Poco::JSON::Object::Ptr eventData);
typedef void (*Adapter_interaction_get)(std::string ThingId, std::string Root_Name, std::string Interaction_Type, std::string Interaction_Name, Poco::JSON::Object *JsonData);// ostream& DataOut);
typedef void (*Adapter_interaction_put)(std::string ThingId, std::string Root_Name, std::string Interaction_Type, std::string Interaction_Name, istream& DataIn, uint32_t Datalen);
typedef void (*Adapter_thing_get)(std::string HostIn, std::string ThingId, std::string Root_Name, Poco::JSON::Object *JsonMeta);
typedef void (*Adapter_affordance_get)(std::string HostIn, std::string ThingId, std::string Root_Name, std::string Interaction_Type, Poco::JSON::Object *ThingDesc);

#define BIND_HTTP_SERVER    1
#define BIND_COAP_SERVER    2


#endif /* BINDSERVERDEF */
