project(wot-binding-rpi)

add_library(${PROJECT_NAME} 
            ./BindClient.cpp
            ./BindServer.cpp
            ./http/Handlers/HttpActionHndl.cpp
            ./http/Handlers/HttpAffordanceHndl.cpp
            ./http/Handlers/HttpEvntObserveHndl.cpp
            ./http/Handlers/HttpPropertyHndl.cpp
            ./http/Handlers/HttpPropObserveHndl.cpp
            ./http/Handlers/HttpThingsServer.cpp
            ./CoAP/BindCoAPServer.cpp
            ./http/BindHttpServer.cpp)

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
# target_include_directories(IP)

target_include_directories(${PROJECT_NAME}
	PUBLIC
        ./
		./http
		./CoAP
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		# WoTCore
		nlohmann_json_schema_validator
		libcoap::coap-3
		${POCO_LIBRARIES}
		cpp-logger
	)