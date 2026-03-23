#include <Poco/Net/SocketAddress.h>
#include <Poco/Random.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Exception.h>
#include <coap3/coap.h>
#include <iostream>
#include <streambuf>
#include <string>


#include <sys/epoll.h>
#include "Logger.h"
#include <errno.h>
#include "BindServerDef.h"
#include "BindCoAPServer.h"


using namespace Logging;
#define PROPERTY_INTERACT "property"
#define ACTION_INTERACT "action"
#define EVENT_INTERACT "event"
// #define URI_THINGS_INDEX        0
#define URI_THING_INDEX         0
#define URI_AFFORDANCES_INDEX   1
#define URI_AFFORDANCE_INDEX    2
#define URI_SUBSCRIPTION_INDEX  3
#define MAX_EVENTS 10
Logger* CoAP_pLogger = NULL; // Create the object pointer for Logger Class
coap_context_t* pCoapContext = nullptr;
pthread_t CoAP_Thread;
pthread_mutex_t CoAP_Receive_Mutex;
int coap_fd;
int epoll_fd;
struct epoll_event ev;
struct epoll_event events[MAX_EVENTS];
int nevents;
Adapter_interaction_get CoapHandleSubscription;
Adapter_interaction_get CoapHandleAffordanceGet;
Adapter_interaction_put CoapHandleAffordancePut;
Adapter_thing_get CoapHandleThing;
Adapter_affordance_get CoapHandleAffordance;



std::string decodeURIComponent(std::string encoded) {

    std::string decoded = encoded;
    std::smatch sm;
    std::string haystack;

    int dynamicLength = decoded.size() - 2;

    if (decoded.size() < 3) return decoded;

    for (int i = 0; i < dynamicLength; i++)
    {

        haystack = decoded.substr(i, 3);

        if (std::regex_match(haystack, sm, std::regex("%[0-9A-F]{2}")))
        {
            haystack = haystack.replace(0, 1, "0x");
            std::string rc = {(char)std::stoi(haystack, nullptr, 16)};
            decoded = decoded.replace(decoded.begin() + i, decoded.begin() + i + 3, rc);
        }

        dynamicLength = decoded.size() - 2;

    }

    return decoded;
}

unsigned int CoAP_ParseThingsURI(std::string RxURI,std::string* ArrayOfURI)
{
    std::string delimiter = "/";
    int LevelOfUTI=0;
    size_t pos = 0;
    std::string token;
	int position = RxURI.find(":");
	std::string RxURI_Local = decodeURIComponent(RxURI.substr(position+1));
    cout << "Parsing:" << RxURI_Local << std::endl;
	// RxURI_Local.replace(0,"%2F","/");
	cout << "Delimiter is:" << delimiter << std::endl;
    while ((pos = RxURI_Local.find(delimiter)) != std::string::npos)
    {
        // cout << "Parsing:" << RxURI << std::endl;  
        token = RxURI_Local.substr(0, pos);
        if (token != "")
        {
            ArrayOfURI[LevelOfUTI]=token;
            LevelOfUTI++;
        }
        RxURI_Local.erase(0, pos + delimiter.length());

    }
    cout << "Parsed:" << RxURI_Local << std::endl;
    cout << "LevelOfUTI:" << LevelOfUTI << std::endl;
    ArrayOfURI[LevelOfUTI]=RxURI_Local;
    // pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Parsed paths:",LevelOfUTI);
    // cout << "Parsed Levels:" << LevelOfUTI << std::endl; 
    return (LevelOfUTI);  
}

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

