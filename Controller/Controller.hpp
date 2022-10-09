//
//  Controller.hpp
//  Controller
//
//  Created by FireWolf on 3/12/21.
//  Revised by FireWolf on 2/21/22.
//      - Controller V2 Implementation.
//      - Use C++ thread primitives instead of pthread ones.
//      - Replace dependent C libraries with C++ ones.
//

#ifndef Controller_hpp
#define Controller_hpp

#include "LinkedBlockingQueue.hpp"
#include "StreamSocket.hpp"
#include "Message.hpp"
#include "Experiments.hpp"

class Controller
{
private:
    /// Socket indices
    enum SocketIndex: size_t
    {
        kMonitor  = 0,
        kActuator = 1,
        kGateway  = 2,
    };

    /// Get the string representation of the given socket index
    static inline const char* SocketIndex2String(SocketIndex index)
    {
        switch (index)
        {
            case SocketIndex::kMonitor:
                return "Monitor";

            case SocketIndex::kActuator:
                return "Actuator";

            case SocketIndex::kGateway:
                return "Gateway";
        }
    }

    /// Command
    struct Command
    {
        /// Message to be sent
        Message message;

        /// Index of the destination socket
        SocketIndex index;

        /// Create a command
        Command(Message message, SocketIndex index) : message(message), index(index) {}

        static Command changeSoilMoisture(UInt32 level)
        {
            return { Message::changeSoilMoisture(level), SocketIndex::kMonitor };
        }

        static Command changeWaterStatus(bool hasWater)
        {
            return { Message::changeWaterStatus(hasWater), SocketIndex::kActuator };
        }

        static Command relayMessageToSensorDevice(const Message& message)
        {
            return { message, SocketIndex::kMonitor };
        }

        static Command relayMessageToActuatorDevice(const Message& message)
        {
            return { message, SocketIndex::kActuator };
        }

        static Command sendDrySoilAlertToActuatorDevice()
        {
            return { Message::soilDryAlert(), SocketIndex::kActuator };
        }

        static Command sendWetSoilAlertToActuatorDevice()
        {
            return { Message::soilWetAlert(), SocketIndex::kActuator };
        }
    };

private:
    /// Sockets used to communicate with the moisture, actuator and gateway devices
    std::optional<StreamSocket> sockets[3];

    /// Command queue for the sender thread
    LinkedBlockingQueue<Command> queue;

    //
    // MARK: - Constructor & Destructor
    //

public:
    ///
    /// Create the controller with device sockets
    ///
    /// @param monitor An optional socket to communicate with the monitor device
    /// @param actuator An optional socket to communicate with the actuator device
    /// @param gateway An optional socket to communicate with the gateway device
    ///
    Controller(std::optional<StreamSocket> monitor, std::optional<StreamSocket> actuator, std::optional<StreamSocket> gateway)
    {
        this->sockets[SocketIndex::kMonitor] = std::move(monitor);

        this->sockets[SocketIndex::kActuator] = std::move(actuator);

        this->sockets[SocketIndex::kGateway] = std::move(gateway);
    }

    //
    // MARK: - Background Threads
    //

    /// The sender thread implementation
    [[noreturn]] void sender();

    ///
    /// The receiver thread implementation
    ///
    /// @param index The index of the socket from which to receive data
    ///
    void receiver(SocketIndex index);

    ///
    /// Print the controller status
    ///
    /// @param format The format string
    ///
    static void status(const char* format, ...);

    //
    // MARK: - CoAP-HTTP Gateway
    //

    ///
    /// Create a CoAP request message
    ///
    /// @param buffer A buffer that contains the CoAP message on return
    /// @param moisture The moisture level
    ///
    static void makeCoAPRequestMessage(uint8_t (&buffer)[32], uint32_t moisture);

    ///
    /// Send a CoAP request message to the gateway device and receive the translated HTTP request message
    ///
    /// @param request The CoAP request message
    /// @param response A non-null buffer that stores the translated HTTP request message on return
    /// @param length The number of bytes that the response buffer can hold
    ///
    void sendRecvCoAPMessage(uint8_t (&request)[32], uint8_t* response, size_t length);

    ///
    /// Send a CoAP request message to the gateway device and receive the translated HTTP request message conveniently
    ///
    void sendRecvCoAPMessageOnce();

    ///
    /// Send multiple CoAP request messages and receive translated HTTP request messages to measure the round trip time in nanoseconds
    ///
    /// @param trials Specify the number of trails
    /// @param delayMS Specify the amount of time in milliseconds to wait until the next trial
    /// @return The experiment result.
    ///
    ExecutionTimeMeasurer::Result sendRecvCoAPMessages(size_t trials, uint64_t delayMS);

    ///
    /// Run the gateway experiment
    ///
    /// @param trials Specify the number of trails
    /// @param delayMS Specify the amount of time in milliseconds to wait until the next trial
    ///
    void runGatewayExperiment(size_t trials, uint64_t delayMS);

    //
    // MARK: - Main Controller
    //

    ///
    /// Receive 15-byte garbage data from the FastModels at the beginning
    ///
    /// @param index The index of the socket from which to receive garbage data
    ///
    void receiveGarbageDataFromFastModels(SocketIndex index);

    /// Run the controller
    int run();
};

#endif /* Controller_hpp */
