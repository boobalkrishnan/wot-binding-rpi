#ifndef BINDCOAPSERVER
#define BINDCOAPSERVER
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include "BindServerDef.h"
#include "Logger.h"

using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;
using namespace Logging;

enum CoAP_UriLevel{
  CoAP_URI_THINGS_LEVEL=1,
  CoAP_URI_THING_LEVEL,
  CoAP_URI_INTERACT_LEVEL,
};

#define LOGGER_COMP_COAPSERVER  "CoapServer"

// typedef void (*CoapAdapter_interaction_get)(std::string ThingId, std::string Root_Name, std::string Interaction_Type, std::string Interaction_Name, ostream& DataOut);
// typedef void (*CoapAdapter_interaction_put)(std::string ThingId, std::string Root_Name, std::string Interaction_Type, std::string Interaction_Name, istream& DataIn, uint32_t Datalen);
// typedef void (*CoapAdapter_thing_get)(std::string HostIn, std::string ThingId, std::string Root_Name, Poco::JSON::Object *JsonMeta);
// typedef void (*CoapAdapter_affordance_get)(std::string HostIn, std::string ThingId, std::string Root_Name, std::string Interaction_Type, Poco::JSON::Object *ThingDesc);

extern Adapter_interaction_get CoapHandleSubscription;
extern Adapter_interaction_get CoapHandleAffordanceGet;
extern Adapter_interaction_put CoapHandleAffordancePut;
extern Adapter_thing_get CoapHandleThing;
extern Adapter_affordance_get CoapHandleAffordance;

class BindCoAPServer //: public ServerApplication
{
    private:
 
        // HTTPServer *WifiHotspot;

    public:
        /**
        * Construct a new Triangle object from another Triangle object.
        * @brief Copy constructor.
        * @param triangle Another Triangle object.

        */
        unsigned int Port;
        std::string Address;
        void Initialize(unsigned int port);
        void Process(void);
        Logger* pLogger = NULL; // Create the object pointer for Logger Class
        unsigned int count;
        void EmitEvent(std::string EventID, std::string out);
        void SetHandlers(Adapter_interaction_get GetHandleIn,
                              Adapter_interaction_put PutHandleIn, 
                              Adapter_thing_get thingGetIn,
                              Adapter_affordance_get affordIn,
                              Adapter_interaction_get SubsHandleIn);
        BindCoAPServer(void);
        void Start(void);
        void Stop(void);
};
#endif /* BINDCOAPSERVER */
