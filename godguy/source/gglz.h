// Decoder for byte streams packed by tools/lz.py.
#ifndef GGLZ_H
#define GGLZ_H
static const unsigned char *ggUnpack(const unsigned char *src,unsigned char *dst,int size){
    int out=0;
    while(out<size){
        int flags=*src++|0x100;
        while(flags>1&&out<size){
            if(flags&1){
                dst[out++]=*src++;
            } else {
                int a=*src++, b=*src++;
                int offset=((a<<4)|(b>>4))+1;
                int length=(b&15)+3;
                if(length==18) length+=*src++;
                while(length--&&out<size){
                    dst[out]=dst[out-offset];
                    out++;
                }
            }
            flags>>=1;
        }
    }
    return src;
}
#endif
