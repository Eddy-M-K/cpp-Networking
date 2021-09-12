#include <iostream>
#include <kim_net.h>

enum class CustomMsgTypes : uint32_t
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};

class CustomServer : public kim::net::server_interface<CustomMsgTypes>
{
public:
    CustomServer(uint16_t nPort) : kim::net::server_interface<CustomMsgTypes>(nPort)
    {

    }

protected:
    virtual bool OnClientConnect(std::shared_ptr<kim::net::connection<CustomMsgTypes>> client)
    {
        return true;
    }

    // Called when a client appears to have disconnected
    virtual void OnClientDisconnect(std::shared_ptr<kim::net::connection<CustomMsgTypes>> client)
    {

    }

    // Called when a message arrives
    virtual void OnMessage(std::shared_ptr<kim::net::connection<CustomMsgTypes>> client, kim::net::message<CustomMsgTypes>& msg)
    {

    }
};

int main()
{
    CustomServer server(60000);
    server.Start();

    while (true)
    {
        server.Update();
    }

    return 0;
}