void CoAP_PutHandler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t* pResponse)
{
    Poco::JSON::Object Response;
    std::string Host;
    std::string ThingURI[10];
    std::string Interaction_Type="";
    std::string Interaction_Name="";
    std::string Thing_Name;
    std::string Root_Name;
    CoAP_UriLevel RequestLevel;
	coap_option_num_t optnum;
	coap_optlist_t *optlist_chain;	
	char addr_str[INET6_ADDRSTRLEN];
	static coap_resource_t *resource_local;
	uint16_t port;
	resource_local = resource;
	coap_string_t *uri= coap_get_uri_path(request); //->get_request_uri();
	const coap_address_t *local_addr = coap_session_get_addr_local(session);
	inet_ntop(AF_INET, &local_addr->addr.sin.sin_addr, addr_str, sizeof(addr_str));
	port = ntohs(local_addr->addr.sin.sin_port);
	//std::string port(coap_address_get_port(local_addr));
	cout <<"Local address: %s, Port: %u\n" << addr_str << port;
	Host = reinterpret_cast<char*>(addr_str);
	Host += ":"+ std::to_string(port);
	// if (coap_path_into_optlist(uri->s,uri->length,optnum,&optlist_chain))
	// {
	CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,(const char*)uri->s);//, uri);
	// }
	std::string Request_URI = (const char*)uri->s;

	// cout << "Request URI=" << req.getURI() << endl;
	RequestLevel = (CoAP_UriLevel) CoAP_ParseThingsURI(Request_URI,ThingURI);
	//CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"Requested Path Level:",RequestLevel);

	switch(RequestLevel)
	{
		// case URI_THINGS_INDEX:
		case URI_THING_INDEX:
		{
			Root_Name = ThingURI[0];
			Thing_Name = ThingURI[0];
			CoapHandleThing(Host,Thing_Name,Root_Name, &Response);
		}
		break;
		/* Handler for specific Interaction */
		case URI_AFFORDANCE_INDEX:
		{
			Interaction_Name = ThingURI[URI_AFFORDANCE_INDEX]; // Name of the interaction
		}
		/* Handler for reteriving all Interaction */
		case URI_AFFORDANCES_INDEX:
		{
			Root_Name = ThingURI[0];    // Root Name is not used
			Thing_Name = ThingURI[0];
			if (ThingURI[URI_AFFORDANCES_INDEX] == PROPERTY_INTERACT)
			{
				Interaction_Type = PROPERTY_INTERACT;
			}
			else if (ThingURI[URI_AFFORDANCES_INDEX] == ACTION_INTERACT)
			{
				Interaction_Type = ACTION_INTERACT;
			}
			else if (ThingURI[URI_AFFORDANCES_INDEX] == EVENT_INTERACT)
			{
				Interaction_Type = EVENT_INTERACT;
			}
			// CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"Interaction Type: ",Interaction_Type.c_str());
			// CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"Interaction Name: ",Interaction_Name.c_str());
			if (Interaction_Name == "")
			{
				/* Handler for reteriving all Interaction */
				CoapHandleAffordance(Host, Thing_Name, Root_Name, Interaction_Type, &Response);
			}
			else
			{
				size_t length;
				const uint8_t *InputData;
				size_t offset;
				size_t total;
				/* Handler for specific Interaction request*/
				// ostream& out = resp.send();
					/* Data will be updated in ostream out parameter in handler function*/
				// CoapHandleAffordanceGet(Thing_Name, Root_Name, Interaction_Type,Interaction_Name, &Response);
                // Response.stringify(out);
				// size_t size = (size_t)req.getContentLength();
				// cout << "size:" << size <<endl;
				// std::istream& stream = req.stream();
				coap_get_data_large(request, &length, &InputData, &offset, &total);

				std::string RxData;
				if (InputData)
				{
					RxData = ((const char*)InputData);
				}

				// CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"Data Received: ", RxData.c_str());
				// membuf sbuf((char*)data, data + sizeof(data));
				istringstream stream(RxData);
    			// std::istream stream(&sbuf);
				/* Data will be extracted in istream stream paramter in handler function */
				CoapHandleAffordancePut(Thing_Name, Root_Name, Interaction_Type,Interaction_Name, stream,length);
			}
		}
		break;
	}

	static Poco::Random rnd;

	coap_pdu_set_code(pResponse, COAP_RESPONSE_CODE_CONTENT);
	// pResponse->code = COAP_RESPONSE_CODE(205);
	int rssi = -50 - rnd.next(20);
	std::string data;
	std::ostringstream oss;
	Poco::JSON::Stringifier::stringify(Response, oss, 0);
	data = oss.str();
	CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"Data to be sent:",data.c_str());
	coap_add_data(pResponse, data.size(), reinterpret_cast<const uint8_t*>(data.data()));
}

