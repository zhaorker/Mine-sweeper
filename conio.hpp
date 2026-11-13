#ifndef CONIO_HPP
#define CONIO_HPP 

#include<utility>
#include<cstdint>

#if defined(_WIN32)

#include<conio.h>
#include<windows.h>

#endif
#if defined(__linux__)

#include<termios.h>
#include<unistd.h>
#include<poll.h>
#include<sys/ioctl.h>

#endif

class Conioer
{
private:
    #if defined(__linux__)
    static inline bool is_standard=true;
    termios save_state;
    static inline int object_count=0;
    #endif
    bool is_valid;
public:
    Conioer(const Conioer&)=delete;
    Conioer& operator=(const Conioer&)=delete;
    Conioer(Conioer&&)=delete;
    Conioer& operator=(Conioer&&)=delete;
    Conioer():is_valid{false}
    {
#ifdef _WIN32
        is_valid=true;
#else
        if(object_count!=0) return;
        if(::tcgetattr(STDIN_FILENO,&save_state)!=0) return;
        is_valid=true;
        ++object_count;
        unlock();
#endif
    }
    ~Conioer()
    {
#ifdef __linux__
        if(!is_valid) return;
        lock();
        --object_count;
#endif
    }
    int getch()
    {
#ifdef _WIN32
        if(!is_valid) return 0;
        return ::_getch();
#else
        if(!is_valid || is_standard) return 0;
        unsigned char result;
        if(::read(STDIN_FILENO,&result,1)<=0) return 0;
        return static_cast<int>(result);
#endif
    }
    bool kbhit()
    {
#ifdef _WIN32
        return is_valid && ::_kbhit();
#else
        if(!is_valid || is_standard) return false;

        pollfd fd{};
        fd.fd=STDIN_FILENO;
        fd.events=POLLIN;
        if(::poll(&fd,1,0)<=0) return false;
        return (fd.revents & POLLIN) !=0;
#endif
    }
    //lock_guard<Conioer>
    void lock()
    {
#if defined(__linux__)
        if(!is_valid || is_standard)
            return;
        tcsetattr(STDIN_FILENO,TCSANOW,&save_state);
        is_standard=true;
#endif
}
    void unlock()
    {
#if defined(__linux__)
        if(!is_valid || !is_standard)
            return;
        termios tstate=save_state;
        tstate.c_iflag |= IGNBRK|INLCR;
        #ifdef IUTF8
        tstate.c_iflag |= IUTF8;
        #endif
        tstate.c_iflag &= ~(INPCK|
                            ISTRIP|
                            IGNCR|
                            ICRNL|
                            IUCLC);
        //oflag保持默认就好
        tstate.c_lflag |=NOFLSH;
        tstate.c_lflag &= ~(ISIG|
                            ICANON|
                            ECHO);
        tstate.c_cc[VMIN]=1;
        tstate.c_cc[VTIME]=0;
        tcsetattr(STDIN_FILENO,TCSANOW/*立即执行*/,&tstate);
        is_standard=false;
#endif
    }
#if defined(__linux__)
    //行数：列数
    static std::pair<int,int> size()
    {
        winsize size_pair{};
        ::ioctl(STDOUT_FILENO,TIOCGWINSZ,&size_pair);
        return {size_pair.ws_row,size_pair.ws_col};
    }
#endif
#if defined(_WIN32)
    static std::pair<int, int> size()
    {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hConsole, &csbi))
        {
            int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
            int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            return {rows, cols};
        }
        return {24, 80};//失败时返回的默认值
    }
#endif
    bool valid()
    {
        return is_valid;
    }
};

#endif
