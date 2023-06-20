class Clock
{
private:
    double pts;
    double updateTime;
    double ptsDrift;
public:
    Clock();
    ~Clock();
    void updateClock(double _pts, double _updateTime, double _ptsDrift);
    double getClock();
};
