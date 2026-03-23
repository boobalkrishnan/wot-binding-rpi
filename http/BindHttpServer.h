#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/DNSSD/DNSSDResponder.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include "Logger.h"
#include "BindServerDef.h"

#ifndef BINDHTTPSERVER
#define BINDHTTPSERVER
using namespace Poco::DNSSD;
using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;
using namespace Logging;

#define LOGGER_COMP_HTTPSERVER "HttpServer"

enum UriLevel{
  URI_THINGS_LEVEL=1,
  URI_THING_LEVEL,
  URI_INTERACT_LEVEL,
};

// #define URI_THINGS_INDEX        0
#define URI_THING_INDEX         0
#define URI_AFFORDANCES_INDEX   1
#define URI_AFFORDANCE_INDEX    2
#define URI_SUBSCRIPTION_INDEX  3

#define PROPERTY_INTERACT "property"
#define ACTION_INTERACT "action"
#define EVENT_INTERACT "event"

// typedef bool (*Adapter_callback_get)(std::string RxURI, Poco::JSON::Object *ThingDesc);
// typedef bool (*Adapter_callback_put)(std::string RxURI, Poco::JSON::Object::Ptr JsonMeta);
// typedef void (*Adapter_interaction_get)(std::string ThingId, std::string Root_Name, std::string Interaction_Type, std::string Interaction_Name, ostream& DataOut);
// typedef void (*Adapter_interaction_put)(std::string ThingId, std::string Root_Name, std::string Interaction_Type, std::string Interaction_Name, istream& DataIn, uint32_t Datalen);
// typedef void (*Adapter_thing_get)(std::string HostIn, std::string ThingId, std::string Root_Name, Poco::JSON::Object *JsonMeta);
// typedef void (*Adapter_affordance_get)(std::string HostIn, std::string ThingId, std::string Root_Name, std::string Interaction_Type, Poco::JSON::Object *ThingDesc);

class BindHttpServerHandler : public HTTPRequestHandler
{
    private:
        void PropertyReadWriteHandler();
    public:
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int ParseThingsURI(std::string RxURI,std::string* ArrayOfURI);
        virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
};

class BindHttpAffordanceHandler : public HTTPRequestHandler
{
    public:
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int ParseThingsURI(std::string RxURI,std::string* ArrayOfURI);
        virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
};

class BindHttpPropertyHandler : public HTTPRequestHandler
{
    public:
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int ParseThingsURI(std::string RxURI,std::string* ArrayOfURI);
        virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
};
class BindHttpActionHandler : public HTTPRequestHandler
{
    public:
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int ParseThingsURI(std::string RxURI,std::string* ArrayOfURI);
        virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
};

class BindHttpThingsHandler : public HTTPRequestHandler
{
    public:
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int ParseThingsURI(std::string RxURI,std::string* ArrayOfURI);
        virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
};

class BindHttpPropertyObservHandler : public HTTPRequestHandler
{
    public:
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int ParseThingsURI(std::string RxURI,std::string* ArrayOfURI);
        virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
};

class BindHttpEventObservHandler : public HTTPRequestHandler
{
    public:
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int ParseThingsURI(std::string RxURI,std::string* ArrayOfURI);
        virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
};

class BindHttpServerHandlerFactory : public HTTPRequestHandlerFactory
{
    public:
        virtual HTTPRequestHandler* createRequestHandler(const HTTPServerRequest &);
};

class BindSseServerHandler : public HTTPRequestHandler
{
    private:
        void PropertyReadWriteHandler();
    public:
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int ParseThingsURI(std::string RxURI,std::string* ArrayOfURI);
        virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
};


class BindSseServerHandlerFactory : public HTTPRequestHandlerFactory
{
    public:
        virtual HTTPRequestHandler* createRequestHandler(const HTTPServerRequest &);
};
/**
 * @brief Triangle class used for triangle manipulations.
 */
 

extern Adapter_interaction_get HandleSubscription;
extern Adapter_interaction_get HandleAffordanceGet;
extern Adapter_interaction_put HandleAffordancePut;
extern Adapter_thing_get HandleThing;
extern Adapter_affordance_get HandleAffordance;
extern void EmitSubscribedEvent(std::string EventID, std::string out);

class BindHttpServer //: public ServerApplication
{
    private:
 
        // HTTPServer *WifiHotspot;
        HTTPServer *WotServer;
        HTTPServer *WotSseServer;
    public:
        /**
        * Construct a new Triangle object from another Triangle object.
        * @brief Copy constructor.
        * @param triangle Another Triangle object.

        */
        unsigned int Port;
        std::string Address;
	    DNSSDResponder *dnssdResponder;
        int main(const vector<string> &);
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int count;
        void Process(void);
        void EmitEvent(std::string EventID, std::string out);
        void SetHandlers(Adapter_interaction_get GetHandleIn,
                         Adapter_interaction_put SetHandleIn,
                         Adapter_thing_get thingGetIn,
                         Adapter_affordance_get affordIn,
                         Adapter_interaction_get SubsHandleIn);
        BindHttpServer(void);
        void Initialize(unsigned int port);
        void RegisterServer(string ThingName);
        void Start(void);
        void Stop(void);
};

#endif /* BINDHTTPSERVER */
