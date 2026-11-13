#include<cstdlib>
#include<cstdio>
#include<vector>
#include<chrono>
#include<thread>
#include"conio.hpp"
#include<string>
#include<random>
#include<memory>
#ifdef _WIN32
#include<windows.h>
#endif
Conioer io;
constexpr int wait_time=50000;
int times=50;
struct point{
	int x;
	int y;
};

constexpr int length=15,height=24,blank=4;
std::vector<std::vector<int>>bnet(height,std::vector<int>(length));//20*10,预留四行不显示

std::vector<std::vector<point>> shapelist{
	std::vector<point>{{0,0},{0,1},{0,-1},{1,-1}},
	std::vector<point>{{0,0},{0,1},{0,-1},{-1,-1}},
	std::vector<point>{{0,0},{0,1},{1,0},{-1,0}},
	std::vector<point>{{0,0},{0,1},{1,0},{1,-1}},
	std::vector<point>{{0,0},{0,1},{-1,0},{-1,-1}},
	std::vector<point>{{0,0},{-1,0},{1,0},{2,0}},
	std::vector<point>{{0,0},{1,0},{0,1},{1,1}}
};
std::default_random_engine eng(time(0));
std::uniform_int_distribution<int> shaperange(0,shapelist.size()-1);
std::uniform_int_distribution<int> positionrange(2,length-3);
std::uniform_int_distribution<int> colorrange(101,106);
class basebox{
private:
	// point *shape,*realout,*newout;
	std::vector<point> shape,realout;
	int direction;
	bool ifturn;
	int position_x,position_y;
	int color;
public:
	bool realset(char op,int x,int y,int newd){
		switch(op){
		case 'd':x++;break;
		case 'a':x--;break;
		case 's':y--;break;
		case 'w':if(ifturn) newd=(newd+1)%4;
		}
		auto newout=shape;//其实不关心内容，只是把newout的大小设为和shape一样
		for(int i=0; i < shape.size(); ++i){
			switch(newd){
				case 0:newout[i]={x+shape[i].x,y+shape[i].y};break;
				case 1:newout[i]={x-shape[i].y,y+shape[i].x};break;
				case 2:newout[i]={x-shape[i].x,y-shape[i].y};break;
				case 3:newout[i]={x+shape[i].y,y-shape[i].x};break;
			}
			if(newout[i].y<0 || bnet[newout[i].y][newout[i].x])
				return false;//运动失败返回false
			else if(newout[i].x>=length)
				return realset('a',x,y,newd);//翻转（w）引发出界时自动纠正位置，当然，如果尝试自动纠正的移动时碰到了其他块就会导致返回false
			else if(newout[i].x<0)
				return realset('d',x,y,newd);
		}
		position_x=x;
		position_y=y;
		direction=newd;
		realout=std::move(newout);
		return true;//返回true代表形态有发生变化
	}
	void putall(){
		printf("\033[H");
		for(auto [x,y]:realout) bnet[y][x]=color;
		for(int y=height-blank-1;y >= 0;y--){
			for(int value: bnet[y]){
				if(value)printf("\033[%dm  \033[0m",value);
				else printf("  ");
			}
			printf("|\n");
		}
		std::string bottom((length)*2+1,'-');
		printf("%s\n%d",bottom.c_str(),times-50);
		for(auto [x,y]:realout) bnet[y][x]=0;
	}
	basebox(int i):
		ifturn(i!=6),
		position_x(positionrange(eng)),
		position_y(height-blank),direction(0),
		realout{},shape(shapelist[i]),
		color{colorrange(eng)}
	{}
	void drop()
	{
		while(realset('s',position_x,position_y,direction)){
			putall();
			auto now=std::chrono::steady_clock::now();
			while(std::chrono::steady_clock::now()-now < std::chrono::milliseconds(wait_time/times)){
				if(io.kbhit()){
					char ch=io.getch();
					if(ch==3)
						std::exit(-1);
					if((realset(ch,position_x,position_y,direction)))
						putall();
				}
			}
		}
		for(auto [x,y] :realout)
			bnet[y][x]=color;
		times++;
		realout.clear();
		putall();
	}
	~basebox(){}
};
int main(){
#ifdef _WIN32
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
#endif
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	while(1){
		basebox(shaperange(eng)).drop();
		for(int y=bnet.size()-1 ;y>=0 ;--y)
		{
			bool iffull=true;
			for(int value:bnet[y]) iffull =iffull && value>0;
			if(iffull)
			{
				bnet.erase(bnet.begin()+y);
				bnet.emplace_back(length,0);
			}
		}

		bool ifreach=0;
		for(int value:bnet[height-blank-1]) ifreach =ifreach || value>0;
		if(ifreach){
			putchar('\a');
			printf("you loss\n");
			io.getch();
			return 0;
		}
	}
}