void CoAP_GetHandler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t* pResponse)
{
    Poco::JSON::Object Response;
    std::string Host;
    std::string ThingURI[10];
    std::string Interaction_Type="";
    std::string Interaction_Name="";
    std::string Thing_Name;
    std::string Root_Name;
    CoAP_UriLevel RequestLevel;
	static coap_resource_t *resource_local;
	char addr_str[INET6_ADDRSTRLEN];
	uint16_t port;

	//coap_string_t *uri= coap_get_uri_path(request); //->get_request_uri();
	const coap_address_t *local_addr = coap_session_get_addr_local(session);
	inet_ntop(AF_INET, &local_addr->addr.sin.sin_addr, addr_str, sizeof(addr_str));
	port = ntohs(local_addr->addr.sin.sin_port);
	//std::string port(coap_address_get_port(local_addr));
	cout <<"Local address: %s, Port: %u\n" << addr_str << port;
	Host = "coap://";
	Host += reinterpret_cast<char*>(addr_str);
	Host += ":"+ std::to_string(port);

	resource_local = resource;
	pthread_mutex_lock(&CoAP_Receive_Mutex);
	coap_string_t *uri= coap_get_uri_path(request); //->get_request_uri();
	// coap_string_t *uri= coap_get_query(request); //->get_request_uri();
	CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,(const char*)uri->s);//, uri);
	std::string Request_URI = (const char*)uri->s;

	// cout << "Request URI=" << req.getURI() << endl;
	RequestLevel = (CoAP_UriLevel) CoAP_ParseThingsURI(Request_URI,ThingURI);
	CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"Requested Path Level:",RequestLevel);

	switch(RequestLevel)
	{
		// case URI_THINGS_INDEX:
		case URI_THING_INDEX:
		{
			Root_Name = ThingURI[0];
			Thing_Name = ThingURI[0];
			CoapHandleThing(Host,Thing_Name,Root_Name, &Response);
		}
		break;
		/* Handler for specific Interaction */
		case URI_AFFORDANCE_INDEX:
		{
			Interaction_Name = ThingURI[URI_AFFORDANCE_INDEX]; // Name of the interaction
		}
		/* Handler for reteriving all Interaction */
		case URI_AFFORDANCES_INDEX:
		{
			Root_Name = ThingURI[0];    // Root Name is not used
			Thing_Name = ThingURI[0];
			if (ThingURI[URI_AFFORDANCES_INDEX] == PROPERTY_INTERACT)
			{
				Interaction_Type = PROPERTY_INTERACT;
			}
			else if (ThingURI[URI_AFFORDANCES_INDEX] == ACTION_INTERACT)
			{
				Interaction_Type = ACTION_INTERACT;
			}
			else if (ThingURI[URI_AFFORDANCES_INDEX] == EVENT_INTERACT)
			{
				Interaction_Type = EVENT_INTERACT;
			}
			CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"Interaction Type: ",Interaction_Type.c_str());
			CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"Interaction Name: ",Interaction_Name.c_str());
			if (Interaction_Name == "")
			{
				/* Handler for reteriving all Interaction */
				CoapHandleAffordance(Host, Thing_Name, Root_Name, Interaction_Type, &Response);
			}
			else
			{
				/* Handler for specific Interaction request*/
				// ostream& out = resp.send();
					/* Data will be updated in ostream out parameter in handler function*/
				CoapHandleAffordanceGet(Thing_Name, Root_Name, Interaction_Type,Interaction_Name, &Response);
                // Response.stringify(out);
			}
		}
		break;
	}

	static Poco::Random rnd;

	coap_pdu_set_code(pResponse, (coap_pdu_code_t)COAP_RESPONSE_CODE(203));
	// pResponse->code = COAP_RESPONSE_CODE(205);
	int rssi = -50 - rnd.next(20);
	std::string data;
	std::ostringstream oss;
	Poco::JSON::Stringifier::stringify(Response, oss, 0);
	data = oss.str();
	CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"Data to be sent:",data.c_str());
	if (data.size() < 1024)
	{
		coap_add_data(pResponse, data.size(), reinterpret_cast<const uint8_t*>(data.data()));
	}
	else
	{
		coap_add_data_large_response(resource_local, session, request, pResponse,
									query, COAP_MEDIATYPE_TEXT_PLAIN, 1, 0,
									data.size(),
									reinterpret_cast<const uint8_t*>(data.data()),
									NULL, NULL);		
	}
	pthread_mutex_unlock(&CoAP_Receive_Mutex);
}

// static void *CoAP_Process(void*)
void BindCoAPServer::Process(void)
{
    // int result,i;
    // /* Wait until any i/o takes place */
    // nevents = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    // if (nevents == -1) {
    //   if (errno != EAGAIN) {
    //     coap_log_debug("epoll_wait: %s (%d)\n", coap_socket_strerror(), errno);
    //   }
    // }
    // for (i = 0; i < nevents; i++) {
    //   if (events[i].data.fd == coap_fd) {
    //     result = coap_io_process(pCoapContext, COAP_IO_NO_WAIT);
    //     if (result < 0) {
    //       /* There is an internal issue */
    //     }
    //   } else {
    //     /* Process other events */
    //   }
    // }
    /* Do any other housekeeping */
	if (pCoapContext != NULL){
		coap_run_once(pCoapContext, 0);
	}

}

