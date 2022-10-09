//
//  Controller.cpp
//  Controller
//
//  Created by FireWolf on 3/12/21.
//  Revised by FireWolf on 2/21/22.
//      - Controller V2 Implementation.
//      - Use C++ thread primitives instead of pthread ones.
//      - Replace dependent C libraries with C++ ones.
//

#include "Controller.hpp"
#include "Debug.hpp"
#include "CoAP.hpp"
#include <iostream>

///
/// Split the given string into an array of tokens
///
/// @param string The string to split
/// @param delimiter The delimiter to use to split the string
/// @return The split tokens.
///
static std::vector<std::string> split(const std::string& string, const std::string& delimiter)
{
    std::vector<std::string> tokens;

    size_t start = 0, end;

    while (true)
    {
        end = string.find(delimiter, start);

        if (end == std::string::npos)
        {
            break;
        }

        tokens.push_back(string.substr(start, end - start));

        start = end + delimiter.length();
    }

    tokens.push_back(string.substr(start, end));

    return tokens;
}

///
/// Remove the leading and trailing white spaces from the given string
///
/// @param string A string
/// @param whitespaces The whitespaces
/// @return A new string without the leading and trailing white spaces.
///
static std::string trim(const std::string& string, const std::string& whitespaces = " \t\r\n")
{
    auto start = string.find_first_not_of(whitespaces);

    if (start == std::string::npos)
    {
        return "";
    }

    auto end = string.find_last_not_of(whitespaces);

    return string.substr(start, end - start + 1);
}

//
// MARK: - Background Threads
//

/// The sender thread implementation
void Controller::sender()
{
    while (true)
    {
        auto command = this->queue.poll();

        if (this->sockets[command.index])
        {
            psoftassert(this->sockets[command.index]->send(command.message),
                        "Failed to send the message to the %s device.",
                        SocketIndex2String(command.index));
        }
        else
        {
            pwarning("Ignore messages sent to the %s device that is not connected.",
                     SocketIndex2String(command.index));
        }
    }
}

///
/// The receiver thread implementation
///
/// @param index The index of the socket from which to receive data
///
void Controller::receiver(SocketIndex index)
{
    passert(this->sockets[index], "The socket should be connected.");

    uint8_t buffer[32] = {};

    this->receiveGarbageDataFromFastModels(index);

    // Run loop
    while (true)
    {
        // Clear the buffer
        bzero(buffer, sizeof(buffer));

        // Receive a message from the designated socket
        if (!this->sockets[index]->receiveWithLength(buffer, sizeof(Message)))
        {
            perr("Failed to receive the message from the %s device.", SocketIndex2String(index));

            break;
        }

        auto message = reinterpret_cast<Message*>(buffer);

        if (message->magic != 0x4657)
        {
            perr("Received an invalid message from the %s device: Magic Mismatched.", SocketIndex2String(index));

            continue;
        }

        switch (message->type)
        {
            case Message::Type::kMoistureUserStack:
            {
                // Received the user stack pointer address
                status("Moisture device reports that the shared user stack starts at 0x%08x.", message->data);

                break;
            }

            case Message::Type::kActuatorUserStack:
            {
                // Received the user stack pointer address
                status("Actuator device reports that the shared user stack starts at 0x%08x.", message->data);

                break;
            }

            case Message::Type::kGateWayUserStack:
            {
                // Received the user stack pointer address
                status("Gateway device reports that a thread stack starts at 0x%08x.", message->data);

                break;
            }

            case Message::Type::kSoilDryAlert:
            {
                // Relay to the actuator device
                status("The controller has received a Soil Dry Alert message from the sensor device.\n");

                this->queue.offer(Command::relayMessageToActuatorDevice(*message));

                break;
            }

            case Message::Type::kSoilWetAlert:
            {
                // Relay to the actuator device
                status("The controller has received a Soil Wet Alert message from the sensor device.\n");

                this->queue.offer(Command::relayMessageToActuatorDevice(*message));

                break;
            }

            case Message::Type::kAckSoilWet:
            {
                // Relay to the sensor device
                status("The controller has received a Ack Soil Wet message from the actuator device.\n");

                this->queue.offer(Command::relayMessageToSensorDevice(*message));

                break;
            }

            case Message::Type::kRunOutOfWaterAlert:
            {
                status("The controller has received a Run Out Of Water Alert message from the actuator device.\n");

                break;
            }

            default:
            {
                perr("Message type is [%s]. Should never reach at here.", Message::Type2String(static_cast<Message::Type>(message->type)));

                break;
            }
        }
    }
}

