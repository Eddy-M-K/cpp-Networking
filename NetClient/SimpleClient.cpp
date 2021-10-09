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

class CustomClient : public kim::net::client_interface<CustomMsgTypes>
{
public:
    
};

int main()
{
    CustomClient c;
    c.Connect("127.0.0.1", 60000);
    return 0;

    /*kim::net::message<CustomMsgTypes>msg;
    msg.header.id = CustomMsgTypes::FireBullet;

    int a = 1;
    bool b = true;
    float c = 3.14159f;

    struct
    {
        float x;
        float y;
    } d[5];

    msg << a << b << c << d;

    a = 99;
    b = false;
    c = 99.0f;

    msg >> d >> c >> b >> a;

    return 0;*/
}
