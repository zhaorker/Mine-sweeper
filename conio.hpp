#ifndef CONIO_HPP
#define CONIO_HPP 

#if defined(_WIN32)

#include<conio.h>
#include<windows.h>

#endif
#if defined(__linux__)

#include<termios.h>
#include<unistd.h>
#include<poll.h>
#include<sys/ioctl.h>
#include<utility>

#endif

class Conioer
{
private:
    static int object_count;
    static bool is_standard;
    #if defined(__linux__)
    termios save_state;
    #endif
    bool is_valid;
public:
    Conioer(const Conioer&)=delete;
    Conioer& operator=(const Conioer&)=delete;
    Conioer(Conioer&&)=delete;
    Conioer& operator=(Conioer&&)=delete;
    Conioer()
    {
        if(object_count==1)
        {
            is_valid=false;
            return;
        }
        is_valid=true;
        ++object_count;
        #if defined(__linux__)
        ::tcgetattr(0,&save_state);
        #endif
        unlock();
    }
    ~Conioer()
    {
        lock();
        --object_count;
    }
    int getch()
    {
        if(!is_valid || is_standard)
            return 0;
        char a=0;
        #if defined(_WIN32)
        a=::getch();
        #else
        read(0,&a,1);
        #endif
        return a;
    }
    bool kbhit()
    {
        if(!is_valid)
            return false;

        #if defined(_WIN32)

        return ::kbhit();
        
        #else
        
        pollfd hint;
        hint.fd=0;
        hint.events=POLLIN;
        int ret=::poll(&hint,1,0);
        if(ret==-1)
            return false;
        if(hint.revents&POLLIN)
            return true;
        return false;
        
        #endif
    }
    //lock_guard<Conioer>
    void lock()
    {
        #if defined(__linux__)
        if(!is_valid || is_standard)
            return;
        tcsetattr(0,TCSANOW,&save_state);
        #endif
        is_standard=true;
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
        tcsetattr(0,TCSANOW/*立即执行*/,&tstate);
        #endif
        is_standard=false;
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
int Conioer::object_count=0;
bool Conioer::is_standard=true;

#endif