///
/// Print the controller status
///
/// @param format The format string
///
void Controller::status(const char* format, ...)
{
    char buffer[64] = {};

    va_list args;

    va_start(args, format);

    flockfile(stdout);

    time_t rawtime;

    time(&rawtime);

    struct tm* timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);

    printf("\n");

    printf("%s: \n", buffer);

    vprintf(format, args);

    printf("\n");

    funlockfile(stdout);

    va_end(args);
}

///
/// Create a CoAP request message
///
/// @param buffer A buffer that contains the CoAP message on return
/// @param moisture The moisture level
///
void Controller::makeCoAPRequestMessage(uint8_t (&buffer)[32], uint32_t moisture)
{
    uint8_t* ptr = buffer;

    // Construct the header (4 bytes)
    auto header = reinterpret_cast<CoAPHeader*>(ptr);

    header->version = 0x01;

    header->type = 0x01;

    header->tokenLength = 0x00;

    header->code = kPost;

    header->msgID = 0x4657;

    ptr += sizeof(CoAPHeader);

    // Construct the option - host address (10 bytes)
    auto address = "localhost";

    auto length = strlen(address);

    *ptr = kURIHost << 4 | length;

    memcpy(ptr + 1, address, length);

    ptr += 1 + length;

    // Construct the option - host port (3 bytes)
    *ptr = (kURIPort - kURIHost) << 4 | 2;

    *reinterpret_cast<uint16_t*>(ptr + 1) = htons(10086);

    ptr += 1 + sizeof(uint16_t);

    // Construct the option - query path (10 bytes)
    auto path = "/moisture";

    length = strlen(path);

    *ptr = (kURIPath - kURIPort) << 4 | length;

    memcpy(ptr + 1, path, length);

    ptr += 1 + length;

    // End of option marker (1 byte)
    *ptr = 0xFF;

    ptr += 1;

    // Payload data (4 bytes)
    *reinterpret_cast<uint32_t*>(ptr) = moisture;

    ptr += sizeof(uint32_t);

    passert(ptr - buffer == 32, "Check the request size.");
}

///
/// Send a CoAP request message to the gateway device and receive the translated HTTP request message
///
/// @param request The CoAP request message
/// @param response A non-null buffer that stores the translated HTTP request message on return
/// @param length The number of bytes that the response buffer can hold
///
void Controller::sendRecvCoAPMessage(uint8_t (&request)[32], uint8_t* response, size_t length)
{
    passert(this->sockets[kGateway]->send(request, sizeof(request)), "Failed to send the CoAP request message.");

    passert(this->sockets[kGateway]->receiveWithLength(response, length), "Failed to receive the HTTP message.");
}

///
/// Send a CoAP request message to the gateway device and receive the translated HTTP request message conveniently
///
void Controller::sendRecvCoAPMessageOnce()
{
    uint8_t request[32] = {}, response[64] = {};

    this->makeCoAPRequestMessage(request, 100);

    this->sendRecvCoAPMessage(request, response, 54);

    status("Received a HTTP request message:");

    printf("%s\n", response);
}

///
/// Send multiple CoAP request messages and receive translated HTTP request messages to measure the round trip time in nanoseconds
///
/// @param trials Specify the number of trails
/// @param delayMS Specify the amount of time in milliseconds to wait until the next trial
/// @return The experiment result.
///
ExecutionTimeMeasurer::Result Controller::sendRecvCoAPMessages(size_t trials, uint64_t delayMS)
{
    uint8_t request[32], response[64];

    Controller::makeCoAPRequestMessage(request, 100);

    return ExecutionTimeMeasurer{}(trials, std::chrono::milliseconds(delayMS), &Controller::sendRecvCoAPMessage, this, request, response, 54);
}

