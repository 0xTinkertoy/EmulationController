//
//  main.cpp
//  Controller
//
//  Created by FireWolf on 2/23/21.
//  Revised by FireWolf on 2/21/22.
//      - Controller V2 Implementation.
//      - Use C++ thread primitives instead of pthread ones.
//      - Replace dependent C libraries with C++ ones.
//

#include <getopt.h>
#include "Controller.hpp"
#include "Debug.hpp"

int main(int argc, const char * argv[])
{
    // Command line options
    static option options[] =
    {
        { "moisture", optional_argument, nullptr, 'm' },
        { "actuator", optional_argument, nullptr, 'a' },
        { "gateway" , optional_argument, nullptr, 'g' },
        { nullptr, no_argument, nullptr, 0 },
    };

    // Parsed port numbers
    uint16_t pMonitor = 0, pActuator = 0, pGateway = 0;

    while (true)
    {
        int option = getopt_long(argc, const_cast<char**>(argv), "m:a:g:", options, nullptr);

        if (option == -1)
        {
            // Finished parsing
            break;
        }

        switch (option)
        {
            case 'm':
            {
                pMonitor = std::stoi(optarg);

                break;
            }

            case 'a':
            {
                pActuator = std::stoi(optarg);

                break;
            }

            case 'g':
            {
                pGateway = std::stoi(optarg);

                break;
            }

            case '?':
            {
                break;
            }

            default:
            {
                perr("Unrecognized option: %c.", option);

                break;
            }
        }
    }

    // Guard: Users must provide at least one port number
    if ((pMonitor | pActuator | pGateway) == 0)
    {
        perr("Must provide at least one port number.");

        return -1;
    }

    // Create sockets to communicate with devices
    std::optional<StreamSocket> monitor, actuator, gateway;

    try
    {
        // Create the TCP socket to communicate with the moisture device
        if (pMonitor != 0)
        {
            monitor.emplace(std::make_pair(INADDR_LOOPBACK, 0), std::make_pair(INADDR_LOOPBACK, pMonitor));
        }

        // Create the TCP socket to communicate with the actuator device
        if (pActuator != 0)
        {
            actuator.emplace(std::make_pair(INADDR_LOOPBACK, 0), std::make_pair(INADDR_LOOPBACK, pActuator));
        }

        // Create the TCP socket to communicate with the gateway device
        if (pGateway != 0)
        {
            gateway.emplace(std::make_pair(INADDR_LOOPBACK, 0), std::make_pair(INADDR_LOOPBACK, pGateway));
        }
    }
    catch (SocketException& exception)
    {
        perr("%s", exception.what());

        return -1;
    }

    // Create the controller and run it
    return Controller(std::move(monitor), std::move(actuator), std::move(gateway)).run();
}
