package main;

// #cgo LDFLAGS: -lX11
// #include <X11/Xlib.h>
import "C"

import "time"
import "net"
import "strings"
import "io/ioutil"

// type CpuUsageMeter struct{
//     last_cpu int64
//     last_time time.Time
// }

// func (c *CpuUsageMeter) get_cpu_time() string{
//     stat, err:=ioutil.ReadFile("/proc/stat")
//     if err!=nil{
//         return ""
//     }
//     stats:=string(stat)

//     line_end:=strings.Index(stats, "\n")
//     if line_end==-1{
//         return ""
//     }

//     cpu_fields:=strings.Fields(stats[:line_end])
//     if len(cpu_fields)<4 || cpu_fields[0]!="cpu"{
//         return ""
//     }

//     cpu_user,err:=strconv.ParseInt(cpu_fields[1], 10, 64)
//     if err!=nil{
//         return ""
//     }

//     cpu_system,err:=strconv.ParseInt(cpu_fields[3], 10, 64)
//     if err!=nil{
//         return ""
//     }

//     current_cpu:=cpu_user+cpu_system
//     current_time:=time.Now()
//     cpu_delta:=(current_cpu-c.last_cpu)*1000000000
//     time_delta:=current_time.Sub(c.last_time).Nanoseconds()
//     c.last_cpu=current_cpu
//     c.last_time=current_time

//     return strconv.FormatInt(cpu_delta/time_delta, 10)+"%"
// }

func get_mb_from_line(line string) string{
    fields:=strings.Fields(line)
    m:=fields[len(fields)-2]
    if len(m)>4{
        return m[:len(m)-3] + " MB"
    } else{
        return m + " KB"
    }
}

func get_mem_avail() string{
    meminfo,err:=ioutil.ReadFile("/proc/meminfo")
    if err!=nil{
        return ""
    }

    meminfo_lines:=strings.Split(string(meminfo), "\n")
    for _,line :=range meminfo_lines{
        if strings.HasPrefix(line, "MemAvailable"){
            return get_mb_from_line(line)
        }
    }

    return "0 B"
}

func get_datetime() string{
    return time.Now().Format("Monday, 02-Jan-06 15:04:05")
}

func get_local_ip_address() string{
    int, err:=net.InterfaceByName("eth0")
    if err!=nil{
        return "0.0.0.0"
    }

    addrs, err:=int.Addrs()
    if err!=nil || len(addrs)==0{
        return "0.0.0.0"
    }

    addr:=addrs[0].String()
    i:=strings.Index(addr, "/")
    if i==-1{
        return addr
    }

    return addr[:i]
}

func main(){
    dpy:=C.XOpenDisplay(nil)
    defer C.XCloseDisplay(dpy)
    for{
        status_string:=" "+get_mem_avail()+" ◌ "+get_local_ip_address()+" ◌ "+get_datetime()+" "
        C.XStoreName(dpy, C.XDefaultRootWindow(dpy), C.CString(status_string))
        C.XSync(dpy, 0)
        time.Sleep(time.Second)
    }
}
