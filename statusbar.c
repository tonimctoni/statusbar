#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <X11/Xlib.h>
#include <unistd.h>

#define ENABLE_MEM 1
#define ENABLE_BAT 1
#define ENABLE_NET 0

#if ENABLE_NET
    #define INTERFACE_NAME "eth0"
    #define DOWN_BYTES_PATH "/sys/class/net/"INTERFACE_NAME"/statistics/rx_bytes"
#endif
#if ENABLE_BAT
    #define BAT_NAME "BAT0"
    #define BAT_PATH_STATUS "/sys/class/power_supply/"BAT_NAME"/status"
    #define BAT_PATH_CAPACITY "/sys/class/power_supply/"BAT_NAME"/capacity"
#endif

void safe_strcpy(char *dst, const int dst_len, const char *src){
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

int read_file(char *buffer, const int buffer_len, const char *filename){
    int s;
    FILE *f=fopen(filename, "rb");
    if (f==0) return -1;
    s=fread(buffer, sizeof(char), buffer_len-1, f);
    buffer[s]=0;
    fclose(f);
    return s;
}

#if ENABLE_NET
void get_host(char *host, const int host_len){
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr)==-1){
        safe_strcpy(host, host_len, "0.0.0.0");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
        if (!ifa->ifa_addr) continue;
        if (!(ifa->ifa_addr->sa_family==AF_INET || ifa->ifa_addr->sa_family==AF_INET6)) continue;
        if (strcmp(ifa->ifa_name, INTERFACE_NAME)!=0) continue;
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

long int get_net_down_bytes(){
    char down_bytes_buffer[256];
    const int s=read_file(down_bytes_buffer, 256, DOWN_BYTES_PATH);
    if (s<0) return -1;

    return strtol(down_bytes_buffer, NULL, 10);
}

#define ROLLING_DIFF_LEN 4
typedef struct {
    long int values[ROLLING_DIFF_LEN];
    int i;
} RollingDiff;
RollingDiff get_net_down_internal_ra;

void init_rolling_average(RollingDiff *rd, long int x){
    rd->i=0;
    for(int i=0;i<ROLLING_DIFF_LEN;i++)
        rd->values[i]=x;
}

long int roll_diff(RollingDiff *rd, long int x){
    rd->i=(rd->i+1)%ROLLING_DIFF_LEN;
    const long int result=x-rd->values[rd->i];
    rd->values[rd->i]=x;
    return result;
}

void get_net_down(char *to, const int tolen){
    const long int down_bytes=get_net_down_bytes();
    if (down_bytes<=0){
        safe_strcpy(to, tolen, "error");
        return;
    }

    const long int result=roll_diff(&get_net_down_internal_ra, down_bytes)/1024/(ROLLING_DIFF_LEN);
    snprintf(to, tolen, "%li kB/s\xe2\x96\xbc", result);
}
#endif

#if ENABLE_MEM
void get_available_memory(char *mem, int memlen){
    char procmeminfo[1024*2];
    const int s=read_file(procmeminfo, 1024*2, "/proc/meminfo");
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

    if (memlen<=number_size+3)
        return;

    memcpy(mem, start_of_number, number_size);
    strcpy(mem+number_size, " MB");

    return;
}
#endif

#if ENABLE_BAT
void get_power_supply(char *ps, int pslen){
    const char *psend = ps + pslen;
    int readlen;

    readlen=read_file(ps, psend-ps, BAT_PATH_STATUS);
    if (readlen<1)
        return;
    ps+=readlen-1;

    safe_strcpy(ps, psend-ps, ": "), ps+=strlen(ps);
    readlen=read_file(ps, psend-ps, BAT_PATH_CAPACITY);
    if (readlen<1)
        return;
    ps[readlen-1]='\0';
}
#endif

void get_datetime(char *datetime, const int datetimelen){
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
#define BUFFER_SIZE 1025
int main(){
    Display *dpy=XOpenDisplay(NULL);
    if (!dpy) return 0;

    char buffer[BUFFER_SIZE];
    #if ENABLE_NET
    init_rolling_average(&get_net_down_internal_ra, 0);
    #endif
    const char * const str_end=buffer+BUFFER_SIZE;
    for(;;){
        char *str=buffer;
        *str=' ', str++;
        #if ENABLE_MEM
        get_available_memory(str, str_end-str),          str+=strlen(str);
        safe_strcpy(str, str_end-str, " \xe2\x97\x8c "), str+=strlen(str);
        #endif
        #if ENABLE_BAT
        get_power_supply(str, str_end-str),              str+=strlen(str);
        safe_strcpy(str, str_end-str, " \xe2\x97\x8c "), str+=strlen(str);
        #endif
        #if ENABLE_NET
        get_host(str, str_end-str),                      str+=strlen(str);
        safe_strcpy(str, str_end-str, ": "),             str+=strlen(str);
        get_net_down(str, str_end-str),                  str+=strlen(str);
        safe_strcpy(str, str_end-str, " \xe2\x97\x8c "), str+=strlen(str);
        #endif
        get_datetime(str, str_end-str),                  str+=strlen(str);
        if (str_end-str>2) *str=' ', *(str+1)='\0';

        XStoreName(dpy, DefaultRootWindow(dpy), buffer);
        XSync(dpy, False);
        sleep(1);
    }

    XCloseDisplay(dpy);

    return 0;
}
