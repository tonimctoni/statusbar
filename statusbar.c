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

    if (memlen<=number_size+3)
        return;

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
    char buffer[1025];
    const char * const str_end=buffer+1025;
    for(;;){
        char *str=buffer;
        *str=' ', str++;
        get_available_memory(str, str_end-str),          str+=strlen(str);
        safe_strcpy(str, str_end-str, " \xe2\x97\x8c "), str+=strlen(str);
        get_host(str, str_end-str, "eth0", "wlan0"),     str+=strlen(str);
        safe_strcpy(str, str_end-str, " \xe2\x97\x8c "), str+=strlen(str);
        get_datetime(str, str_end-str),                  str+=strlen(str);
        if (str_end-str>2) *str=' ', *(str+1)='\0';

        XStoreName(dpy, DefaultRootWindow(dpy), buffer);
        XSync(dpy, False);
        sleep(1);
    }

    XCloseDisplay(dpy);

    return 0;
}

