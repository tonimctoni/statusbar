#include <stdio.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <X11/Xlib.h>
#include <unistd.h>

void safe_strcpy(char *dst, int dst_len, char *src){
    if (!dst || !src){
        return;
    }

    if (dst_len>strlen(src)){
        strcpy(dst,src);
        return;
    }

    if (dst_len>0){
        *dst=0;
        return;
    }

    return;
}

// void join_strs(char *dst, int dst_len, const char * const inbetween, const char **strs){
//     *dst='\0';
//     if (*strs==NULL){
//         return;
//     }
// 
//     const int inbetween_size=strlen(inbetween);
//     while (*(strs+1)!=NULL){
//         const int str_size=strlen(*strs);
//         if (str_size+inbetween_size>=dst_len)
//             return;
// 
//         strcpy(dst, *strs);
//         strcpy(dst+str_size, inbetween);
//         dst+=str_size+inbetween_size;
//         dst_len+=str_size+inbetween_size;
//         strs++;
//     }
// 
//     const int str_size=strlen(*strs);
//     if (str_size>=dst_len)
//         return;
//     strcpy(dst, *strs);
// }

// Not checking for dst_len>0
void join_three_strs(char *dst, const int dst_len, const char * const inbetween, const char *const str1, const char *const str2, const char *const str3){
    const int inbetween_size=strlen(inbetween);
    const int str1_size=strlen(str1);
    const int str2_size=strlen(str2);
    const int str3_size=strlen(str3);

    if (inbetween_size*2+str1_size+str2_size+str3_size+2>=dst_len){
        *dst='\0';
        return;
    }

    *dst=' ',                               dst+=1;
    memcpy(dst, str1, str1_size),           dst+=str1_size;
    memcpy(dst, inbetween, inbetween_size), dst+=inbetween_size;
    memcpy(dst, str2, str2_size),           dst+=str2_size;
    memcpy(dst, inbetween, inbetween_size), dst+=inbetween_size;
    memcpy(dst, str3, str3_size),           dst+=str3_size;
    *dst=' ',                               *(dst+1)=0;
}


void get_host(char *host, int host_len, const char *interface_name, const char *other_interface_name){
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr)==-1){
        safe_strcpy(host, host_len, "0.0.0.0");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
        if (!ifa->ifa_addr) continue;
        if (!(ifa->ifa_addr->sa_family==AF_INET || ifa->ifa_addr->sa_family==AF_INET6)) continue;
        if (strcmp(ifa->ifa_name, interface_name)!=0 && strcmp(ifa->ifa_name, other_interface_name)!=0) continue;
        int s;
        int ifa_addr_len;

        ifa_addr_len=(ifa->ifa_addr->sa_family==AF_INET)
        ?sizeof(struct sockaddr_in)
        :sizeof(struct sockaddr_in6);

        s=getnameinfo(ifa->ifa_addr, ifa_addr_len, host, host_len, 0, 0, NI_NUMERICHOST);
        host[host_len-1]=0;
        if (s!=0) continue;
        else break;
    }

    freeifaddrs(ifaddr);

    if(ifa==NULL){
        safe_strcpy(host, host_len, "0.0.0.0");
    }

    return;
}

int read_file(char *buffer, int buffer_len, const char *filename){
    int s;
    FILE *f=fopen(filename, "rb");
    if (f==0) return -1;
    s=fread(buffer, sizeof(char), buffer_len-1, f);
    buffer[s]=0;
    fclose(f);
    return s;
}

void get_available_memory(char *mem, int memlen){
    char procmeminfo[1024*2];
    const int s=read_file(procmeminfo, 2014*2, "/proc/meminfo");
    if (s<0){
        safe_strcpy(mem, memlen, "0 kB");
        return;
    }

    const char *const found_line_start=strstr(procmeminfo, "MemAvailable:");
    if (found_line_start==NULL){
        safe_strcpy(mem, memlen, "0 kB");
        return;
    }

    const char *const found_line_end=strchr(found_line_start, '\n');
    if (found_line_end==NULL){
        safe_strcpy(mem, memlen, "0 kB");
        return;
    }

    if (strncmp(found_line_end-3, " kB", 3)!=0){
        safe_strcpy(mem, memlen, "0 kB");
        return;
    }

    const char *str=found_line_start+strlen("MemAvailable:");
    while (*str==' ') str++;
    const char *const start_of_number=str;

    const int number_size=found_line_end-start_of_number-3-3;
    if (number_size<1 || number_size>100){
        safe_strcpy(mem, memlen, "0 MB");
        return;
    }

    memcpy(mem, start_of_number, number_size);
    strcpy(mem+number_size, " MB");

    return;
}

void get_datetime(char *datetime, int datetimelen){
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    int s=strftime(datetime, datetimelen, "%A, %d-%b-%g %H:%M:%S", timeinfo);
    if (!s) safe_strcpy(datetime, datetimelen, "00000, 00-00-00 00:00:00");
    else datetime[datetimelen-1]=0;
}

// gcc -pedantic -Wall -O2 A.c -lX11
// tcc A.c -lX11
int main(){
    Display *dpy=XOpenDisplay(NULL);
    if (!dpy) return 0;
    char mem[1025];
    char host[1025];
    char datetime[1025];
    char str[1025];
    for(;;){
        get_available_memory(mem, 1025);
        get_host(host, 1025, "eth0", "wlan0");
        get_datetime(datetime, 1025);

        join_three_strs(str, 1025, " \xe2\x97\x8c ", mem, host, datetime);
        XStoreName(dpy, DefaultRootWindow(dpy), str);
        XSync(dpy, False);
        sleep(1);
    }

    XCloseDisplay(dpy);

    return 0;
}
