#ifndef BLUETOOTH_MODULE
#define BLUETOOTH_MODULE


class Bluetooth
{
private:
    //static Bluetooth* bluetooth_instance;

    bool connected;
public:
    Bluetooth(/* args */);
    ~Bluetooth();
};

Bluetooth::Bluetooth(/* args */)
{
    this->connected = false;
}

Bluetooth::~Bluetooth()
{
}


#endif