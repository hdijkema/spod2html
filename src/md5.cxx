#include <md5.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static
std::string mkmd5(const std::string & s)
{
    md5_state_t st;
    md5_byte_t  dig[16];
        md5_init(&st);
        md5_append(&st,(md5_byte_t *) s.c_str(),s.size());
        md5_finish(&st,dig);
        {int i;
         char hex[33],*p=hex;
            for(i=0;i<16;i++) {
                sprintf(p,"%02x",dig[i]);
                p+=2;
            }
            return std::string(hex);
        }
}

md5::md5(const std::string & s) {
	md5_hex=mkmd5(s);
}

void md5::operator = (const std::string & s) {
	md5_hex=mkmd5(s);
}
        
std::string md5::hex(void) {
    return md5_hex;
}