void BindCoAPServer::SetHandlers(Adapter_interaction_get GetHandleIn,
                                 Adapter_interaction_put PutHandleIn, 
                                 Adapter_thing_get thingGetIn,
                                 Adapter_affordance_get affordIn,
                                 Adapter_interaction_get SubsHandleIn)
{
	CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"SetHandlers call");    
    CoapHandleAffordanceGet = GetHandleIn;
    CoapHandleAffordancePut = PutHandleIn;
    CoapHandleThing = thingGetIn;
    CoapHandleAffordance = affordIn;
    CoapHandleSubscription = SubsHandleIn;
}

void BindCoAPServer::Initialize(unsigned int port)
{
	Port = port;
	pLogger = Logger::getInstance();
	CoAP_pLogger = Logger::getInstance();
	coap_set_log_level(COAP_LOG_DEBUG);
	coap_startup();
	pthread_mutex_init(&CoAP_Receive_Mutex, NULL);
	CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,":Initializing");
	try
	{
		Poco::Net::SocketAddress sockAddr("192.168.137.100", "5683");
		coap_address_t coapAddr;
		coapAddr.size = sockAddr.length();
		std::memcpy(&coapAddr.addr.sin, sockAddr.addr(), sockAddr.length());

		pCoapContext = coap_new_context(nullptr);
		if (!pCoapContext) throw Poco::IOException("cannot create CoAP context");

		/* See coap_block(3) */
		coap_context_set_block_mode(pCoapContext,
									COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);

		// coap_fd = coap_context_get_coap_fd(pCoapContext);
		// if (coap_fd == -1) {
		// 	exit(1);
		// }
		// epoll_fd = epoll_create1(0);
		// if (epoll_fd == -1) {
		// 	exit(2);
		// }
		// ev.events = EPOLLIN;
		// ev.data.fd = coap_fd;
		// if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, coap_fd, &ev) == -1) {
		// 	exit(3);
		// }
		coap_endpoint_t* pCoapEndpoint = coap_new_endpoint(pCoapContext, &coapAddr, COAP_PROTO_UDP);
		if (!pCoapEndpoint) throw Poco::IOException("cannot create CoAP endpoint");

		// coap_str_const_t uri = { 8, reinterpret_cast<const uint8_t*>("property") };
		// coap_resource_t* pCoapResource = coap_resource_init(&uri, 0);

		/* Create a resource to handle PUTs to unknown URIs */
		coap_resource_t* pCoapResource = coap_resource_unknown_init2(CoAP_GetHandler, 0);
		coap_register_request_handler(pCoapResource, COAP_REQUEST_GET, CoAP_GetHandler);
		coap_register_request_handler(pCoapResource, COAP_REQUEST_PUT, CoAP_PutHandler);
		// coap_register_handler(pCoapResource, COAP_REQUEST_GET,CoAP_PropertyHandler);
		// coap_register_handler(pCoapResource, COAP_REQUEST_GET,
		// 	[](coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t* pResponse)
		// 	{
		// 		static Poco::Random rnd;

        //         coap_pdu_set_code(pResponse, COAP_RESPONSE_CODE_CONTENT);
		// 		// pResponse->code = COAP_RESPONSE_CODE(205);
		// 		int rssi = -50 - rnd.next(20);
		// 		std::string data = Poco::NumberFormatter::format(rssi);
		// 		coap_add_data(pResponse, data.size(), reinterpret_cast<const uint8_t*>(data.data()));
		// 	});

		coap_add_resource(pCoapContext, pCoapResource);
		CoAP_pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"CoAP Server Initialized");
		// if (pthread_mutex_init(&CoAP_Receive_Mutex, NULL) != 0) {
		// 	printf("\n mutex init has failed\n");
		// }
		// pthread_create (&CoAP_Thread, NULL, CoAP_Process, NULL);
		// for (;;)
		// {
		// 	// coap_run_once(pCoapContext, 0);
		// 	CoAP_Process();
		// }
	}
	catch (Poco::Exception& exc)
	{
		std::cerr << exc.displayText() << std::endl;
	}

	// if (pCoapContext) coap_free_context(pCoapContext);

	// coap_cleanup();

	// return 0;
}

BindCoAPServer::BindCoAPServer(void)
{

    

}

void BindCoAPServer::Start(void)
{
	pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_COAPSERVER,"CoAP Server starting");
}

void BindCoAPServer::Stop(void)
{

}