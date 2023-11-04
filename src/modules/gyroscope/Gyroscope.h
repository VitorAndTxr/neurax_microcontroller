#ifndef GYROSCOPE_MODULE
#define GYROSCOPE_MODULE

typedef float angle;

class Gyroscope
{
private:
    static float last_value;
public:

    Gyroscope() = delete;
    ~Gyroscope() = delete;
    static void init();
    static angle acquire();
    static angle getLastValue();
    static void sendLastValue();

};

#endif