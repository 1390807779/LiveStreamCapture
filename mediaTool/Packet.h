struct AVPacket;

class Packet
{
public:
    AVPacket *packet;
    Packet(AVPacket *_packet);
    bool isOutput;
    ~Packet();
};
