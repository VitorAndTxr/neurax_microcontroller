#ifndef SESSION_MODULE
#define SESSION_MODULE

struct SessionDetails
{
    short maximumStimuliAmount;
    short completeStimuliAmount;
    short interruptedStimuliAmount;
    short minimumSecondsBetweenStimuli;
};


class Session
{
private:
    Fes fes;
    Semg semg;
    SessionDetails details;
public:
    Session(/* args */);
    ~Session();
    start();
    stop();
    
};

Session::Session(/* args */)
{
}

Session::~Session()
{
}



#endif