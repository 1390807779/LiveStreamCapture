#include <Packet.h>
extern "C"
{
#include<libavformat/avformat.h>
}

Packet::Packet(AVPacket *_packet):
    packet(av_packet_clone(_packet)),
    isOutput(false)
{
}

Packet::~Packet()
{
    if (packet)
    {
        av_packet_unref(packet);
        av_packet_free(&packet);
    }
    
}