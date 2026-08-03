#include<termios.h>
#include<unistd.h>
#include<sys/poll.h>
#include<sys/ioctl.h>
#include<utility>

class Conioer
{
private:
    static int object_count;
    static bool is_standard;
    termios save_state;
    bool is_valid;
public:
    Conioer()
    {
        if(object_count==1)
        {
            is_valid=false;
            return;
        }
        is_valid=true;
        ::tcgetattr(0,&save_state);
        ++object_count;
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
        char a;
        read(0,&a,1);
        return a;
    }
    bool kbhit()
    {
        if(!is_valid)
            return false;
        pollfd hint;
        hint.fd=0;
        hint.events=POLLIN;
        int ret=::poll(&hint,1,0);
        if(ret==-1)
            return false;
        if(hint.revents&POLLIN)
            return true;
        return false;
    }
    //lock_guard<Conioer>
    void lock()
    {
        if(!is_valid || is_standard)
            return;
        tcsetattr(0,TCSANOW,&save_state);
        is_standard=true;
    }
    void unlock()
    {
        if(!is_valid || !is_standard)
            return;
        termios tstate=save_state;
        tstate.c_iflag |= IGNBRK|INLCR|IUTF8;
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
        is_standard=false;
    }
    //行数：列数
    static std::pair<int,int> size()
    {
        winsize size_pair{};
        ::ioctl(STDOUT_FILENO,TIOCGWINSZ,&size_pair);
        return {size_pair.ws_row,size_pair.ws_col};
    }
    bool valid()
    {
        return is_valid;
    }
};
int Conioer::object_count=0;
bool Conioer::is_standard=true;