///
/// Run the gateway experiment
///
/// @param trials Specify the number of trails
/// @param delayMS Specify the amount of time in milliseconds to wait until the next trial
///
void Controller::runGatewayExperiment(size_t trials, uint64_t delayMS)
{
    printf("Running the gateway experiment...\n");

    printf("\tTrials = %zu; Delay = %llu milliseconds.\n", trials, delayMS);

    auto result = this->sendRecvCoAPMessages(trials, delayMS);

    printf("Execution time:\n");

    printf("- Min = %llu nanoseconds.\n", result.min());

    printf("- Max = %llu nanoseconds.\n", result.max());

    printf("- Med = %llu nanoseconds.\n", result.medium());

    printf("- Avg = %.2f nanoseconds.\n", result.mean());

    printf("- Std = %.2f nanoseconds.\n", result.sd());
}

///
/// Receive 15-byte garbage data from the FastModels at the beginning
///
/// @param index The index of the socket from which to receive garbage data
///
void Controller::receiveGarbageDataFromFastModels(SocketIndex index)
{
    uint8_t buffer[16] = {};

    const char* name = SocketIndex2String(index);

    pinfo("Receiving 15-byte garbage data from the %s device emulated by the ARM FastModels.", name);

    passert(this->sockets[index], "The socket to the %s device does not exist.", name);

    if (this->sockets[index]->receiveWithLength(buffer, 15))
    {
        pinfo("Received 15-byte garbage data from the %s device.", name);
    }
    else
    {
        pwarning("Failed to receive the garbage data from the %s device.", name);

        pwarning("The controller may not function properly.");
    }
}

/// Run the controller
int Controller::run()
{
    std::thread sender(&Controller::sender, this);

    std::thread mReceiver, aReceiver;

    if (this->sockets[SocketIndex::kMonitor])
    {
        mReceiver = std::thread(&Controller::receiver, this, SocketIndex::kMonitor);
    }

    if (this->sockets[SocketIndex::kActuator])
    {
        aReceiver = std::thread(&Controller::receiver, this, SocketIndex::kActuator);
    }

    if (this->sockets[SocketIndex::kGateway])
    {
        this->receiveGarbageDataFromFastModels(SocketIndex::kGateway);
    }

    // Wait for the user command
    std::string input;

    while (true)
    {
        printf("Commander > ");

        // Read the user input
        std::getline(std::cin, input);

        std::vector<std::string> args = split(trim(input), " ");

        const std::string& command = args[0];

        // Guard: Check the empty line
        if (command.empty())
        {
            continue;
        }

        // Parse the client command
        if (command == "exit")
        {
            printf("Goodbye.\n");

            break;
        }
        else if (command == "soil")
        {
            if (args.size() != 2)
            {
                printf("Usage: soil level\n");

                printf("e.g. `soil 30` to set the moisture level to 30%%.\n");
            }
            else
            {
                this->queue.offer(Command::changeSoilMoisture(std::stoi(args[1])));
            }
        }
        else if (command == "water")
        {
            if (args.size() != 2)
            {
                printf("Usage: water status\n");

                printf("e.g. `water 1` to fill the bottle with water.\n");

                printf("     `water 0` to empty the bottle.\n");
            }
            else
            {
                this->queue.offer(Command::changeWaterStatus(std::stoi(args[1])));
            }
        }
        else if (command == "dry")
        {
            this->queue.offer(Command::sendDrySoilAlertToActuatorDevice());
        }
        else if (command == "wet")
        {
            this->queue.offer(Command::sendWetSoilAlertToActuatorDevice());
        }
        else if (command == "coap")
        {
            this->sendRecvCoAPMessageOnce();
        }
        else if (command == "gateway")
        {
            if (args.size() != 3)
            {
                printf("Usage: gateway trials delay\n");

                printf("where `trials` specify the number of trials;\n");

                printf("      `delay` specify the amount of time in milliseconds between each trial.\n");
            }
            else
            {
                this->runGatewayExperiment(std::stoll(args[1]), std::stoll(args[2]));
            }
        }
        else
        {
            printf("Unknown command: [%s].\n", command.c_str());
        }
    }

    return 0;
}
