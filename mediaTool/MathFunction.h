class MathFunction
{
private:
    
public:
    MathFunction();
    ~MathFunction();
    int AVLog(unsigned int num);
    int AVCeilRightShift(int ceil, int step);
};

class PIDFunction
{
private:
    double p;
    double i;
    double d;
    double kp;
    double ki;
    double kd;
};
