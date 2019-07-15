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

void get_host(char *host, int host_len, const char *interface_name){
    if (!host){
        return;
    }
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != -1){
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
            if (!ifa->ifa_addr) continue;
            if (!(ifa->ifa_addr->sa_family==AF_INET || ifa->ifa_addr->sa_family==AF_INET6)) continue;
            if (strcmp(ifa->ifa_name, interface_name)!=0) continue;
            int s;
            int ifa_addr_len;

            ifa_addr_len=(ifa->ifa_addr->sa_family==AF_INET)
            ?sizeof(struct sockaddr_in)
            :sizeof(struct sockaddr_in6);

            s=getnameinfo(ifa->ifa_addr, ifa_addr_len, host, host_len, 0, 0, NI_NUMERICHOST);
            host[host_len-1]=0;
            if (s!=0) continue;
            else {freeifaddrs(ifaddr);return;}
        }
        freeifaddrs(ifaddr);
    }

    safe_strcpy(host, host_len, "0.0.0.0");
    return;
}

int read_file(char *buffer, int buffer_len, const char *filename){
    if (!buffer || !filename){
        return -1;
    }
    int s;
    FILE *f=fopen(filename, "rb");
    if (f==0) return -1;
    s=fread(buffer, sizeof(char), buffer_len-1, f);
    buffer[s]=0;
    fclose(f);
    return s;
}

void get_available_memory(char *mem, int memlen){
    if (!mem){
        return;
    }
    char procmeminfo[1024*2];
    int s;
    char *str;
    s=read_file(procmeminfo, 2014*2, "/proc/meminfo");
    if (s<0){
        safe_strcpy(mem, memlen, "0 kB");
        return;
    }

    int memavailstrlen=strlen("MemAvailable:");
    for(str=procmeminfo;str && *str;str=strstr(str, "\n")+1){
        if(strncmp(str, "MemAvailable:", memavailstrlen)==0){
            char *end_of_line=strchr(str, '\n');
            if (!end_of_line){
                safe_strcpy(mem, memlen, "0 kB");
                return;
            }
            *end_of_line=0;
            str+=memavailstrlen;
            while (*str==' ') str++;
            int strstrlen=strlen(str);
            if (strstrlen<=6){
                safe_strcpy(mem, memlen, "0 MB");
                return;
            }
            if (strcmp(str+(strstrlen-3), " kB")!=0){
                safe_strcpy(mem, memlen, "0 kB");
                return;
            }
            *(str+(strstrlen-6))=0;
            strstrlen=strlen(str);
            safe_strcpy(mem, memlen, str);
            safe_strcpy(mem+strstrlen, memlen-strstrlen, " MB");
            break;
        }
    }
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
        get_host(host, 1025, "eth0");
        get_datetime(datetime, 1025);

        if (strlen(mem)+strlen(host)+strlen(datetime)<1000)
            sprintf(str, " %s \xe2\x97\x8c %s \xe2\x97\x8c %s ", mem, host, datetime);
        else
            strcpy(str, "[string too long]");
        XStoreName(dpy, DefaultRootWindow(dpy), str);
        XSync(dpy, False);
        sleep(1);
    }

    XCloseDisplay(dpy);

    return 0;
}
